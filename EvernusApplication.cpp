/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unordered_set>
#include <stdexcept>
#include <algorithm>
#include <future>

#include <QStandardPaths>
#include <QSplashScreen>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QSet>

#include "ExternalOrderImporterNames.h"
#include "LanguageSelectDialog.h"
#include "UploaderSettings.h"
#include "UpdaterSettings.h"
#include "ImportSettings.h"
#include "WalletSettings.h"
#include "PriceSettings.h"
#include "PathSettings.h"
#include "HttpSettings.h"
#include "SyncSettings.h"
#include "IGBSettings.h"
#include "SimpleCrypt.h"
#include "HttpService.h"
#include "IGBService.h"
#include "SyncDialog.h"
#include "UISettings.h"
#include "PathUtils.h"
#include "Updater.h"

#include "qxtmailmessage.h"

#include "EvernusApplication.h"

namespace Evernus
{
    const QString EvernusApplication::versionKey = "version";

    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication(argc, argv)
        , ExternalOrderImporterRegistry()
        , AssetProvider()
        , CacheTimerProvider()
        , ItemCostProvider()
        , RepositoryProvider()
        , LMeveDataProvider()
        , mMainDb(QSqlDatabase::addDatabase("QSQLITE", "main"))
        , mEveDb(QSqlDatabase::addDatabase("QSQLITE", "eve"))
        , mAPIManager(*this)
    {
        QSettings settings;

        auto lang = settings.value(UISettings::languageKey).toString();
        if (lang.isEmpty())
        {
            lang = QLocale{}.name();
            updateTranslator(lang);

            LanguageSelectDialog dlg;
            if (dlg.exec() == QDialog::Accepted)
            {
                lang = dlg.getSelectedLanguage();
                settings.setValue(UISettings::languageKey, lang);
                updateTranslator(lang);
            }
            else
            {
                settings.setValue(UISettings::languageKey, lang);
            }
        }
        else
        {
            updateTranslator(lang);
        }

#ifdef EVERNUS_DROPBOX_ENABLED
        if (settings.value(SyncSettings::enabledOnStartupKey, SyncSettings::enabledOnStartupDefault).toBool())
        {
            SyncDialog syncDlg{SyncDialog::Mode::Download};
            syncDlg.exec();
        }
#endif

        QSplashScreen splash{QPixmap{":/images/splash.png"}};
        splash.show();
        showSplashMessage(tr("Loading..."), splash);

        showSplashMessage(tr("Creating databases..."), splash);
        createDb();

        showSplashMessage(tr("Creating schemas..."), splash);
        createDbSchema();

        mCharacterOrderProvider = std::make_unique<CachingMarketOrderProvider>(*mMarketOrderRepository);
        connect(mCharacterOrderProvider.get(), &CachingMarketOrderProvider::orderDeleted,
                this, &EvernusApplication::scheduleMarketOrderChange);

        mCorpOrderProvider = std::make_unique<CachingMarketOrderProvider>(*mCorpMarketOrderRepository);
        connect(mCorpOrderProvider.get(), &CachingMarketOrderProvider::orderDeleted,
                this, &EvernusApplication::scheduleCorpMarketOrderChange);

        mCombinedOrderProvider
            = std::make_unique<CharacterCorporationCombinedMarketOrderProvider>(*mCharacterOrderProvider, *mCorpOrderProvider);

        mCharacterContractProvider = std::make_unique<CachingContractProvider>(*mContractRepository);
        mCorpContractProvider = std::make_unique<CachingContractProvider>(*mCorpContractRepository);

        mDataProvider = std::make_unique<CachingEveDataProvider>(*mEveTypeRepository,
                                                                 *mMetaGroupRepository,
                                                                 *mExternalOrderRepository,
                                                                 *mMarketOrderRepository,
                                                                 *mCorpMarketOrderRepository,
                                                                 *mConquerableStationRepository,
                                                                 *mMarketGroupRepository,
                                                                 *mRefTypeRepository,
                                                                 mAPIManager,
                                                                 mEveDb);

        showSplashMessage(tr("Precaching ref types..."), splash);
        mDataProvider->precacheRefTypes();

        showSplashMessage(tr("Precaching timers..."), splash);
        precacheCacheTimers();
        precacheUpdateTimers();

        showSplashMessage(tr("Precaching jump map..."), splash);
        mDataProvider->precacheJumpMap();

        showSplashMessage(tr("Clearing old wallet entries..."), splash);
        deleteOldWalletEntries();

        showSplashMessage(tr("Setting up IGB service..."), splash);
        auto igbService = new IGBService{*mCharacterOrderProvider,
                                         *mCorpOrderProvider,
                                         *mDataProvider,
                                         *this,
                                         *mFavoriteItemRepository,
                                         *mCharacterRepository,
                                         &mIGBSessionManager,
                                         this};
        connect(igbService, SIGNAL(openMarginTool()), this, SIGNAL(openMarginTool()));
        connect(igbService, &IGBService::importFromCache, this, [this] {
            refreshExternalOrdersFromCache(ExternalOrderImporter::TypeLocationPairs{});
        });
        connect(this, &EvernusApplication::showInEve, igbService, &IGBService::showInEve);

        mIGBSessionManager.setPort(settings.value(IGBSettings::portKey, IGBSettings::portDefault).value<quint16>());
        mIGBSessionManager.setListenInterface(QHostAddress::LocalHost);
        mIGBSessionManager.setStaticContentService(igbService);
        mIGBSessionManager.setConnector(QxtHttpSessionManager::HttpServer);

        if (settings.value(IGBSettings::enabledKey, IGBSettings::enabledDefault).toBool())
            mIGBSessionManager.start();

        showSplashMessage(tr("Setting up HTTP service..."), splash);
        auto httpService = new HttpService{*mCombinedOrderProvider,
                                           *mCorpOrderProvider,
                                           *mDataProvider,
                                           *mCharacterRepository,
                                           *this,
                                           *this,
                                           &mHttpSessionManager,
                                           this};

        mHttpSessionManager.setPort(settings.value(HttpSettings::portKey, HttpSettings::portDefault).value<quint16>());
        mHttpSessionManager.setStaticContentService(httpService);
        mHttpSessionManager.setConnector(QxtHttpSessionManager::HttpServer);

        if (settings.value(HttpSettings::enabledKey, HttpSettings::enabledDefault).toBool())
            mHttpSessionManager.start();

        showSplashMessage(tr("Loading..."), splash);

        Updater::getInstance().performVersionMigration(*mCacheTimerRepository,
                                                       *mUpdateTimerRepository,
                                                       *mCharacterRepository,
                                                       *mExternalOrderRepository,
                                                       *mMarketOrderRepository,
                                                       *mCorpMarketOrderRepository,
                                                       *mWalletJournalEntryRepository,
                                                       *mCorpWalletJournalEntryRepository,
                                                       *mWalletTransactionRepository,
                                                       *mCorpWalletTransactionRepository,
                                                       *mMarketOrderValueSnapshotRepository,
                                                       *mCorpMarketOrderValueSnapshotRepository);

        settings.setValue(versionKey, applicationVersion());

        connect(&mSmtp, SIGNAL(authenticationFailed(const QByteArray &)), SLOT(showSmtpError(const QByteArray &)));
        connect(&mSmtp, SIGNAL(connectionFailed(const QByteArray &)), SLOT(showSmtpError(const QByteArray &)));
        connect(&mSmtp, SIGNAL(encryptionFailed(const QByteArray &)), SLOT(showSmtpError(const QByteArray &)));
        connect(&mSmtp, &QxtSmtp::finished, &mSmtp, &QxtSmtp::disconnectFromHost);
        setSmtpSettings();

        connect(&mAPIManager, &APIManager::generalError, this, &EvernusApplication::apiError);

        mMarketUploader = std::make_unique<Uploader>(*mExternalOrderRepository);
        connect(this, &EvernusApplication::externalOrdersChanged, mMarketUploader.get(), &Uploader::dataChanged);
        connect(mMarketUploader.get(), &Uploader::statusChanged, this, &EvernusApplication::uploaderStatusChanged);

        if (settings.value(UpdaterSettings::autoUpdateKey, UpdaterSettings::autoUpdateDefault).toBool())
            Updater::getInstance().checkForUpdates(true);
    }

    void EvernusApplication::registerImporter(const std::string &name, std::unique_ptr<ExternalOrderImporter> &&importer)
    {
        Q_ASSERT(mExternalOrderImporters.find(name) == std::end(mExternalOrderImporters));

        connect(importer.get(), &ExternalOrderImporter::error, this, &EvernusApplication::showPriceImportError, Qt::QueuedConnection);
        connect(importer.get(), &ExternalOrderImporter::externalOrdersChanged, this, &EvernusApplication::updateExternalOrdersAndAssetValue, Qt::QueuedConnection);
        mExternalOrderImporters.emplace(name, std::move(importer));
    }

    std::shared_ptr<AssetList> EvernusApplication::fetchAssetsForCharacter(Character::IdType id) const
    {
        if (id == Character::invalidId)
            return std::make_shared<AssetList>();

        const auto it = mCharacterAssets.find(id);
        if (it != std::end(mCharacterAssets))
            return it->second;

        AssetListRepository::EntityPtr assets;

        try
        {
            assets = mAssetListRepository->fetchForCharacter(id);
        }
        catch (const AssetListRepository::NotFoundException &)
        {
            assets = std::make_shared<AssetList>();
            assets->setCharacterId(id);
        }

        mCharacterAssets.emplace(id, assets);
        return assets;
    }

    QDateTime EvernusApplication::getLocalCacheTimer(Character::IdType id, TimerType type) const
    {
        CharacterTimerMap::const_iterator it;

        switch (type) {
        case TimerType::Character:
            it = mCharacterUtcCacheTimes.find(id);
            if (it == std::end(mCharacterUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::AssetList:
            it = mAssetsUtcCacheTimes.find(id);
            if (it == std::end(mAssetsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::WalletJournal:
            it = mWalletJournalUtcCacheTimes.find(id);
            if (it == std::end(mWalletJournalUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::WalletTransactions:
            it = mWalletTransactionsUtcCacheTimes.find(id);
            if (it == std::end(mWalletTransactionsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::MarketOrders:
            it = mMarketOrdersUtcCacheTimes.find(id);
            if (it == std::end(mMarketOrdersUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::Contracts:
            it = mContractsUtcCacheTimes.find(id);
            if (it == std::end(mContractsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::CorpWalletJournal:
            it = mCorpWalletJournalUtcCacheTimes.find(id);
            if (it == std::end(mCorpWalletJournalUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::CorpWalletTransactions:
            it = mCorpWalletTransactionsUtcCacheTimes.find(id);
            if (it == std::end(mCorpWalletTransactionsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::CorpMarketOrders:
            it = mCorpMarketOrdersUtcCacheTimes.find(id);
            if (it == std::end(mCorpMarketOrdersUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::CorpContracts:
            it = mCorpContractsUtcCacheTimes.find(id);
            if (it == std::end(mCorpContractsUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        case TimerType::LMeveTasks:
            it = mLMeveUtcCacheTimes.find(id);
            if (it == std::end(mLMeveUtcCacheTimes))
                return QDateTime::currentDateTime();
            break;
        default:
            throw std::logic_error{tr("Unknown cache timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        return it->second.toLocalTime();
    }

    void EvernusApplication::setUtcCacheTimer(Character::IdType id, TimerType type, const QDateTime &dt)
    {
        if (!dt.isValid())
            return;

        switch (type) {
        case TimerType::Character:
            mCharacterUtcCacheTimes[id] = dt;
            break;
        case TimerType::AssetList:
            mAssetsUtcCacheTimes[id] = dt;
            break;
        case TimerType::WalletJournal:
            mWalletJournalUtcCacheTimes[id] = dt;
            break;
        case TimerType::WalletTransactions:
            mWalletTransactionsUtcCacheTimes[id] = dt;
            break;
        case TimerType::MarketOrders:
            mMarketOrdersUtcCacheTimes[id] = dt;
            break;
        case TimerType::Contracts:
            mContractsUtcCacheTimes[id] = dt;
            break;
        case TimerType::CorpWalletJournal:
            mCorpWalletJournalUtcCacheTimes[id] = dt;
            break;
        case TimerType::CorpWalletTransactions:
            mCorpWalletTransactionsUtcCacheTimes[id] = dt;
            break;
        case TimerType::CorpMarketOrders:
            mCorpMarketOrdersUtcCacheTimes[id] = dt;
            break;
        case TimerType::CorpContracts:
            mCorpContractsUtcCacheTimes[id] = dt;
            break;
        case TimerType::LMeveTasks:
            mLMeveUtcCacheTimes[id] = dt;
            break;
        default:
            throw std::logic_error{tr("Unknown cache timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        if (type != TimerType::Character)
        {
            CacheTimer timer;
            timer.setCharacterId(id);
            timer.setType(type);
            timer.setCacheUntil(dt);

            mCacheTimerRepository->store(timer);
        }
    }

    QDateTime EvernusApplication::getLocalUpdateTimer(Character::IdType id, TimerType type) const
    {
        CharacterTimerMap::const_iterator it;

        switch (type) {
        case TimerType::Character:
            it = mCharacterUtcUpdateTimes.find(id);
            if (it == std::end(mCharacterUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::AssetList:
            it = mAssetsUtcUpdateTimes.find(id);
            if (it == std::end(mAssetsUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::WalletJournal:
            it = mWalletJournalUtcUpdateTimes.find(id);
            if (it == std::end(mWalletJournalUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::WalletTransactions:
            it = mWalletTransactionsUtcUpdateTimes.find(id);
            if (it == std::end(mWalletTransactionsUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::MarketOrders:
            it = mMarketOrdersUtcUpdateTimes.find(id);
            if (it == std::end(mMarketOrdersUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::Contracts:
            it = mContractsUtcUpdateTimes.find(id);
            if (it == std::end(mContractsUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::CorpWalletJournal:
            it = mCorpWalletJournalUtcUpdateTimes.find(id);
            if (it == std::end(mCorpWalletJournalUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::CorpWalletTransactions:
            it = mCorpWalletTransactionsUtcUpdateTimes.find(id);
            if (it == std::end(mCorpWalletTransactionsUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::CorpMarketOrders:
            it = mCorpMarketOrdersUtcUpdateTimes.find(id);
            if (it == std::end(mCorpMarketOrdersUtcUpdateTimes))
                return QDateTime{};
            break;
        case TimerType::CorpContracts:
            it = mCorpContractsUtcUpdateTimes.find(id);
            if (it == std::end(mCorpContractsUtcUpdateTimes))
                return QDateTime{};
            break;
        default:
            throw std::logic_error{tr("Unknown update timer type: %1").arg(static_cast<int>(type)).toStdString()};
        }

        return it->second.toLocalTime();
    }

    std::shared_ptr<ItemCost> EvernusApplication::fetchForCharacterAndType(Character::IdType characterId, EveType::IdType typeId) const
    {
        const auto it = mCharacterItemCostCache.find(std::make_pair(characterId, typeId));
        if (it != std::end(mCharacterItemCostCache))
            return it->second;

        ItemCostRepository::EntityPtr cost;

        try
        {
            cost = mItemCostRepository->fetchForCharacterAndType(characterId, typeId);
        }
        catch (const ItemCostRepository::NotFoundException &)
        {
            QSettings settings;
            if (settings.value(PriceSettings::shareCostsKey, PriceSettings::shareCostsDefault).toBool())
            {
                const auto it = mTypeItemCostCache.find(typeId);
                if (it != std::end(mTypeItemCostCache))
                {
                    mCharacterItemCostCache.emplace(std::make_pair(characterId, typeId), it->second);
                    return it->second;
                }

                try
                {
                    cost = mItemCostRepository->fetchLatestForType(typeId);
                }
                catch (const ItemCostRepository::NotFoundException &)
                {
                    cost = std::make_shared<ItemCost>();
                }

                mTypeItemCostCache.emplace(typeId, cost);
            }
            else
            {
                cost = std::make_shared<ItemCost>();
            }
        }

        mCharacterItemCostCache.emplace(std::make_pair(characterId, typeId), cost);
        return cost;
    }

    ItemCostProvider::CostList EvernusApplication::fetchForCharacter(Character::IdType characterId) const
    {
        return mItemCostRepository->fetchForCharacter(characterId);
    }

    void EvernusApplication::setForCharacterAndType(Character::IdType characterId, EveType::IdType typeId, double value)
    {
        auto cost = std::make_shared<ItemCost>();
        cost->setCharacterId(characterId);
        cost->setTypeId(typeId);
        cost->setCost(value);

        mItemCostRepository->store(*cost);

        mCharacterItemCostCache[std::make_pair(characterId, typeId)] = cost;
        mTypeItemCostCache[typeId] = cost;

        if (!mItemCostUpdateScheduled)
        {
            mItemCostUpdateScheduled = true;
            QMetaObject::invokeMethod(this, "emitNewItemCosts", Qt::QueuedConnection);
        }
    }

    std::shared_ptr<ItemCost> EvernusApplication::findItemCost(ItemCost::IdType id) const
    {
        return mItemCostRepository->find(id);
    }

    void EvernusApplication::removeItemCost(ItemCost::IdType id) const
    {
        mItemCostRepository->remove(id);
        mCharacterItemCostCache.clear();
        mTypeItemCostCache.clear();

        emit itemCostsChanged();
    }

    void EvernusApplication::storeItemCost(ItemCost &cost) const
    {
        mItemCostRepository->store(cost);

        auto sharedCost = std::make_shared<ItemCost>(cost);

        mCharacterItemCostCache[std::make_pair(cost.getCharacterId(), cost.getTypeId())] = sharedCost;
        mTypeItemCostCache[cost.getTypeId()] = sharedCost;

        emit itemCostsChanged();
    }

    void EvernusApplication::removeAllItemCosts(Character::IdType characterId) const
    {
        mItemCostRepository->removeForCharacter(characterId);
        mCharacterItemCostCache.clear();
        mTypeItemCostCache.clear();

        emit itemCostsChanged();
    }

    const KeyRepository &EvernusApplication::getKeyRepository() const noexcept
    {
        return *mKeyRepository;
    }

    const CorpKeyRepository &EvernusApplication::getCorpKeyRepository() const noexcept
    {
        return *mCorpKeyRepository;
    }

    const CharacterRepository &EvernusApplication::getCharacterRepository() const noexcept
    {
        return *mCharacterRepository;
    }

    const WalletSnapshotRepository &EvernusApplication::getWalletSnapshotRepository() const noexcept
    {
        return *mWalletSnapshotRepository;
    }

    const CorpWalletSnapshotRepository &EvernusApplication::getCorpWalletSnapshotRepository() const noexcept
    {
        return *mCorpWalletSnapshotRepository;
    }

    const AssetValueSnapshotRepository &EvernusApplication::getAssetValueSnapshotRepository() const noexcept
    {
        return *mAssetValueSnapshotRepository;
    }

    const WalletJournalEntryRepository &EvernusApplication::getWalletJournalEntryRepository() const noexcept
    {
        return *mWalletJournalEntryRepository;
    }

    const WalletTransactionRepository &EvernusApplication::getWalletTransactionRepository() const noexcept
    {
        return *mWalletTransactionRepository;
    }

    const MarketOrderRepository &EvernusApplication::getMarketOrderRepository() const noexcept
    {
        return *mMarketOrderRepository;
    }

    const WalletJournalEntryRepository &EvernusApplication::getCorpWalletJournalEntryRepository() const noexcept
    {
        return *mCorpWalletJournalEntryRepository;
    }

    const WalletTransactionRepository &EvernusApplication::getCorpWalletTransactionRepository() const noexcept
    {
        return *mCorpWalletTransactionRepository;
    }

    const MarketOrderRepository &EvernusApplication::getCorpMarketOrderRepository() const noexcept
    {
        return *mCorpMarketOrderRepository;
    }

    const ItemCostRepository &EvernusApplication::getItemCostRepository() const noexcept
    {
        return *mItemCostRepository;
    }

    const MarketOrderValueSnapshotRepository &EvernusApplication::getMarketOrderValueSnapshotRepository() const noexcept
    {
        return *mMarketOrderValueSnapshotRepository;
    }

    const CorpMarketOrderValueSnapshotRepository &EvernusApplication::getCorpMarketOrderValueSnapshotRepository() const noexcept
    {
        return *mCorpMarketOrderValueSnapshotRepository;
    }

    const FilterTextRepository &EvernusApplication::getFilterTextRepository() const noexcept
    {
        return *mFilterTextRepository;
    }

    const OrderScriptRepository &EvernusApplication::getOrderScriptRepository() const noexcept
    {
        return *mOrderScriptRepository;
    }

    const FavoriteItemRepository &EvernusApplication::getFavoriteItemRepository() const noexcept
    {
        return *mFavoriteItemRepository;
    }

    const LocationBookmarkRepository &EvernusApplication::getLocationBookmarkRepository() const noexcept
    {
        return *mLocationBookmarkRepository;
    }

    const ExternalOrderRepository &EvernusApplication::getExternalOrderRepository() const noexcept
    {
        return *mExternalOrderRepository;
    }

    std::vector<std::shared_ptr<LMeveTask>> EvernusApplication::getTasks(Character::IdType characterId) const
    {
        const auto it = mLMeveTaskCache.find(characterId);
        if (it != std::end(mLMeveTaskCache))
            return it->second;

        return mLMeveTaskCache.emplace(characterId, mLMeveTaskRepository->fetchForCharacter(characterId)).first->second;
    }

    MarketOrderProvider &EvernusApplication::getMarketOrderProvider() const noexcept
    {
        return *mCombinedOrderProvider;
    }

    MarketOrderProvider &EvernusApplication::getCorpMarketOrderProvider() const noexcept
    {
        return *mCorpOrderProvider;
    }

    const ContractProvider &EvernusApplication::getContractProvider() const noexcept
    {
        return *mCharacterContractProvider;
    }

    const ContractProvider &EvernusApplication::getCorpContractProvider() const noexcept
    {
        return *mCorpContractProvider;
    }

    EveDataProvider &EvernusApplication::getDataProvider() noexcept
    {
        return *mDataProvider;
    }

    void EvernusApplication::refreshCharacters()
    {
        qDebug() << "Refreshing characters...";

        const auto task = startTask(tr("Fetching characters..."));

        const auto keys = mKeyRepository->fetchAll();

        if (keys.empty())
        {
            mCharacterRepository->exec(QString{"UPDATE %1 SET key_id = NULL, enabled = 0"}.arg(mCharacterRepository->getTableName()));
            emit taskEnded(task, QString{});
        }
        else
        {
            for (const auto &key : keys)
            {
                const auto charListSubtask = startTask(task, tr("Fetching characters for key %1...").arg(key->getId()));
                mAPIManager.fetchCharacterList(*key, [key, charListSubtask, this](const auto &characters, const auto &error) {
                    if (error.isEmpty())
                    {
                        try
                        {
                            if (characters.empty())
                                mCharacterRepository->disableByKey(key->getId());
                            else
                                mCharacterRepository->disableByKey(key->getId(), characters);
                        }
                        catch (const std::exception &e)
                        {
                            QMessageBox::warning(activeWindow(), tr("Evernus"), tr("An error occurred while updating character key information: %1. "
                                "Data sync should work, but character tab will display incorrect information.").arg(e.what()));
                            return;
                        }
                        catch (...)
                        {
                            QMessageBox::warning(activeWindow(), tr("Evernus"), tr("An error occurred while updating character key information. "
                                "Data sync should work, but character tab will display incorrect information."));
                            return;
                        }

                        if (characters.empty())
                        {
                            emit taskEnded(charListSubtask, error);
                        }
                        else
                        {
                            for (const auto id : characters)
                                importCharacter(id, charListSubtask, *key);
                        }
                    }
                    else
                    {
                        emit taskEnded(charListSubtask, error);
                    }
                });
            }
        }
    }

    void EvernusApplication::refreshCharacter(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing character: " << id;

        try
        {
            importCharacter(id, parentTask, *getCharacterKey(id));
        }
        catch (const KeyRepository::NotFoundException &)
        {
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }
    }

    void EvernusApplication::refreshAssets(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing assets: " << id;

        const auto assetSubtask = startTask(parentTask, tr("Fetching assets for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::AssetList, assetSubtask))
            return;

        try
        {
            const auto key = getCharacterKey(id);
            mAPIManager.fetchAssets(*key, id, [assetSubtask, id, this](auto &&data, const auto &error) {
                if (error.isEmpty())
                {
                    mAssetListRepository->deleteForCharacter(id);

                    const auto it = mCharacterAssets.find(id);
                    if (it != std::end(mCharacterAssets))
                        *it->second = data;

                    mAssetListRepository->store(data);

                    QSettings settings;

                    if (settings.value(Evernus::ImportSettings::autoUpdateAssetValueKey, Evernus::ImportSettings::autoUpdateAssetValueDefault).toBool())
                        computeAssetListSellValue(data);

                    saveUpdateTimer(Evernus::TimerType::AssetList, mAssetsUtcUpdateTimes, id);

                    emit assetsChanged();
                }

                emit taskEnded(assetSubtask, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(assetSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(assetSubtask, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshContracts(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing contracts: " << id;

        const auto task = startTask(tr("Fetching contracts for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::Contracts, task))
            return;

        try
        {
            const auto key = getCharacterKey(id);
            mAPIManager.fetchContracts(*key, id, [key, task, id, this](auto &&data, const auto &error) {
                if (error.isEmpty())
                {
                    const auto it = std::remove_if(std::begin(data), std::end(data), [](const auto &contract) {
                        return contract.isForCorp();
                    });

                    if (it != std::end(data))
                    {
                        Evernus::Contracts corpContracts(it, std::end(data));
                        asyncBatchStore(*mCorpContractRepository, corpContracts, true);

                        mCharacterContractProvider->clearForCorporation(corpContracts.front().getIssuerCorpId());
                        mCorpContractProvider->clearForCorporation(corpContracts.front().getIssuerCorpId());
                        mCharacterContractProvider->clearForCorporation(corpContracts.front().getAssigneeId());
                        mCorpContractProvider->clearForCorporation(corpContracts.front().getAssigneeId());
                    }

                    asyncBatchStore(*mContractRepository, data, true);

                    mCharacterContractProvider->clearForCharacter(id);
                    mCorpContractProvider->clearForCharacter(id);

                    saveUpdateTimer(Evernus::TimerType::Contracts, mContractsUtcUpdateTimes, id);

                    this->handleIncomingContracts<&Evernus::EvernusApplication::contractsChanged>(key,
                                                                                                  data,
                                                                                                  id,
                                                                                                  *mContractItemRepository,
                                                                                                  task);
                }
                else
                {
                    emit taskEnded(task, error);
                }
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshWalletJournal(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing wallet journal: " << id;

        const auto task = startTask(tr("Fetching wallet journal for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        try
        {
            const auto key = getCharacterKey(id);
            const auto maxId = mWalletJournalEntryRepository->getLatestEntryId(id);

            if (maxId == WalletJournalEntry::invalidId)
                emit taskInfoChanged(task, tr("Fetching wallet journal for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletJournal(*key, id, WalletJournalEntry::invalidId, maxId,
                                           [task, id, this](const auto &data, const auto &error) {
                if (error.isEmpty())
                {
                    std::vector<Evernus::WalletSnapshot> snapshots;
                    snapshots.reserve(data.size());

                    QSet<QDateTime> usedSnapshots;

                    for (auto &entry : data)
                    {
                        const auto timestamp = entry.getTimestamp();

                        if (!usedSnapshots.contains(timestamp))
                        {
                            Evernus::WalletSnapshot snapshot;
                            snapshot.setTimestamp(timestamp);
                            snapshot.setBalance(entry.getBalance());
                            snapshot.setCharacterId(entry.getCharacterId());

                            snapshots.emplace_back(std::move(snapshot));
                            usedSnapshots << timestamp;
                        }
                    }

                    asyncBatchStore(*mWalletJournalEntryRepository, data, true);
                    asyncBatchStore(*mWalletSnapshotRepository, snapshots, false);

                    saveUpdateTimer(Evernus::TimerType::WalletJournal, mWalletJournalUtcUpdateTimes, id);

                    emit walletJournalChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshWalletTransactions(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing wallet transactions: " << id;

        const auto task = startTask(tr("Fetching wallet transactions for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::WalletTransactions, task))
            return;

        try
        {
            const auto key = getCharacterKey(id);
            const auto maxId = mWalletTransactionRepository->getLatestEntryId(id);

            if (maxId == WalletTransaction::invalidId)
                emit taskInfoChanged(task, tr("Fetching wallet transactions for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletTransactions(*key, id, WalletTransaction::invalidId, maxId,
                                                [task, id, this](const auto &data, const auto &error) {
                if (error.isEmpty())
                {
                    asyncBatchStore(*mWalletTransactionRepository, data, true);
                    saveUpdateTimer(Evernus::TimerType::WalletTransactions, mWalletTransactionsUtcUpdateTimes, id);

                    QSettings settings;
                    if (settings.value(Evernus::PriceSettings::autoAddCustomItemCostKey, Evernus::PriceSettings::autoAddCustomItemCostDefault).toBool() &&
                        !mPendingAutoCostOrders.empty())
                    {
                        computeAutoCosts(id,
                                         mCharacterOrderProvider->getBuyOrders(id),
                                         std::bind(&Evernus::WalletTransactionRepository::fetchForCharacterInRange,
                                                   mWalletTransactionRepository.get(),
                                                   id,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   Evernus::WalletTransactionRepository::EntryType::Buy,
                                                   std::placeholders::_3));
                    }

                    emit walletTransactionsChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshMarketOrdersFromAPI(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing market orders from API: " << id;

        const auto task = startTask(tr("Fetching market orders for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::MarketOrders, task))
            return;

        try
        {
            const auto key = getCharacterKey(id);
            doRefreshMarketOrdersFromAPI<&EvernusApplication::marketOrdersChanged>(*key, id, task);
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshMarketOrdersFromLogs(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing market orders from logs: " << id;

        const auto task = startTask(tr("Fetching market orders for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        importMarketOrdersFromLogs(id, task, false);

        emit taskEnded(task, QString{});
    }

    void EvernusApplication::refreshCorpContracts(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing corp contracts: " << id;

        const auto task = startTask(tr("Fetching corporation contracts for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::CorpContracts, task))
            return;

        try
        {
            const auto key = getCorpKey(id);
            mAPIManager.fetchContracts(*key, id, [key, task, id, this](auto &&data, const auto &error) {
                if (error.isEmpty())
                {
                    asyncBatchStore(*mCorpContractRepository, data, true);

                    try
                    {
                        const auto corpId = mCharacterRepository->getCorporationId(id);

                        mCharacterContractProvider->clearForCorporation(corpId);
                        mCorpContractProvider->clearForCorporation(corpId);
                        mCharacterContractProvider->clearForCorporation(corpId);
                        mCorpContractProvider->clearForCorporation(corpId);
                    }
                    catch (const Evernus::CharacterRepository::NotFoundException &)
                    {
                    }

                    saveUpdateTimer(Evernus::TimerType::CorpContracts, mCorpContractsUtcUpdateTimes, id);

                    this->handleIncomingContracts<&Evernus::EvernusApplication::corpContractsChanged>(key,
                                                                                                      data,
                                                                                                      id,
                                                                                                      *mCorpContractItemRepository,
                                                                                                      task);
                }
                else
                {
                    emit taskEnded(task, error);
                }
            });
        }
        catch (const CorpKeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
    }

    void EvernusApplication::refreshCorpWalletJournal(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing corp wallet journal: " << id;

        const auto task = startTask(tr("Fetching corporation wallet journal for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::CorpWalletJournal, task))
            return;

        try
        {
            const auto key = getCorpKey(id);
            const auto maxId = mCorpWalletJournalEntryRepository->getLatestEntryId(id);

            if (maxId == WalletJournalEntry::invalidId)
                emit taskInfoChanged(task, tr("Fetching corporation wallet journal for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletJournal(*key, id, mCharacterRepository->getCorporationId(id), WalletJournalEntry::invalidId, maxId,
                                           [task, id, this](auto &&data, const auto &error) {
                if (error.isEmpty())
                {
                    std::vector<Evernus::CorpWalletSnapshot> snapshots;
                    snapshots.reserve(data.size());

                    QSet<QDateTime> usedSnapshots;

                    for (auto &entry : data)
                    {
                        const auto timestamp = entry.getTimestamp();

                        if (!usedSnapshots.contains(timestamp))
                        {
                            Evernus::CorpWalletSnapshot snapshot;
                            snapshot.setTimestamp(timestamp);
                            snapshot.setBalance(entry.getBalance());
                            snapshot.setCorporationId(entry.getCorporationId());

                            snapshots.emplace_back(std::move(snapshot));
                            usedSnapshots << timestamp;
                        }
                    }

                    asyncBatchStore(*mCorpWalletJournalEntryRepository, data, true);
                    asyncBatchStore(*mCorpWalletSnapshotRepository, snapshots, false);

                    saveUpdateTimer(Evernus::TimerType::CorpWalletJournal, mCorpWalletJournalUtcUpdateTimes, id);

                    emit corpWalletJournalChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const CorpKeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshCorpWalletTransactions(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing corp wallet transactions: " << id;

        const auto task = startTask(tr("Fetching corporation wallet transactions for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::CorpWalletTransactions, task))
            return;

        try
        {
            const auto key = getCorpKey(id);
            const auto maxId = mCorpWalletTransactionRepository->getLatestEntryId(id);
            const auto corpId = mCharacterRepository->getCorporationId(id);

            if (maxId == WalletTransaction::invalidId)
                emit taskInfoChanged(task, tr("Fetching corporation wallet transactions for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletTransactions(*key, id, corpId, WalletTransaction::invalidId, maxId,
                                                [task, id, corpId, this](auto &&data, const auto &error) {
                if (error.isEmpty())
                {
                    asyncBatchStore(*mCorpWalletTransactionRepository, data, true);
                    saveUpdateTimer(Evernus::TimerType::CorpWalletTransactions, mCorpWalletTransactionsUtcUpdateTimes, id);

                    QSettings settings;
                    if (settings.value(Evernus::PriceSettings::autoAddCustomItemCostKey, Evernus::PriceSettings::autoAddCustomItemCostDefault).toBool() &&
                        !mPendingAutoCostOrders.empty())
                    {
                        computeAutoCosts(id,
                                         mCorpOrderProvider->getBuyOrdersForCorporation(corpId),
                                         std::bind(&Evernus::WalletTransactionRepository::fetchForCorporationInRange,
                                                   mCorpWalletTransactionRepository.get(),
                                                   corpId,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   Evernus::WalletTransactionRepository::EntryType::Buy,
                                                   std::placeholders::_3));
                    }

                    emit corpWalletTransactionsChanged();
                }

                emit taskEnded(task, error);
            });
        }
        catch (const CorpKeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Character not found!"));
        }
    }

    void EvernusApplication::refreshCorpMarketOrdersFromAPI(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing corp market orders from API: " << id;

        const auto task = startTask(tr("Fetching corporation market orders for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::CorpMarketOrders, task))
            return;

        try
        {
            const auto key = getCorpKey(id);
            doRefreshMarketOrdersFromAPI<&EvernusApplication::corpMarketOrdersChanged>(*key, id, task);
        }
        catch (const CorpKeyRepository::NotFoundException &)
        {
            emit taskEnded(task, tr("Key not found!"));
        }
    }

    void EvernusApplication::refreshCorpMarketOrdersFromLogs(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing corp market orders from logs: " << id;

        const auto task = startTask(tr("Fetching corporation market orders for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        importMarketOrdersFromLogs(id, task, true);

        emit taskEnded(task, QString{});
    }

    void EvernusApplication::refreshConquerableStations()
    {
        qDebug() << "Refreshing conquerable stations...";

        const auto task = startTask(tr("Fetching conquerable stations..."));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        mAPIManager.fetchConquerableStationList([task, this](const auto &list, const auto &error) {
            if (error.isEmpty())
            {
                mDataProvider->clearStationCache();

                mConquerableStationRepository->exec(QString{"DELETE FROM %1"}.arg(mConquerableStationRepository->getTableName()));
                asyncBatchStore(*mConquerableStationRepository, list, true);

                emit conquerableStationsChanged();
            }

            emit taskEnded(task, error);
        });
    }

    void EvernusApplication::refreshAllExternalOrders()
    {
        ExternalOrderImporter::TypeLocationPairs target;

        QSqlQuery query{mMainDb};
        query.prepare(QString{"SELECT DISTINCT ids.type_id, ids.location_id FROM ("
            "SELECT type_id, location_id FROM %1 WHERE state = ? "
            "UNION "
            "SELECT type_id, location_id FROM %2 WHERE state = ?"
        ") ids"}.arg(mMarketOrderRepository->getTableName()).arg(mCorpMarketOrderRepository->getTableName()));

        query.addBindValue(static_cast<int>(MarketOrder::State::Active));
        query.addBindValue(static_cast<int>(MarketOrder::State::Active));

        DatabaseUtils::execQuery(query);

        while (query.next())
            target.emplace(std::make_pair(query.value(0).value<EveType::IdType>(), query.value(1).toUInt()));

        QSettings settings;

        const auto source = static_cast<ImportSettings::PriceImportSource>(
            settings.value(ImportSettings::priceImportSourceKey, static_cast<int>(ImportSettings::priceImportSourceDefault)).toInt());
        switch (source) {
        case ImportSettings::PriceImportSource::Logs:
            refreshExternalOrdersFromFile(target);
            break;
        case ImportSettings::PriceImportSource::Cache:
            refreshExternalOrdersFromCache(target);
            break;
        default:
            refreshExternalOrdersFromWeb(target);
        }
    }

    void EvernusApplication::refreshExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::webImporter, target);
    }

    void EvernusApplication::refreshExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::logImporter, target);
    }

    void EvernusApplication::refreshExternalOrdersFromCache(const ExternalOrderImporter::TypeLocationPairs &target)
    {
        QSettings settings;
        if (!settings.value(UISettings::cacheImportApprovedKey, UISettings::cacheImportApprovedDefault).toBool())
        {
            if (QMessageBox::warning(activeWindow(), tr("Cache import"), tr(
                "Warning! Reading cache is considered a gray area. CPP on one hand considers this a violation of the EULA, "
                "but on the other has stated they will only penalize when used in conjunction with illegal activities, like botting.\n\n"
                "Do wish to continue?\n\n"
                "By choosing 'Yes' you accept all responsibility of any action CCP may impose upon you, should they choose to change"
                "their policy."
            ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
            {
                return;
            }

            settings.setValue(UISettings::cacheImportApprovedKey, true);
        }

        importExternalOrders(ExternalOrderImporterNames::cacheImporter, target);
    }

    void EvernusApplication::refreshExternalOrdersFromCREST(const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::crestImporter, target);
    }

    void EvernusApplication::updateExternalOrdersAndAssetValue(const std::vector<ExternalOrder> &orders)
    {
        try
        {
            asyncExecute(&CachingEveDataProvider::updateExternalOrders, mDataProvider.get(), std::cref(orders));

            QSettings settings;
            if (settings.value(ImportSettings::autoUpdateAssetValueKey, ImportSettings::autoUpdateAssetValueDefault).toBool())
            {
                for (const auto &list : mCharacterAssets)
                    computeAssetListSellValue(*list.second);
            }

            finishExternalOrderImportTask(QString{});
        }
        catch (const std::exception &e)
        {
            finishExternalOrderImportTask(e.what());
        }

        emit externalOrdersChanged();
    }

    void EvernusApplication::handleNewPreferences()
    {
        QSettings settings;
        updateTranslator(settings.value(UISettings::languageKey).toString());

        mIGBSessionManager.shutdown();
        mIGBSessionManager.setPort(settings.value(IGBSettings::portKey, IGBSettings::portDefault).value<quint16>());

        if (settings.value(IGBSettings::enabledKey, IGBSettings::enabledDefault).toBool())
            mIGBSessionManager.start();

        mHttpSessionManager.shutdown();
        mHttpSessionManager.setPort(settings.value(HttpSettings::portKey, HttpSettings::portDefault).value<quint16>());

        if (settings.value(HttpSettings::enabledKey, HttpSettings::enabledDefault).toBool())
            mHttpSessionManager.start();

        mCharacterItemCostCache.clear();
        mDataProvider->handleNewPreferences();

        mMarketUploader->setEnabled(settings.value(UploaderSettings::enabledKey, UploaderSettings::enabledDefault).toBool());

        setSmtpSettings();

        emit itemCostsChanged();
        emit itemVolumeChanged();
    }

    void EvernusApplication::importFromMentat()
    {
        const auto path = QFileDialog::getExistingDirectory(activeWindow(), tr("Select Mentat directory"));
        if (path.isEmpty())
            return;

        auto db = QSqlDatabase::addDatabase("QSQLITE", "mentat");
        db.setDatabaseName(path + "/Storage/EVEMentat.dynamic.database");
        db.setConnectOptions("QSQLITE_OPEN_READONLY");

        if (!db.open())
        {
            QMessageBox::warning(activeWindow(), tr("Error"), tr("Error opening %1").arg(path + "/Storage/EVEMentat.dynamic.database"));
            return;
        }

        const auto task = startTask(tr("Importing order history..."));
        const auto charIds = mCharacterRepository->fetchAllIds();

        auto query = db.exec(
            "SELECT * FROM mentatOrders o INNER JOIN mentatOrdersHistory h ON h.orderID = o.orderID WHERE o.orderState != 0 AND o.isCorp = 0");

        std::vector<MarketOrder> orders;
        if (query.size() > 0)
            orders.reserve(query.size());

        while (query.next())
        {
            const auto charId = query.value("charID").value<Character::IdType>();
            if (charIds.find(charId) == std::end(charIds))
                continue;

            auto issued = query.value("issued").toDateTime();
            issued.setTimeSpec(Qt::UTC);

            auto lastSeen = query.value("referenceTime").toDateTime();
            lastSeen.setTimeSpec(Qt::UTC);

            auto intState = query.value("orderState").toInt();
            if (intState < static_cast<int>(MarketOrder::State::Active) || intState > static_cast<int>(MarketOrder::State::CharacterDeleted))
                intState = static_cast<int>(MarketOrder::State::Fulfilled);

            MarketOrder order{query.value("orderID").value<MarketOrder::IdType>()};
            order.setAccountKey(query.value("accountID").value<short>());
            order.setCharacterId(charId);
            order.setDuration(query.value("duration").value<short>());
            order.setEscrow(query.value("escrow").toDouble() / 100.);
            order.setFirstSeen(issued);
            order.setIssued(issued);
            order.setLastSeen(lastSeen);
            order.setStationId(query.value("stationID").toULongLong());
            order.setMinVolume(query.value("minVolume").toUInt());
            order.setPrice(query.value("price").toDouble() / 100.);
            order.setRange(query.value("range").value<short>());
            order.setState(static_cast<MarketOrder::State>(intState));
            order.setType((query.value("bid").toBool()) ? (MarketOrder::Type::Buy) : (MarketOrder::Type::Sell));
            order.setTypeId(query.value("typeID").value<EveType::IdType>());
            order.setVolumeEntered(query.value("volEntered").toUInt());
            order.setVolumeRemaining(query.value("volRemaining").toUInt());

            orders.emplace_back(std::move(order));

            emit taskInfoChanged(task, tr("Importing order history: %1 processed").arg(orders.size()));

            processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        emit taskInfoChanged(task, tr("Importing order history: storing %1 orders (this may take a while)").arg(orders.size()));

        try
        {
            asyncBatchStore(*mMarketOrderRepository, orders, true);
            emit taskEnded(task, QString{});
        }
        catch (const std::exception &e)
        {
            emit taskEnded(task, e.what());
        }

        mCharacterOrderProvider->clearArchived();

        emit marketOrdersChanged();
    }

    void EvernusApplication::syncLMeve(Character::IdType id)
    {
        qDebug() << "Performing LMeve sync:" << id;

        const auto task = startTask(tr("Synchronizing with LMeve..."));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (getLocalCacheTimer(id, TimerType::LMeveTasks) > QDateTime::currentDateTime())
        {
            emit taskEnded(task, QString{});
            return;
        }

        mLMeveAPIManager.fetchTasks(id, [id, task, this](auto &&list, const auto &error) {
            if (error.isEmpty())
            {
                mLMeveTaskCache.erase(id);
                mLMeveTaskRepository->removeForCharacter(id);

                asyncBatchStore(*mLMeveTaskRepository, list, true);
                setUtcCacheTimer(id, Evernus::TimerType::LMeveTasks, QDateTime::currentDateTimeUtc().addSecs(3600));

                emit lMeveTasksChanged();
            }

            emit taskEnded(task, error);
        });
    }

    void EvernusApplication::scheduleCharacterUpdate()
    {
        if (mCharacterUpdateScheduled)
            return;

        mCharacterUpdateScheduled = true;
        QMetaObject::invokeMethod(this, "updateCharacters", Qt::QueuedConnection);
    }

    void EvernusApplication::updateCharacters()
    {
        mCharacterUpdateScheduled = false;

        emit charactersChanged();
    }

    void EvernusApplication::scheduleMarketOrderChange()
    {
        if (mMarketOrderUpdateScheduled)
            return;

        mMarketOrderUpdateScheduled = true;
        QMetaObject::invokeMethod(this, "updateMarketOrders", Qt::QueuedConnection);
    }

    void EvernusApplication::updateMarketOrders()
    {
        mMarketOrderUpdateScheduled = false;

        emit marketOrdersChanged();
    }

    void EvernusApplication::scheduleCorpMarketOrderChange()
    {
        if (mCorpMarketOrderUpdateScheduled)
            return;

        mCorpMarketOrderUpdateScheduled = true;
        QMetaObject::invokeMethod(this, "updateCorpMarketOrders", Qt::QueuedConnection);
    }

    void EvernusApplication::updateCorpMarketOrders()
    {
        mCorpMarketOrderUpdateScheduled = false;

        emit corpMarketOrdersChanged();
    }

    void EvernusApplication::showPriceImportError(const QString &info)
    {
        Q_ASSERT(mCurrentExternalOrderImportTask != TaskConstants::invalidTask);
        finishExternalOrderImportTask(info);
    }

    void EvernusApplication::emitNewItemCosts()
    {
        mItemCostUpdateScheduled = false;
        emit itemCostsChanged();
    }

    void EvernusApplication::showSmtpError(const QByteArray &message)
    {
        QMessageBox::warning(activeWindow(), tr("SMTP Error"), tr("Error sending email: %1").arg(QString{message}));
    }

    void EvernusApplication::showMailError(int mailID, int errorCode, const QByteArray &message)
    {
        Q_UNUSED(mailID);
        Q_UNUSED(errorCode);

        QMessageBox::warning(activeWindow(), tr("Mail Error"), tr("Error sending email: %1").arg(QString{message}));
    }

    void EvernusApplication::updateTranslator(const QString &lang)
    {
        qDebug() << "Switching language to" << lang;

        const QLocale locale{lang};
        QLocale::setDefault(locale);

        removeTranslator(&mTranslator);
        removeTranslator(&mQtTranslator);

        mTranslator.load(locale, "lang", "_", applicationDirPath() + UISettings::translationPath);
        mQtTranslator.load(locale, "qt", "_", applicationDirPath() + UISettings::translationPath);

        installTranslator(&mQtTranslator);
        installTranslator(&mTranslator);
    }

    void EvernusApplication::createDb()
    {
        DatabaseUtils::createDb(mMainDb, "main.db");

        if (!mEveDb.isValid())
            throw std::runtime_error{"Error crating Eve DB object!"};

        auto eveDbPath = applicationDirPath() + "/res/eve.db";
        if (!QFile::exists(eveDbPath))
        {
            eveDbPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, applicationName() + "/res/eve.db");
            if (!QFile::exists(eveDbPath))
                throw std::runtime_error{"Cannot find Eve DB!"};
        }

        qDebug() << "Eve DB path:" << eveDbPath;

        mEveDb.setDatabaseName(eveDbPath);
        mEveDb.setConnectOptions("QSQLITE_OPEN_READONLY");

        if (!mEveDb.open())
            throw std::runtime_error{"Error opening Eve DB!"};

        mKeyRepository.reset(new KeyRepository{mMainDb});
        mCorpKeyRepository.reset(new CorpKeyRepository{mMainDb});
        mCharacterRepository.reset(new CharacterRepository{mMainDb});
        mItemRepository.reset(new ItemRepository{mMainDb});
        mAssetListRepository.reset(new AssetListRepository{mMainDb, *mItemRepository});
        mConquerableStationRepository.reset(new ConquerableStationRepository{mMainDb});
        mWalletSnapshotRepository.reset(new WalletSnapshotRepository{mMainDb});
        mCorpWalletSnapshotRepository.reset(new CorpWalletSnapshotRepository{mMainDb});
        mExternalOrderRepository.reset(new ExternalOrderRepository{mMainDb});
        mAssetValueSnapshotRepository.reset(new AssetValueSnapshotRepository{mMainDb});
        mWalletJournalEntryRepository.reset(new WalletJournalEntryRepository{false, mMainDb});
        mCorpWalletJournalEntryRepository.reset(new WalletJournalEntryRepository{true, mMainDb});
        mRefTypeRepository.reset(new RefTypeRepository{mMainDb});
        mCacheTimerRepository.reset(new CacheTimerRepository{mMainDb});
        mUpdateTimerRepository.reset(new UpdateTimerRepository{mMainDb});
        mWalletTransactionRepository.reset(new WalletTransactionRepository{false, mMainDb});
        mCorpWalletTransactionRepository.reset(new WalletTransactionRepository{true, mMainDb});
        mMarketOrderRepository.reset(new MarketOrderRepository{false, mMainDb});
        mCorpMarketOrderRepository.reset(new MarketOrderRepository{true, mMainDb});
        mItemCostRepository.reset(new ItemCostRepository{mMainDb});
        mMarketOrderValueSnapshotRepository.reset(new MarketOrderValueSnapshotRepository{mMainDb});
        mCorpMarketOrderValueSnapshotRepository.reset(new CorpMarketOrderValueSnapshotRepository{mMainDb});
        mFilterTextRepository.reset(new FilterTextRepository{mMainDb});
        mOrderScriptRepository.reset(new OrderScriptRepository{mMainDb});
        mFavoriteItemRepository.reset(new FavoriteItemRepository{mMainDb});
        mLocationBookmarkRepository.reset(new LocationBookmarkRepository{mMainDb});
        mContractItemRepository.reset(new ContractItemRepository{false, mMainDb});
        mCorpContractItemRepository.reset(new ContractItemRepository{true, mMainDb});
        mContractRepository.reset(new ContractRepository{*mContractItemRepository, false, mMainDb});
        mCorpContractRepository.reset(new ContractRepository{*mCorpContractItemRepository, true, mMainDb});
        mLMeveTaskRepository.reset(new LMeveTaskRepository{mMainDb});
        mEveTypeRepository.reset(new EveTypeRepository{mEveDb});
        mMarketGroupRepository.reset(new MarketGroupRepository{mEveDb});
        mMetaGroupRepository.reset(new MetaGroupRepository{mEveDb});
    }

    void EvernusApplication::createDbSchema()
    {
        mKeyRepository->create();
        mCharacterRepository->create(*mKeyRepository);
        mCorpKeyRepository->create(*mCharacterRepository);
        mAssetListRepository->create(*mCharacterRepository);
        mItemRepository->create(*mAssetListRepository);
        mConquerableStationRepository->create();
        mWalletSnapshotRepository->create(*mCharacterRepository);
        mCorpWalletSnapshotRepository->create();
        mAssetValueSnapshotRepository->create(*mCharacterRepository);
        mWalletJournalEntryRepository->create(*mCharacterRepository);
        mCorpWalletJournalEntryRepository->create(*mCharacterRepository);
        mExternalOrderRepository->create();
        mCacheTimerRepository->create(*mCharacterRepository);
        mUpdateTimerRepository->create(*mCharacterRepository);
        mWalletTransactionRepository->create(*mCharacterRepository);
        mCorpWalletTransactionRepository->create(*mCharacterRepository);
        mMarketOrderRepository->create(*mCharacterRepository);
        mCorpMarketOrderRepository->create(*mCharacterRepository);
        mItemCostRepository->create(*mCharacterRepository);
        mMarketOrderValueSnapshotRepository->create(*mCharacterRepository);
        mCorpMarketOrderValueSnapshotRepository->create();
        mFilterTextRepository->create();
        mOrderScriptRepository->create();
        mFavoriteItemRepository->create();
        mLocationBookmarkRepository->create();
        mContractRepository->create();
        mCorpContractRepository->create();
        mContractItemRepository->create(*mContractRepository);
        mCorpContractItemRepository->create(*mCorpContractRepository);
        mLMeveTaskRepository->create(*mCharacterRepository);
        mRefTypeRepository->create();
    }

    void EvernusApplication::precacheCacheTimers()
    {
        const auto timers = mCacheTimerRepository->fetchAll();
        for (const auto &timer : timers)
        {
            switch (timer->getType()) {
            case TimerType::Character:
                mCharacterUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::AssetList:
                mAssetsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::WalletJournal:
                mWalletJournalUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::WalletTransactions:
                mWalletTransactionsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::Contracts:
                mContractsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::MarketOrders:
                mMarketOrdersUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::CorpWalletJournal:
                mCorpWalletJournalUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::CorpWalletTransactions:
                mCorpWalletTransactionsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::CorpMarketOrders:
                mCorpMarketOrdersUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::CorpContracts:
                mCorpContractsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            case TimerType::LMeveTasks:
                mLMeveUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
                break;
            }
        }
    }

    void EvernusApplication::precacheUpdateTimers()
    {
        const auto timers = mUpdateTimerRepository->fetchAll();
        for (const auto &timer : timers)
        {
            switch (timer->getType()) {
            case TimerType::Character:
                mCharacterUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::AssetList:
                mAssetsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::WalletJournal:
                mWalletJournalUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::WalletTransactions:
                mWalletTransactionsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::MarketOrders:
                mMarketOrdersUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::Contracts:
                mContractsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::CorpWalletJournal:
                mCorpWalletJournalUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::CorpWalletTransactions:
                mCorpWalletTransactionsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::CorpMarketOrders:
                mCorpMarketOrdersUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::CorpContracts:
                mCorpContractsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
                break;
            case TimerType::LMeveTasks:
                break;
            }
        }
    }

    void EvernusApplication::deleteOldWalletEntries()
    {
        QSettings settings;

        if (settings.value(WalletSettings::deleteOldJournalKey, WalletSettings::deleteOldJournalDefault).toBool())
        {
            const auto journalDt = QDateTime::currentDateTimeUtc().addDays(
                -settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());
            mWalletJournalEntryRepository->deleteOldEntires(journalDt);
        }

        if (settings.value(WalletSettings::deleteOldTransactionsKey, WalletSettings::deleteOldTransactionsDefault).toBool())
        {
            const auto transactionDt = QDateTime::currentDateTimeUtc().addDays(
                -settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());
            mWalletTransactionRepository->deleteOldEntires(transactionDt);
        }
    }

    uint EvernusApplication::startTask(const QString &description)
    {
        QMetaObject::invokeMethod(this, "taskStarted", Qt::QueuedConnection, Q_ARG(uint, mTaskId), Q_ARG(QString, description));
        return mTaskId++;
    }

    uint EvernusApplication::startTask(uint parentTask, const QString &description)
    {
        if (parentTask == TaskConstants::invalidTask)
            return startTask(description);

        QMetaObject::invokeMethod(this, "taskStarted", Qt::QueuedConnection, Q_ARG(uint, mTaskId), Q_ARG(uint, parentTask), Q_ARG(QString, description));
        return mTaskId++;
    }

    void EvernusApplication::importCharacter(Character::IdType id, uint parentTask, const Key &key)
    {
        const auto charSubtask = startTask(parentTask, tr("Fetching character %1...").arg(id));
        if (!checkImportAndEndTask(id, TimerType::Character, charSubtask))
            return;

        mAPIManager.fetchCharacter(key, id, [charSubtask, id, this](auto &&data, const auto &error) {
            if (error.isEmpty())
            {
                try
                {
                    const auto prevData = mCharacterRepository->find(data.getId());

                    QSettings settings;
                    if (!settings.value(Evernus::ImportSettings::importSkillsKey, Evernus::ImportSettings::importSkillsDefault).toBool())
                    {
                        data.setOrderAmountSkills(prevData->getOrderAmountSkills());
                        data.setTradeRangeSkills(prevData->getTradeRangeSkills());
                        data.setFeeSkills(prevData->getFeeSkills());
                        data.setContractSkills(prevData->getContractSkills());
                    }

                    data.setCorpStanding(prevData->getCorpStanding());
                    data.setFactionStanding(prevData->getFactionStanding());
                    data.setEnabled(prevData->isEnabled());
                }
                catch (const Evernus::CharacterRepository::NotFoundException &)
                {
                }

                mMainDb.exec("PRAGMA foreign_keys = OFF;");
                mCharacterRepository->store(data);
                mMainDb.exec("PRAGMA foreign_keys = ON;");

                const auto cacheTimer = mCharacterUtcCacheTimes[id];
                if (cacheTimer.isValid())
                {
                    Evernus::CacheTimer timer;
                    timer.setCharacterId(id);
                    timer.setType(Evernus::TimerType::Character);
                    timer.setCacheUntil(cacheTimer);

                    mCacheTimerRepository->store(timer);
                }

                Evernus::WalletSnapshot snapshot;
                snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
                snapshot.setBalance(data.getISK());
                snapshot.setCharacterId(data.getId());
                mWalletSnapshotRepository->store(snapshot);

                saveUpdateTimer(Evernus::TimerType::Character, mCharacterUtcUpdateTimes, id);

                QMetaObject::invokeMethod(this, "scheduleCharacterUpdate", Qt::QueuedConnection);
            }

            emit taskEnded(charSubtask, error);
        });
    }

    void EvernusApplication::importExternalOrders(const std::string &importerName, const ExternalOrderImporter::TypeLocationPairs &target)
    {
        if (mCurrentExternalOrderImportTask != TaskConstants::invalidTask)
            return;

        qDebug() << "Refreshing item prices using importer:" << importerName.c_str();

        const auto it = mExternalOrderImporters.find(importerName);
        Q_ASSERT(it != std::end(mExternalOrderImporters));

        mCurrentExternalOrderImportTask = startTask(tr("Importing item prices..."));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        it->second->fetchExternalOrders(target);
    }

    void EvernusApplication::importMarketOrdersFromLogs(Character::IdType id, uint task, bool corp)
    {
        const auto logPath = PathUtils::getMarketLogsPath();
        if (logPath.isEmpty())
        {
            emit taskEnded(task, tr("Cannot determine market logs path!"));
            return;
        }

        QSettings settings;

        const auto wildcard = (corp) ?
                              (settings.value(PathSettings::corporationLogWildcardKey, PathSettings::corporationLogWildcardDefault).toString()) :
                              (settings.value(PathSettings::characterLogWildcardKey, PathSettings::characterLogWildcardDefault).toString());

        const QDir logDir{logPath};
        const auto logs = logDir.entryList(QStringList{wildcard}, QDir::Files | QDir::Readable, QDir::Time);
        if (logs.isEmpty())
        {
            emit taskEnded(task, tr("No market logs found!"));
            return;
        }

        for (const auto &log : logs)
        {
            qDebug() << "Parsing" << log;

            QFile file{logDir.filePath(log)};
            if (!file.open(QIODevice::ReadOnly))
            {
                emit taskEnded(task, tr("Could not open market log file!"));
                return;
            }

            file.readLine();

            auto characterFound = false;

            MarketOrders orders;
            while (!file.atEnd())
            {
                const QString line = file.readLine();
                const auto values = line.split(',');

                if (values.size() >= 22)
                {
                    const auto idColumn = 0;
                    const auto typeIdColumn = 1;
                    const auto characterIdColumn = 2;
                    const auto stationIdColumn = 6;
                    const auto rangeColumn = 8;
                    const auto typeColumn = 9;
                    const auto priceColumn = 10;
                    const auto volumeEnteredColumn = 11;
                    const auto volumeRemainingColumn = 12;
                    const auto issuedColumn = 13;
                    const auto stateColumn = 14;
                    const auto minVolumeColumn = 15;
                    const auto durationColumn = 17;
                    const auto isCorpColumn = 18;
                    const auto escrowColumn = 21;

                    const auto characterId = values[characterIdColumn].toULongLong();
                    if (!characterFound && !corp)
                    {
                        if (characterId != id)
                        {
                            file.close();
                            continue;
                        }

                        characterFound = true;
                    }

                    if ((corp && values[isCorpColumn] != "True") || (!corp && values[isCorpColumn] != "False"))
                        continue;

                    MarketOrder order{values[idColumn].toULongLong()};
                    order.setCharacterId(characterId);
                    order.setStationId(values[stationIdColumn].toULongLong());
                    order.setVolumeEntered(values[volumeEnteredColumn].toUInt());
                    order.setVolumeRemaining(static_cast<uint>(values[volumeRemainingColumn].toDouble()));
                    order.setMinVolume(values[minVolumeColumn].toUInt());
                    order.setDelta(order.getVolumeRemaining() - order.getVolumeEntered());
                    order.setState(static_cast<MarketOrder::State>(values[stateColumn].toInt()));
                    order.setTypeId(values[typeIdColumn].toUInt());
                    order.setRange(values[rangeColumn].toShort());
                    order.setDuration(values[durationColumn].toShort());
                    order.setEscrow(values[escrowColumn].toDouble());
                    order.setPrice(values[priceColumn].toDouble());
                    order.setType((values[typeColumn] == "True") ? (MarketOrder::Type::Buy) : (MarketOrder::Type::Sell));

                    auto issued = QDateTime::fromString(values[issuedColumn], "yyyy-MM-dd HH:mm:ss.zzz");
                    issued.setTimeSpec(Qt::UTC);

                    order.setIssued(issued);
                    order.setFirstSeen(issued);

                    orders.emplace_back(std::move(order));
                }
            }

            if (characterFound || corp)
            {
                if (settings.value(PathSettings::deleteLogsKey, PathSettings::deleteLogsDefault).toBool())
                    file.remove();

                importMarketOrders(id, orders, corp);

                if (corp)
                    emit corpMarketOrdersChanged();
                else
                    emit marketOrdersChanged();

                emit externalOrdersChangedWithMarketOrders();

                break;
            }
        }
    }

    void EvernusApplication::importMarketOrders(Character::IdType id, MarketOrders &orders, bool corp)
    {
        try
        {
            const auto corpId = mCharacterRepository->getCorporationId(id);

            const auto &orderRepo = (corp) ? (*mCorpMarketOrderRepository) : (*mMarketOrderRepository);
            auto curStates = orderRepo.getOrderStates(id);

            if (corp)
            {
                mCorpOrderProvider->clearOrdersForCharacter(id);
                mCorpOrderProvider->clearOrdersForCorporation(corpId);
            }
            else
            {
                mCharacterOrderProvider->clearOrdersForCharacter(id);
                mCharacterOrderProvider->clearOrdersForCorporation(corpId);
            }

            mCombinedOrderProvider->clearOrdersForCharacter(id);
            mPendingAutoCostOrders.clear();

            QSettings settings;
            const auto autoSetCosts = settings.value(PriceSettings::autoAddCustomItemCostKey, PriceSettings::autoAddCustomItemCostDefault).toBool();
            const auto makeCorpSnapshot = settings.value(ImportSettings::makeCorpSnapshotsKey, ImportSettings::makeCorpSnapshotsDefault).toBool();
            const auto emailNotification = settings.value(ImportSettings::autoImportEnabledKey, ImportSettings::autoImportEnabledDefault).toBool() &&
                                           settings.value(ImportSettings::emailNotificationsEnabledKey, ImportSettings::emailNotificationsEnabledDefault).toBool();

            struct EmailOrderInfo
            {
                EveType::IdType mTypeId;
                uint mVolumeEntered;
                MarketOrder::State mState;
            };

            std::vector<EmailOrderInfo> emailOrders;

            const auto curDt = QDateTime::currentDateTimeUtc();

            for (auto &order : orders)
            {
                const auto cIt = curStates.find(order.getId());
                if (cIt != std::end(curStates))
                {
                    order.setDelta(order.getVolumeRemaining() - cIt->second.mVolumeRemaining);
                    order.setFirstSeen(cIt->second.mFirstSeen);

                    if (order.getState() != MarketOrder::State::Active)
                    {
                        if (!cIt->second.mLastSeen.isNull())
                        {
                            order.setLastSeen(cIt->second.mLastSeen);
                        }
                        else
                        {
                            order.setLastSeen(std::min(curDt, order.getIssued().addDays(order.getDuration())));

                            if (emailNotification)
                            {
                                EmailOrderInfo info;
                                info.mTypeId = order.getTypeId();
                                info.mVolumeEntered = order.getVolumeEntered();
                                info.mState = order.getState();

                                emailOrders.emplace_back(std::move(info));
                            }
                        }
                    }

                    curStates.erase(cIt);
                }
                else
                {
                    order.setDelta(order.getVolumeRemaining() - order.getVolumeEntered());
                }

                order.setCorporationId(corpId); // TODO: move up after 0.5

                if (autoSetCosts && order.getType() == MarketOrder::Type::Buy && order.getDelta() != 0 && order.getState() == MarketOrder::State::Fulfilled)
                    mPendingAutoCostOrders.emplace(order.getId());
            }

            std::vector<MarketOrder::IdType> toArchive, toFulfill;
            toArchive.reserve(curStates.size());
            toFulfill.reserve(curStates.size());

            for (const auto &order : curStates)
            {
                if (order.second.mLastSeen.isNull() || order.second.mDelta != 0)
                {
                    if (order.second.mExpiry < curDt)
                        toArchive.emplace_back(order.first);
                    else
                        toFulfill.emplace_back(order.first);
                }
            }

            if (!toArchive.empty())
                orderRepo.archive(toArchive);
            if (!toFulfill.empty())
                orderRepo.fulfill(toFulfill);

            asyncBatchStore(orderRepo, orders, true);

            if (!corp)
            {
                MarketOrderValueSnapshot snapshot;
                snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
                snapshot.setCharacterId(id);

                double buy = 0., sell = 0.;
                for (const auto &order : orders)
                {
                    if (order.getState() != MarketOrder::State::Active)
                        continue;

                    if (order.getType() == MarketOrder::Type::Buy)
                        buy += order.getEscrow();
                    else
                        sell += order.getPrice() * order.getVolumeRemaining();
                }

                snapshot.setBuyValue(buy);
                snapshot.setSellValue(sell);

                mMarketOrderValueSnapshotRepository->store(snapshot);
            }
            else if (makeCorpSnapshot)
            {
                CorpMarketOrderValueSnapshot snapshot;
                snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
                snapshot.setCorporationId(corpId);

                double buy = 0., sell = 0.;
                for (const auto &order : orders)
                {
                    if (order.getState() != MarketOrder::State::Active)
                        continue;

                    if (order.getType() == MarketOrder::Type::Buy)
                        buy += order.getEscrow();
                    else
                        sell += order.getPrice() * order.getVolumeRemaining();
                }

                snapshot.setBuyValue(buy);
                snapshot.setSellValue(sell);

                mCorpMarketOrderValueSnapshotRepository->store(snapshot);
            }

            mDataProvider->clearExternalOrderCaches();

            if (corp)
                saveUpdateTimer(TimerType::CorpMarketOrders, mCorpMarketOrdersUtcUpdateTimes, id);
            else
                saveUpdateTimer(TimerType::MarketOrders, mMarketOrdersUtcUpdateTimes, id);

            if (emailNotification && !emailOrders.empty())
            {
                try
                {
                    if (static_cast<ImportSettings::SmtpConnectionSecurity>(settings.value(
                        ImportSettings::smtpConnectionSecurityKey).toInt()) == ImportSettings::SmtpConnectionSecurity::TLS)
                    {
                        mSmtp.connectToSecureHost(settings.value(ImportSettings::smtpHostKey, ImportSettings::smtpHostDefault).toString());
                    }
                    else
                    {
                        mSmtp.connectToHost(settings.value(ImportSettings::smtpHostKey, ImportSettings::smtpHostDefault).toString());
                    }

                    QxtMailMessage message{tr("Evernus"), settings.value(ImportSettings::emailNotificationAddressKey).toString()};
                    message.setSubject(tr("[Evernus] Market orders fulfilled"));

                    QLocale locale;

                    QString body{tr("The following orders have changed their status:\n\n")};
                    for (const auto &order : emailOrders)
                    {
                        body.append(tr("    %1 x%2 [%3]\n")
                            .arg(mDataProvider->getTypeName(order.mTypeId))
                            .arg(locale.toString(order.mVolumeEntered))
                            .arg(MarketOrder::stateToString(order.mState)));
                    }

                    message.setBody(body);

                    mSmtp.send(message);
                }
                catch (...)
                {
                    mSmtp.disconnectFromHost();
                    throw;
                }
            }

            if (autoSetCosts)
            {
                if (corp)
                    refreshCorpWalletTransactions(id);
                else
                    refreshWalletTransactions(id);
            }

            if (settings.value(PriceSettings::refreshPricesWithOrdersKey).toBool())
                refreshAllExternalOrders();
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            QMessageBox::warning(activeWindow(), tr("Evernus"), tr("Couldn't find character for order import!"));
        }
    }

    KeyRepository::EntityPtr EvernusApplication::getCharacterKey(Character::IdType id) const
    {
        try
        {
            const auto character = mCharacterRepository->find(id);
            const auto keyId = character->getKeyId();

            if (keyId)
            {
                try
                {
                    return mKeyRepository->find(*keyId);
                }
                catch (const KeyRepository::NotFoundException &)
                {
                    qCritical() << "Attempted to refresh character without a key!";
                    throw;
                }
            }
            else
            {
                throw KeyRepository::NotFoundException{};
            }
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            qCritical() << "Attempted to refresh non-existent character!";
            throw;
        }
    }

    CorpKeyRepository::EntityPtr EvernusApplication::getCorpKey(Character::IdType id) const
    {
        return mCorpKeyRepository->fetchForCharacter(id);
    }

    void EvernusApplication::finishExternalOrderImportTask(const QString &info)
    {
        const auto task = mCurrentExternalOrderImportTask;
        mCurrentExternalOrderImportTask = TaskConstants::invalidTask;

        emit taskEnded(task, info);
    }

    void EvernusApplication::computeAssetListSellValue(const AssetList &list) const
    {
        try
        {
            auto value = 0.;
            for (const auto &item : list)
            {
                const auto locationId = item->getLocationId();
                if (!locationId)
                    continue;

                value += getTotalItemSellValue(*item, *locationId);
            }

            AssetValueSnapshot snapshot;
            snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
            snapshot.setBalance(value);
            snapshot.setCharacterId(list.getCharacterId());

            mAssetValueSnapshotRepository->store(snapshot);
        }
        catch (const ExternalOrderRepository::NotFoundException &)
        {
        }
    }

    double EvernusApplication::getTotalItemSellValue(const Item &item, quint64 locationId) const
    {
        QSettings settings;
        const auto throwOnUnavailable
            = settings.value(ImportSettings::updateOnlyFullAssetValueKey, ImportSettings::updateOnlyFullAssetValueDefault).toBool();

        auto price
            = mDataProvider->getTypeSellPrice(item.getTypeId(), locationId, !throwOnUnavailable)->getPrice() * item.getQuantity();
        for (const auto &child : item)
            price += getTotalItemSellValue(*child, locationId);

        return price;
    }

    void EvernusApplication::saveUpdateTimer(TimerType timer, CharacterTimerMap &map, Character::IdType characterId) const
    {
        const auto time = QDateTime::currentDateTimeUtc();

        map[characterId] = time;

        UpdateTimer storedTimer;
        storedTimer.setCharacterId(characterId);
        storedTimer.setType(timer);
        storedTimer.setUpdateTime(time);

        mUpdateTimerRepository->store(storedTimer);
    }

    void EvernusApplication::computeAutoCosts(Character::IdType characterId,
                                              const MarketOrderProvider::OrderList &orders,
                                              const TransactionFetcher &transFetcher)
    {
        struct ItemCostData
        {
            uint mQuantity;
            double mPrice;
        };

        std::unordered_map<Evernus::EveType::IdType, ItemCostData> newItemCosts;

        for (const auto &order : orders)
        {
            const auto it = mPendingAutoCostOrders.find(order->getId());
            if (it == std::end(mPendingAutoCostOrders))
                continue;

            const auto lastSeen = std::min(QDateTime::currentDateTimeUtc(), order->getIssued().addDays(order->getDuration()));
            const auto transactions = transFetcher(order->getFirstSeen(), lastSeen, order->getTypeId());

            auto &cost = newItemCosts[order->getTypeId()];
            for (const auto &transaction : transactions)
            {
                cost.mQuantity += transaction->getQuantity();
                cost.mPrice += transaction->getQuantity() * transaction->getPrice();
            }

            mPendingAutoCostOrders.erase(it);
        }


        for (const auto &cost : newItemCosts)
        {
            if (cost.second.mQuantity > 0)
                setForCharacterAndType(characterId, cost.first, cost.second.mPrice / cost.second.mQuantity);
        }
    }

    void EvernusApplication::setSmtpSettings()
    {
        SimpleCrypt crypt{ImportSettings::smtpCryptKey};
        QSettings settings;

        mSmtp.setUsername(settings.value(ImportSettings::smtpUserKey).toByteArray());
        mSmtp.setPassword(crypt.decryptToByteArray(settings.value(ImportSettings::smtpPasswordKey).toString()));
        mSmtp.setStartTlsDisabled(static_cast<ImportSettings::SmtpConnectionSecurity>(
            settings.value(ImportSettings::smtpConnectionSecurityKey).toInt()) != ImportSettings::SmtpConnectionSecurity::STARTTLS);
    }

    bool EvernusApplication::shouldImport(Character::IdType id, TimerType type) const
    {
        QSettings settings;
        if (!settings.value(ImportSettings::ignoreCachedImportKey, ImportSettings::ignoreCachedImportDefault).toBool())
            return true;

        return getLocalCacheTimer(id, type) <= QDateTime::currentDateTime();
    }

    bool EvernusApplication::checkImportAndEndTask(Character::IdType id, TimerType type, uint task)
    {
        const auto ret = shouldImport(id, type);
        if (!ret)
            QMetaObject::invokeMethod(this, "taskEnded", Qt::QueuedConnection, Q_ARG(uint, task), Q_ARG(QString, QString{}));

        return ret;
    }

    template<void (EvernusApplication::* Signal)(), class Key>
    void EvernusApplication::handleIncomingContracts(const Key &key,
                                                     const Contracts &data,
                                                     Character::IdType id,
                                                     const ContractItemRepository &itemRepo,
                                                     uint task)
    {
        if (data.empty())
        {
            emit (this->*Signal)();
            emit taskEnded(task, QString{});
        }
        else
        {
            for (const auto &contract : data)
            {
                if (contract.getType() == Evernus::Contract::Type::Courier)
                    continue;

                ++mPendingContractItemRequests;

                const auto subTask = startTask(task, tr("Fetching contract items for contract %1...").arg(contract.getId()));
                mAPIManager.fetchContractItems(*key, id, contract.getId(), [subTask, &itemRepo, this](APIManager::ContractItemList &&data, const QString &error) {
                    --mPendingContractItemRequests;

                    if (error.isEmpty())
                        asyncBatchStore(itemRepo, data, true);

                    if (mPendingContractItemRequests == 0)
                        emit (this->*Signal)();

                    emit taskEnded(subTask, error);
                });
            }

            if (mPendingContractItemRequests == 0)
                emit (this->*Signal)();
        }
    }

    template<void (EvernusApplication::* Signal)(), class Key>
    void EvernusApplication::doRefreshMarketOrdersFromAPI(const Key &key, Character::IdType id, uint task)
    {
        mAPIManager.fetchMarketOrders(key, id, [task, id, this](MarketOrders &&data, const QString &error) {
            if (error.isEmpty())
            {
                importMarketOrders(id, data, std::is_same<Key, CorpKey>());
                emit (this->*Signal)();
                emit externalOrdersChangedWithMarketOrders();
            }

            emit taskEnded(task, error);
        });
    }

    template<class T, class Data>
    void EvernusApplication::asyncBatchStore(const T &repo, const Data &data, bool hasId)
    {
        asyncExecute(std::bind(&T::template batchStore<Data>, &repo, std::cref(data), hasId));
    }

    template<class Func, class... Args>
    void EvernusApplication::asyncExecute(Func &&func, Args && ...args)
    {
        qDebug() << "Stating async task...";

        auto future = std::async(std::launch::async, std::forward<Func>(func), std::forward<Args>(args)...);
        while (future.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
            processEvents(QEventLoop::ExcludeUserInputEvents);

        qDebug() << "Done.";
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignRight, Qt::white);
        processEvents();
    }
}
