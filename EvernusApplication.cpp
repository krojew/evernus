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

#include <QCommandLineParser>
#include <QSslConfiguration>
#include <QStandardPaths>
#include <QSplashScreen>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QSet>

#include "ExternalOrderImporterNames.h"
#include "LanguageSelectDialog.h"
#include "StatisticsSettings.h"
#include "UpdaterSettings.h"
#include "NetworkSettings.h"
#include "ImportSettings.h"
#include "WalletSettings.h"
#include "PriceSettings.h"
#include "OrderSettings.h"
#include "PathSettings.h"
#include "HttpSettings.h"
#include "SyncSettings.h"
#include "SimpleCrypt.h"
#include "HttpService.h"
#include "SyncDialog.h"
#include "UISettings.h"
#include "PathUtils.h"
#include "Updater.h"

#include "qxtmailmessage.h"

#include "EvernusApplication.h"
#include "SSOMessageBox.h"

#define STR_VALUE(s) #s
#define EVERNUS_TEXT(s) STR_VALUE(s)

#if defined(EVERNUS_CLIENT_ID) && defined(EVERNUS_CLIENT_SECRET)
#   define EVERNUS_CLIENT_ID_TEXT EVERNUS_TEXT(EVERNUS_CLIENT_ID)
#   define EVERNUS_CLIENT_SECRET_TEXT EVERNUS_TEXT(EVERNUS_CLIENT_SECRET)
#else
#   define EVERNUS_CLIENT_ID_TEXT ""
#   define EVERNUS_CLIENT_SECRET_TEXT ""
#endif

namespace Evernus
{
    const QString EvernusApplication::versionKey = "version";

    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication(argc, argv)
        , ExternalOrderImporterRegistry()
        , CacheTimerProvider()
        , ItemCostProvider()
        , RepositoryProvider()
        , LMeveDataProvider()
        , TaskManager()
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

        const auto forceVersionArg = QStringLiteral("force-version");
        const auto noUpdateArg = QStringLiteral("no-update");
        const auto clientIdArg = QStringLiteral("client-id");
        const auto clientSecretArg = QStringLiteral("client-secret");

        QCommandLineParser parser;
        parser.setApplicationDescription(QCoreApplication::translate("main", "Evernus EVE Online trade tool"));
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOptions({
            { forceVersionArg, tr("Force specific version") },
            { noUpdateArg, tr("Don't run internal updater") },
            { clientIdArg, tr("SSO client id"), "id", EVERNUS_CLIENT_ID_TEXT },
            { clientSecretArg, tr("SSO client secret"), "secret", EVERNUS_CLIENT_SECRET_TEXT },
        });

        parser.process(*this);

        mClientId = parser.value(clientIdArg).toLatin1();
        mClientSecret = parser.value(clientSecretArg).toLatin1();

        if (parser.isSet(forceVersionArg))
            setApplicationVersion(parser.value(forceVersionArg));

        setProxySettings();

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

        mCharacterAssetProvider = std::make_unique<CachingAssetProvider>(*mCharacterRepository,
                                                                         *mAssetListRepository,
                                                                         *mItemRepository);
        mCorpAssetProvider = std::make_unique<CachingAssetProvider>(*mCharacterRepository,
                                                                    *mCorpAssetListRepository,
                                                                    *mItemRepository);

        mCharacterOrderProvider = std::make_unique<CachingMarketOrderProvider>(*mMarketOrderRepository);
        connect(mCharacterOrderProvider.get(), &CachingMarketOrderProvider::orderChanged,
                this, &EvernusApplication::scheduleMarketOrderChange);

        mCorpOrderProvider = std::make_unique<CachingMarketOrderProvider>(*mCorpMarketOrderRepository);
        connect(mCorpOrderProvider.get(), &CachingMarketOrderProvider::orderChanged,
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
                                                                 *mCitadelRepository,
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

        showSplashMessage(tr("Clearing old market orders..."), splash);
        deleteOldMarketOrders();

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

        if (parser.isSet(noUpdateArg))
            Updater::getInstance().updateDatabaseVersion(mMainDb);
        else
            Updater::getInstance().performVersionMigration(*this);

        settings.setValue(versionKey, applicationVersion());

        connect(&mSmtp, QOverload<const QByteArray &>::of(&QxtSmtp::authenticationFailed),
                this, &EvernusApplication::showSmtpError);
        connect(&mSmtp, QOverload<const QByteArray &>::of(&QxtSmtp::connectionFailed),
                this, &EvernusApplication::showSmtpError);
        connect(&mSmtp, QOverload<const QByteArray &>::of(&QxtSmtp::encryptionFailed),
                this, &EvernusApplication::showSmtpError);
        connect(&mSmtp, &QxtSmtp::finished, &mSmtp, &QxtSmtp::disconnectFromHost);
        setSmtpSettings();

        if (settings.value(UpdaterSettings::autoUpdateKey, UpdaterSettings::autoUpdateDefault).toBool())
            Updater::getInstance().checkForUpdates(true);

        mESIManager = std::make_unique<ESIManager>(mClientId, mClientSecret, *mDataProvider, *mCharacterRepository);
        connect(mESIManager.get(), &ESIManager::error, this, &EvernusApplication::showGenericError);

        mDataProvider->precacheNames(*mESIManager);
    }

    void EvernusApplication::registerImporter(const std::string &name, std::unique_ptr<ExternalOrderImporter> &&importer)
    {
        Q_ASSERT(mExternalOrderImporters.find(name) == std::end(mExternalOrderImporters));

        connect(importer.get(), &ExternalOrderImporter::statusChanged, this, &EvernusApplication::showPriceImportStatus, Qt::QueuedConnection);
        connect(importer.get(), &ExternalOrderImporter::error, this, &EvernusApplication::showPriceImportError, Qt::QueuedConnection);
        connect(importer.get(), &ExternalOrderImporter::genericError, this, &EvernusApplication::showGenericError, Qt::QueuedConnection);
        connect(importer.get(), &ExternalOrderImporter::externalOrdersChanged, this, &EvernusApplication::finishExternalOrderImport, Qt::QueuedConnection);
        mExternalOrderImporters.emplace(name, std::move(importer));
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
        case TimerType::CorpAssetList:
            it = mCorpAssetsUtcCacheTimes.find(id);
            if (it == std::end(mCorpAssetsUtcCacheTimes))
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
        case TimerType::CorpAssetList:
            mCorpAssetsUtcCacheTimes[id] = dt;
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
        case TimerType::CorpAssetList:
            it = mCorpAssetsUtcUpdateTimes.find(id);
            if (it == std::end(mCorpAssetsUtcUpdateTimes))
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

    const CorpAssetValueSnapshotRepository &EvernusApplication::getCorpAssetValueSnapshotRepository() const noexcept
    {
        return *mCorpAssetValueSnapshotRepository;
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

    const EveTypeRepository &EvernusApplication::getEveTypeRepository() const noexcept
    {
        return *mEveTypeRepository;
    }

    const MarketGroupRepository &EvernusApplication::getMarketGroupRepository() const noexcept
    {
        return *mMarketGroupRepository;
    }

    const CacheTimerRepository &EvernusApplication::getCacheTimerRepository() const noexcept
    {
        return *mCacheTimerRepository;
    }

    const UpdateTimerRepository &EvernusApplication::getUpdateTimerRepository() const noexcept
    {
        return *mUpdateTimerRepository;
    }

    const ItemRepository &EvernusApplication::getItemRepository() const noexcept
    {
        return *mItemRepository;
    }

    const CitadelRepository &EvernusApplication::getCitadelRepository() const noexcept
    {
        return *mCitadelRepository;
    }

    const RegionTypePresetRepository &EvernusApplication::getRegionTypePresetRepository() const noexcept
    {
        return *mRegionTypePresetRepository;
    }

    const ItemRepository &EvernusApplication::getCorpItemRepository() const noexcept
    {
        return *mCorpItemRepository;
    }

    const RegionStationPresetRepository &EvernusApplication::getRegionStationPresetRepository() const noexcept
    {
        return *mRegionStationPresetRepository;
    }

    std::vector<std::shared_ptr<LMeveTask>> EvernusApplication::getTasks(Character::IdType characterId) const
    {
        const auto it = mLMeveTaskCache.find(characterId);
        if (it != std::end(mLMeveTaskCache))
            return it->second;

        return mLMeveTaskCache.emplace(characterId, mLMeveTaskRepository->fetchForCharacter(characterId)).first->second;
    }

    uint EvernusApplication::startTask(const QString &description)
    {
        QMetaObject::invokeMethod(this,
                                  "taskStarted",
                                  Qt::QueuedConnection,
                                  Q_ARG(uint, mTaskId),
                                  Q_ARG(QString, description));
        return mTaskId++;
    }

    uint EvernusApplication::startTask(uint parentTask, const QString &description)
    {
        if (parentTask == TaskConstants::invalidTask)
            return startTask(description);

        QMetaObject::invokeMethod(this,
                                  "taskStarted",
                                  Qt::QueuedConnection,
                                  Q_ARG(uint, mTaskId),
                                  Q_ARG(uint, parentTask),
                                  Q_ARG(QString, description));
        return mTaskId++;
    }

    void EvernusApplication::updateTask(uint taskId, const QString &description)
    {
        QMetaObject::invokeMethod(this,
                                  "taskInfoChanged",
                                  Qt::QueuedConnection,
                                  Q_ARG(uint, taskId),
                                  Q_ARG(QString, description));
    }

    void EvernusApplication::endTask(uint taskId, const QString &error)
    {
        QMetaObject::invokeMethod(this,
                                  "taskEnded",
                                  Qt::QueuedConnection,
                                  Q_ARG(uint, taskId),
                                  Q_ARG(QString, error));
    }

    QByteArray EvernusApplication::getSSOClientId() const
    {
        return mClientId;
    }

    QByteArray EvernusApplication::getSSOClientSecret() const
    {
        return mClientSecret;
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

    AssetProvider &EvernusApplication::getAssetProvider() const noexcept
    {
        return *mCharacterAssetProvider;
    }

    AssetProvider &EvernusApplication::getCorpAssetProvider() const noexcept
    {
        return *mCorpAssetProvider;
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
                            std::vector<uint> charTasks(characters.size());
                            for (auto i = 0u; i < characters.size(); ++i)
                                charTasks[i] = startTask(charListSubtask, getCharacterImportMessage(characters[i]));

                            if (shouldUseESIOverXML())
                            {
                                for (auto i = 0u; i < characters.size(); ++i)
                                    importCharacterFromESI(characters[i], charTasks[i], *key);
                            }
                            else
                            {
                                for (auto i = 0u; i < characters.size(); ++i)
                                    importCharacterFromXML(characters[i], charTasks[i], *key);
                            }
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
            const auto charSubtask = startTask(parentTask, getCharacterImportMessage(id));

            if (!checkImportAndEndTask(id, TimerType::Character, charSubtask))
                return;

            if (shouldUseESIOverXML())
                importCharacterFromESI(id, charSubtask, *getCharacterKey(id));
            else
                importCharacterFromXML(id, charSubtask, *getCharacterKey(id));
        }
        catch (const KeyRepository::NotFoundException &)
        {
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }
    }

    void EvernusApplication::refreshCharacterAssets(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing assets: " << id;

        const auto assetSubtask = startTask(parentTask, tr("Fetching assets for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::AssetList, assetSubtask))
            return;

        if (shouldUseESIOverXML())
            importCharacterAssetsFromESI(id, assetSubtask);
        else
            importCharacterAssetsFromXML(id, assetSubtask);
    }

    void EvernusApplication::importCharacterAssetsFromXML(Character::IdType id, uint importSubtask)
    {
        try
        {
            const auto key = getCharacterKey(id);

            markImport(id, TimerType::AssetList);
            mAPIManager.fetchAssets(*key, id, [=](AssetList &&data, const QString &error) {
                unmarkImport(id, TimerType::AssetList);

                if (error.isEmpty())
                    updateCharacterAssets(id, data);

                emit taskEnded(importSubtask, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(importSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(importSubtask, tr("Character not found!"));
        }
    }

    void EvernusApplication::importCharacterAssetsFromESI(Character::IdType id, uint importSubtask)
    {
        Q_ASSERT(mESIManager);

        markImport(id, TimerType::AssetList);
        mESIManager->fetchCharacterAssets(id, [=](auto &&assets, const auto &error, const auto &expires) {
            unmarkImport(id, TimerType::AssetList);

            if (error.isEmpty())
            {
                setUtcCacheTimer(id, TimerType::AssetList, expires);
                updateCharacterAssets(id, assets);
            }

            emit taskEnded(importSubtask, error);
        });
    }

    void EvernusApplication::importCharacterMarketOrdersFromXML(Character::IdType id, uint importSubtask)
    {
        try
        {
            const auto key = getCharacterKey(id);
            doRefreshMarketOrdersFromAPI<&EvernusApplication::characterMarketOrdersChanged>(*key, id, importSubtask, TimerType::MarketOrders);
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(importSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(importSubtask, tr("Character not found!"));
        }
    }

    void EvernusApplication::importCharacterMarketOrdersFromESI(Character::IdType id, uint importSubtask)
    {
        Q_ASSERT(mESIManager);

        markImport(id, TimerType::MarketOrders);
        mESIManager->fetchCharacterMarketOrders(id, [=](auto &&data, const auto &error, const auto &expires) {
            unmarkImport(id, TimerType::MarketOrders);

            if (error.isEmpty())
            {
                setUtcCacheTimer(id, TimerType::MarketOrders, expires);
                importMarketOrders(id, data, false);

                emit characterMarketOrdersChanged();
                emit externalOrdersChangedWithMarketOrders();
            }

            emit taskEnded(importSubtask, error);
        });
    }

    void EvernusApplication::importCharacterWalletJournalFromXML(Character::IdType id, uint importSubtask)
    {
        try
        {
            const auto key = getCharacterKey(id);
            const auto maxId = mWalletJournalEntryRepository->getLatestEntryId(id);

            if (maxId == WalletJournalEntry::invalidId)
                emit taskInfoChanged(importSubtask, tr("Fetching wallet journal for character %1 (this may take a while)...").arg(id));

            mAPIManager.fetchWalletJournal(*key, id, WalletJournalEntry::invalidId, maxId,
                                           [=](const WalletJournal &data, const QString &error) {
                if (error.isEmpty())
                    updateCharacterWalletJournal(id, data);

                emit taskEnded(importSubtask, error);
            });
        }
        catch (const KeyRepository::NotFoundException &)
        {
            emit taskEnded(importSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(importSubtask, tr("Character not found!"));
        }
    }

    void EvernusApplication::importCharacterWalletJournalFromESI(Character::IdType id, uint importSubtask)
    {
        const auto maxId = mWalletJournalEntryRepository->getLatestEntryId(id);

        if (maxId == WalletJournalEntry::invalidId)
            emit taskInfoChanged(importSubtask, tr("Fetching wallet journal for character %1 (this may take a while)...").arg(id));

        Q_ASSERT(mESIManager);

        markImport(id, TimerType::WalletJournal);
        mESIManager->fetchCharacterWalletJournal(id, maxId, [=](auto &&data, const auto &error, const auto &expires) {
            unmarkImport(id, TimerType::WalletJournal);

            if (error.isEmpty())
            {
                setUtcCacheTimer(id, TimerType::WalletJournal, expires);
                updateCharacterWalletJournal(id, data);
            }

            emit taskEnded(importSubtask, error);
        });
    }

    void EvernusApplication::refreshCharacterContracts(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing contracts: " << id;

        const auto task = startTask(tr("Fetching contracts for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::Contracts, task))
            return;

        try
        {
            const auto key = getCharacterKey(id);

            markImport(id, TimerType::Contracts);
            mAPIManager.fetchContracts(*key, id, [key, task, id, this](Contracts &&data, const QString &error) {
                unmarkImport(id, TimerType::Contracts);

                if (error.isEmpty())
                {
                    const auto it = std::remove_if(std::begin(data), std::end(data), [](const Contract &contract) {
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

                    this->handleIncomingContracts<&Evernus::EvernusApplication::characterContractsChanged>(key,
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

    void EvernusApplication::refreshCharacterWalletJournal(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing wallet journal: " << id;

        const auto task = startTask(tr("Fetching wallet journal for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::WalletJournal, task))
            return;

        // TODO: enable when complete
//        if (shouldUseESIOverXML())
//            importCharacterWalletJournalFromESI(id, task);
//        else
            importCharacterWalletJournalFromXML(id, task);
    }

    void EvernusApplication::refreshCharacterWalletTransactions(Character::IdType id, uint parentTask, bool force)
    {
        qDebug() << "Refreshing wallet transactions: " << id;

        const auto task = startTask(tr("Fetching wallet transactions for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!force && !checkImportAndEndTask(id, TimerType::WalletTransactions, task))
            return;

        try
        {
            const auto key = getCharacterKey(id);
            const auto maxId = mWalletTransactionRepository->getLatestEntryId(id);

            if (maxId == WalletTransaction::invalidId)
                emit taskInfoChanged(task, tr("Fetching wallet transactions for character %1 (this may take a while)...").arg(id));

            markImport(id, TimerType::WalletTransactions);
            mAPIManager.fetchWalletTransactions(*key, id, WalletTransaction::invalidId, maxId,
                                                [task, id, this](const auto &data, const auto &error) {
                unmarkImport(id, Evernus::TimerType::WalletTransactions);

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

                    emit characterWalletTransactionsChanged();
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

    void EvernusApplication::refreshCharacterMarketOrdersFromAPI(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing market orders from API: " << id;

        const auto task = startTask(tr("Fetching market orders for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::MarketOrders, task))
            return;

        if (shouldUseESIOverXML())
            importCharacterMarketOrdersFromESI(id, task);
        else
            importCharacterMarketOrdersFromXML(id, task);
    }

    void EvernusApplication::refreshCharacterMarketOrdersFromLogs(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing market orders from logs: " << id;

        const auto task = startTask(tr("Fetching market orders for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        importMarketOrdersFromLogs(id, task, false);

        emit taskEnded(task, QString{});
    }

    void EvernusApplication::refreshCorpAssets(Character::IdType id, uint parentTask)
    {
        qDebug() << "Refreshing corp assets: " << id;

        const auto assetSubtask = startTask(parentTask, tr("Fetching corporation assets for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!checkImportAndEndTask(id, TimerType::CorpAssetList, assetSubtask))
            return;

        try
        {
            const auto key = getCorpKey(id);

            markImport(id, TimerType::CorpAssetList);
            mAPIManager.fetchAssets(*key, id, [=](AssetList &&data, const QString &error) {
                unmarkImport(id, TimerType::CorpAssetList);

                if (error.isEmpty())
                {
                    mCorpAssetListRepository->deleteForCharacter(id);
                    mCorpAssetProvider->setForCharacter(id, data);
                    mCorpAssetListRepository->store(data);

                    QSettings settings;

                    if (settings.value(Evernus::ImportSettings::autoUpdateAssetValueKey, Evernus::ImportSettings::autoUpdateAssetValueDefault).toBool() &&
                        settings.value(StatisticsSettings::automaticSnapshotsKey, StatisticsSettings::automaticSnapshotsKey).toBool())
                    {
                        computeCorpAssetListSellValueSnapshot(data);
                    }

                    saveUpdateTimer(Evernus::TimerType::CorpAssetList, mCorpAssetsUtcUpdateTimes, id);

                    emit corpAssetsChanged();
                }

                emit taskEnded(assetSubtask, error);
            });
        }
        catch (const CorpKeyRepository::NotFoundException &)
        {
            emit taskEnded(assetSubtask, tr("Key not found!"));
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            emit taskEnded(assetSubtask, tr("Character not found!"));
        }
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

            markImport(id, TimerType::CorpContracts);
            mAPIManager.fetchContracts(*key, id, [key, task, id, this](auto &&data, const auto &error) {
                unmarkImport(id, Evernus::TimerType::CorpContracts);

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

            QSettings settings;
            const auto accountKey = settings.value(ImportSettings::corpWalletDivisionKey, ImportSettings::corpWalletDivisionDefault).toInt();

            markImport(id, TimerType::CorpWalletJournal);
            mAPIManager.fetchWalletJournal(*key, id, mCharacterRepository->getCorporationId(id), WalletJournalEntry::invalidId, maxId, accountKey,
                                           [task, id, this](WalletJournal &&data, const QString &error) {
                unmarkImport(id, TimerType::CorpWalletJournal);

                if (error.isEmpty())
                {
                    QSettings settings;
                    if (settings.value(StatisticsSettings::automaticSnapshotsKey, StatisticsSettings::automaticSnapshotsDefault).toBool())
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

                        asyncBatchStore(*mCorpWalletSnapshotRepository, snapshots, false);
                    }

                    asyncBatchStore(*mCorpWalletJournalEntryRepository, data, true);

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

    void EvernusApplication::refreshCorpWalletTransactions(Character::IdType id, uint parentTask, bool force)
    {
        qDebug() << "Refreshing corp wallet transactions: " << id;

        const auto task = startTask(tr("Fetching corporation wallet transactions for character %1...").arg(id));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!force && !checkImportAndEndTask(id, TimerType::CorpWalletTransactions, task))
            return;

        try
        {
            const auto key = getCorpKey(id);
            const auto maxId = mCorpWalletTransactionRepository->getLatestEntryId(id);
            const auto corpId = mCharacterRepository->getCorporationId(id);

            if (maxId == WalletTransaction::invalidId)
                emit taskInfoChanged(task, tr("Fetching corporation wallet transactions for character %1 (this may take a while)...").arg(id));

            QSettings settings;
            const auto accountKey = settings.value(ImportSettings::corpWalletDivisionKey, ImportSettings::corpWalletDivisionDefault).toInt();

            markImport(id, TimerType::CorpWalletTransactions);
            mAPIManager.fetchWalletTransactions(*key, id, corpId, WalletTransaction::invalidId, maxId, accountKey,
                                                [task, id, corpId, this](auto &&data, const auto &error) {
                unmarkImport(id, Evernus::TimerType::CorpWalletTransactions);

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
            doRefreshMarketOrdersFromAPI<&EvernusApplication::corpMarketOrdersChanged>(*key, id, task, TimerType::CorpMarketOrders);
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

        mAPIManager.fetchConquerableStationList([=](const auto &list, const auto &error) {
            if (error.isEmpty())
            {
                mDataProvider->clearStationCache();

                mConquerableStationRepository->deleteAll();
                asyncBatchStore(*mConquerableStationRepository, list, true);

                emit conquerableStationsChanged();
            }

            emit taskEnded(task, error);
        });
    }

    void EvernusApplication::refreshCitadels()
    {
        qDebug() << "Refreshing citadels...";

        const auto task = startTask(tr("Fetching citadels..."));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        mCitadelManager.fetchCitadels([=](auto &&citadels, const auto &error) {
            if (error.isEmpty())
            {
                mDataProvider->clearCitadelCache();

                asyncExecute([=, citadels = std::move(citadels)] {
                    mCitadelRepository->replace(std::move(citadels));
                    mExternalOrderRepository->fixMissingData(*mCitadelRepository);
                    emit citadelsChanged();
                });
            }

            emit taskEnded(task, error);
        });
    }

    void EvernusApplication::refreshAllExternalOrders(Character::IdType id)
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
            refreshExternalOrdersFromFile(id, target);
            break;
        default:
            refreshExternalOrdersFromWeb(id, target);
        }
    }

    void EvernusApplication::refreshExternalOrdersFromWeb(Character::IdType id, const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::webImporter, id, target);
    }

    void EvernusApplication::refreshExternalOrdersFromFile(Character::IdType id, const ExternalOrderImporter::TypeLocationPairs &target)
    {
        importExternalOrders(ExternalOrderImporterNames::logImporter, id, target);
    }

    void EvernusApplication::finishExternalOrderImport(const std::vector<ExternalOrder> &orders)
    {
        try
        {
            emit taskInfoChanged(mCurrentExternalOrderImportTask, tr("Saving %1 imported orders...").arg(orders.size()));

            updateExternalOrdersAndAssetValue(orders);
            finishExternalOrderImportTask(QString{});
        }
        catch (const std::exception &e)
        {
            finishExternalOrderImportTask(e.what());
        }
    }

    void EvernusApplication::updateExternalOrdersAndAssetValue(const std::vector<ExternalOrder> &orders)
    {
        asyncExecute(&CachingEveDataProvider::updateExternalOrders, mDataProvider.get(), std::cref(orders));

        QSettings settings;
        if (settings.value(ImportSettings::autoUpdateAssetValueKey, ImportSettings::autoUpdateAssetValueDefault).toBool())
        {
            const auto assets = mCharacterAssetProvider->fetchAllAssets();
            for (const auto &list : assets)
                computeAssetListSellValueSnapshot(*list);
        }

        emit externalOrdersChanged();
    }

    void EvernusApplication::handleNewPreferences()
    {
        QSettings settings;
        updateTranslator(settings.value(UISettings::languageKey).toString());

        setProxySettings();

        mHttpSessionManager.shutdown();
        mHttpSessionManager.setPort(settings.value(HttpSettings::portKey, HttpSettings::portDefault).value<quint16>());

        if (settings.value(HttpSettings::enabledKey, HttpSettings::enabledDefault).toBool())
            mHttpSessionManager.start();

        mCharacterItemCostCache.clear();
        mDataProvider->handleNewPreferences();

        setSmtpSettings();

        mESIManager->handleNewPreferences();

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

        emit characterMarketOrdersChanged();
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

    void EvernusApplication::clearCorpWalletData()
    {
        mCorpWalletJournalEntryRepository->deleteAll();
        mCorpWalletTransactionRepository->deleteAll();

        emit corpWalletJournalChanged();
        emit corpWalletTransactionsChanged();
    }

    void EvernusApplication::clearCitadelCache()
    {
        mDataProvider->clearCitadelCache();
        emit citadelsChanged();
    }

    void EvernusApplication::makeValueSnapshots(Character::IdType id)
    {
        qDebug() << "Making all value snapshots for" << id;

        const auto time = QDateTime::currentDateTimeUtc();

        auto assets = mCharacterAssetProvider->fetchAssetsForCharacter(id);
        if (assets)
            computeAssetListSellValueSnapshot(*assets);

        assets = mCorpAssetProvider->fetchAssetsForCharacter(id);
        if (assets)
            computeCorpAssetListSellValueSnapshot(*assets);

        const auto charData = mCharacterRepository->find(id);
        if (charData)
            createWalletSnapshot(id, charData->getISK());

        auto orderData = mMarketOrderRepository->getAggregatedData(id);

        MarketOrderValueSnapshot orderSnapshot;
        orderSnapshot.setTimestamp(time);
        orderSnapshot.setCharacterId(id);
        orderSnapshot.setBuyValue(orderData.mBuyData.mPriceSum);
        orderSnapshot.setSellValue(orderData.mSellData.mPriceSum);

        mMarketOrderValueSnapshotRepository->store(orderSnapshot);

        orderData = mCorpMarketOrderRepository->getAggregatedData(id);

        CorpMarketOrderValueSnapshot corpOrderSnapshot;
        corpOrderSnapshot.setTimestamp(time);
        corpOrderSnapshot.setCorporationId(mCharacterRepository->getCorporationId(id));
        corpOrderSnapshot.setBuyValue(orderData.mBuyData.mPriceSum);
        corpOrderSnapshot.setSellValue(orderData.mSellData.mPriceSum);

        mCorpMarketOrderValueSnapshotRepository->store(corpOrderSnapshot);

        emit snapshotsTaken();
    }

    void EvernusApplication::showInEve(EveType::IdType typeId, Character::IdType charId)
    {
        mESIManager->openMarketDetails(typeId, charId);
    }

    void EvernusApplication::setDestinationInEve(quint64 locationId, Character::IdType charId)
    {
        mESIManager->setDestination(locationId, charId);
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
        QMetaObject::invokeMethod(this, "updateCharacterMarketOrders", Qt::QueuedConnection);
    }

    void EvernusApplication::updateCharacterMarketOrders()
    {
        mMarketOrderUpdateScheduled = false;
        emit characterMarketOrdersChanged();
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

    void EvernusApplication::showGenericError(const QString &info)
    {
        SSOMessageBox::showMessage(info, activeWindow());
    }

    void EvernusApplication::showPriceImportStatus(const QString &info)
    {
        Q_ASSERT(mCurrentExternalOrderImportTask != TaskConstants::invalidTask);
        emit taskInfoChanged(mCurrentExternalOrderImportTask, info);
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
        mQtBaseTranslator.load(locale, "qtbase", "_", applicationDirPath() + UISettings::translationPath);
        mQtScriptTranslator.load(locale, "qtscript", "_", applicationDirPath() + UISettings::translationPath);
        mQtXmlPatternsTranslator.load(locale, "qtxmlpatterns", "_", applicationDirPath() + UISettings::translationPath);

        installTranslator(&mQtXmlPatternsTranslator);
        installTranslator(&mQtScriptTranslator);
        installTranslator(&mQtBaseTranslator);
        installTranslator(&mQtTranslator);
        installTranslator(&mTranslator);
    }

    void EvernusApplication::createDb()
    {
        DatabaseUtils::createDb(mMainDb, "main.db");

        if (!mEveDb.isValid())
            throw std::runtime_error{"Error crating Eve DB object!"};

        auto eveDbPath = applicationDirPath() + "/resources/eve.db";
        qDebug() << "Eve DB path:" << eveDbPath;

        if (!QFile::exists(eveDbPath))
        {
            eveDbPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, applicationName() + "/resources/eve.db");
            qDebug() << "Eve DB path:" << eveDbPath;

            if (!QFile::exists(eveDbPath))
                throw std::runtime_error{"Cannot find Eve DB!"};
        }

        mEveDb.setDatabaseName(eveDbPath);
        mEveDb.setConnectOptions("QSQLITE_OPEN_READONLY");

        if (!mEveDb.open())
            throw std::runtime_error{"Error opening Eve DB!"};

        // disable syncing changes to the disk between
        // each transaction. This means the database can become
        // corrupted in the event of a power failure or OS crash
        // but NOT in the event of an application error
        mMainDb.exec("PRAGMA synchronous = OFF");

        mKeyRepository.reset(new KeyRepository{mMainDb});
        mCorpKeyRepository.reset(new CorpKeyRepository{mMainDb});
        mCharacterRepository.reset(new CharacterRepository{mMainDb});
        mItemRepository.reset(new ItemRepository{false, mMainDb});
        mCorpItemRepository.reset(new ItemRepository{true, mMainDb});
        mAssetListRepository.reset(new AssetListRepository{false, mMainDb, *mItemRepository});
        mCorpAssetListRepository.reset(new AssetListRepository{true, mMainDb, *mCorpItemRepository});
        mConquerableStationRepository.reset(new ConquerableStationRepository{mMainDb});
        mWalletSnapshotRepository.reset(new WalletSnapshotRepository{mMainDb});
        mCorpWalletSnapshotRepository.reset(new CorpWalletSnapshotRepository{mMainDb});
        mExternalOrderRepository.reset(new ExternalOrderRepository{mMainDb});
        mAssetValueSnapshotRepository.reset(new AssetValueSnapshotRepository{mMainDb});
        mCorpAssetValueSnapshotRepository.reset(new CorpAssetValueSnapshotRepository{mMainDb});
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
        mCitadelRepository.reset(new CitadelRepository{mMainDb});
        mRegionTypePresetRepository.reset(new RegionTypePresetRepository{mMainDb});
        mRegionStationPresetRepository.reset(new RegionStationPresetRepository{mMainDb});
    }

    void EvernusApplication::createDbSchema()
    {
        mKeyRepository->create();
        mCharacterRepository->create(*mKeyRepository);
        mCorpKeyRepository->create(*mCharacterRepository);
        mAssetListRepository->create(*mCharacterRepository);
        mCorpAssetListRepository->create(*mCharacterRepository);
        mItemRepository->create(*mAssetListRepository);
        mCorpItemRepository->create(*mCorpAssetListRepository);
        mConquerableStationRepository->create();
        mWalletSnapshotRepository->create(*mCharacterRepository);
        mCorpWalletSnapshotRepository->create();
        mAssetValueSnapshotRepository->create(*mCharacterRepository);
        mCorpAssetValueSnapshotRepository->create();
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
        mCitadelRepository->create();
        mRegionTypePresetRepository->create();
        mRegionStationPresetRepository->create();
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
            case TimerType::CorpAssetList:
                mCorpAssetsUtcCacheTimes[timer->getCharacterId()] = timer->getCacheUntil();
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
            case TimerType::CorpAssetList:
                mCorpAssetsUtcUpdateTimes[timer->getCharacterId()] = timer->getUpdateTime();
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
            mWalletJournalEntryRepository->deleteOldEntries(journalDt);
        }

        if (settings.value(WalletSettings::deleteOldTransactionsKey, WalletSettings::deleteOldTransactionsDefault).toBool())
        {
            const auto transactionDt = QDateTime::currentDateTimeUtc().addDays(
                -settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());
            mWalletTransactionRepository->deleteOldEntries(transactionDt);
        }
    }

    void EvernusApplication::deleteOldMarketOrders()
    {
        QSettings settings;
        if (settings.value(OrderSettings::deleteOldMarketOrdersKey, OrderSettings::deleteOldMarketOrdersDefault).toBool())
        {
            const auto oldDt = QDateTime::currentDateTimeUtc().addDays(
                -settings.value(OrderSettings::oldMarketOrderDaysKey, OrderSettings::oldMarketOrderDaysDefault).toInt());
            mMarketOrderRepository->deleteOldEntries(oldDt);
            mCorpMarketOrderRepository->deleteOldEntries(oldDt);
        }
    }

    void EvernusApplication::importCharacterFromXML(Character::IdType id, uint task, const Key &key)
    {
        markImport(id, TimerType::Character);
        mAPIManager.fetchCharacter(key, id, [task, id, this](Character &&data, const QString &error) {
            unmarkImport(id, TimerType::Character);

            if (error.isEmpty())
                updateCharacter(data);

            emit taskEnded(task, error);
        });
    }

    void EvernusApplication::importCharacterFromESI(Character::IdType id, uint task, const Key &key)
    {
        Q_ASSERT(mESIManager);

        markImport(id, TimerType::Character);
        mESIManager->fetchCharacter(id, [=](auto &&data, const auto &error, const auto &expires) {
            unmarkImport(id, TimerType::Character);

            if (error.isEmpty())
            {
                data.setKeyId(key.getId());
                updateCharacter(data);
            }

            emit taskEnded(task, error);
        });
    }

    void EvernusApplication::importExternalOrders(const std::string &importerName, Character::IdType id, const ExternalOrderImporter::TypeLocationPairs &target)
    {
        if (mCurrentExternalOrderImportTask != TaskConstants::invalidTask)
            return;

        qDebug() << "Refreshing item prices using importer:" << importerName.c_str();

        const auto it = mExternalOrderImporters.find(importerName);
        Q_ASSERT(it != std::end(mExternalOrderImporters));

        mCurrentExternalOrderImportTask = startTask(tr("Importing item prices..."));
        processEvents(QEventLoop::ExcludeUserInputEvents);

        it->second->fetchExternalOrders(id, target);
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
                    if (!issued.isValid())
                    {
                        issued = QDateTime::fromString(values[issuedColumn], "yyyy-MM-dd");
                        if (!issued.isValid())
                        {
                            // thank CCP
                            issued = QDateTime::currentDateTimeUtc();
                        }
                    }
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
                    emit characterMarketOrdersChanged();

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
            const auto makeSnapshot = settings.value(StatisticsSettings::automaticSnapshotsKey, StatisticsSettings::automaticSnapshotsDefault).toBool();
            const auto makeCorpSnapshot = makeSnapshot && settings.value(ImportSettings::makeCorpSnapshotsKey, ImportSettings::makeCorpSnapshotsDefault).toBool();
            const auto emailNotification = settings.value(ImportSettings::autoImportEnabledKey, ImportSettings::autoImportEnabledDefault).toBool() &&
                                           settings.value(ImportSettings::emailNotificationsEnabledKey, ImportSettings::emailNotificationsEnabledDefault).toBool();
            const auto defaultCustomStation = settings.value(OrderSettings::defaultCustomStationKey).toUInt();

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
                    order.setCustomStationId(cIt->second.mCustomStation);
                    order.setNotes(cIt->second.mNotes);
                    order.setColorTag(cIt->second.mColorTag);

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
                    if (defaultCustomStation != 0)
                        order.setCustomStationId(defaultCustomStation);
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
                    {
                        toArchive.emplace_back(order.first);
                    }
                    else
                    {
                        toFulfill.emplace_back(order.first);
                        mPendingAutoCostOrders.emplace(order.first);
                    }
                }
            }

            if (!toArchive.empty())
                orderRepo.archive(toArchive);
            if (!toFulfill.empty())
                orderRepo.fulfill(toFulfill);

            asyncBatchStore(orderRepo, orders, true);

            if (!corp)
            {
                if (makeSnapshot)
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
                    refreshCorpWalletTransactions(id, TaskConstants::invalidTask, true);
                else
                    refreshCharacterWalletTransactions(id, TaskConstants::invalidTask, true);
            }

            if (settings.value(PriceSettings::refreshPricesWithOrdersKey).toBool())
                refreshAllExternalOrders(id);
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

    void EvernusApplication::computeAssetListSellValueSnapshot(const AssetList &list) const
    {
        try
        {
            AssetValueSnapshot snapshot;
            snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
            snapshot.setBalance(getTotalAssetListValue(list));
            snapshot.setCharacterId(list.getCharacterId());

            mAssetValueSnapshotRepository->store(snapshot);
        }
        catch (const ExternalOrderRepository::NotFoundException &)
        {
        }
    }

    void EvernusApplication::computeCorpAssetListSellValueSnapshot(const AssetList &list) const
    {
        try
        {
            CorpAssetValueSnapshot snapshot;
            snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
            snapshot.setBalance(getTotalAssetListValue(list));
            snapshot.setCorporationId(mCharacterRepository->getCorporationId(list.getCharacterId()));

            mCorpAssetValueSnapshotRepository->store(snapshot);
        }
        catch (const ExternalOrderRepository::NotFoundException &)
        {
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }
    }

    void EvernusApplication::updateCharacterAssets(Character::IdType id, AssetList &list)
    {
        mItemRepository->fillCustomValues(list);
        mAssetListRepository->deleteForCharacter(id);
        mCharacterAssetProvider->setForCharacter(id, list);
        mAssetListRepository->store(list);

        QSettings settings;

        if (settings.value(Evernus::ImportSettings::autoUpdateAssetValueKey, Evernus::ImportSettings::autoUpdateAssetValueDefault).toBool() &&
            settings.value(StatisticsSettings::automaticSnapshotsKey, StatisticsSettings::automaticSnapshotsKey).toBool())
        {
            computeAssetListSellValueSnapshot(list);
        }

        saveUpdateTimer(Evernus::TimerType::AssetList, mAssetsUtcUpdateTimes, id);

        emit characterAssetsChanged();
    }

    void EvernusApplication::updateCharacter(Character &character)
    {
        QSettings settings;

        const auto charId = character.getId();

        try
        {
            const auto prevData = mCharacterRepository->find(charId);

            if (!settings.value(ImportSettings::importSkillsKey, ImportSettings::importSkillsDefault).toBool())
            {
                character.setOrderAmountSkills(prevData->getOrderAmountSkills());
                character.setTradeRangeSkills(prevData->getTradeRangeSkills());
                character.setFeeSkills(prevData->getFeeSkills());
                character.setContractSkills(prevData->getContractSkills());
            }

            character.setCorpStanding(prevData->getCorpStanding());
            character.setFactionStanding(prevData->getFactionStanding());
            character.setEnabled(prevData->isEnabled());
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }

        mMainDb.exec("PRAGMA foreign_keys = OFF;");
        mCharacterRepository->store(character);
        mMainDb.exec("PRAGMA foreign_keys = ON;");

        if (settings.value(StatisticsSettings::automaticSnapshotsKey, StatisticsSettings::automaticSnapshotsDefault).toBool())
            createWalletSnapshot(charId, character.getISK());

        saveUpdateTimer(TimerType::Character, mCharacterUtcUpdateTimes, charId);

        QMetaObject::invokeMethod(this, "scheduleCharacterUpdate", Qt::QueuedConnection);

        const auto cacheTimer = mCharacterUtcCacheTimes[charId];
        if (cacheTimer.isValid())
        {
            Evernus::CacheTimer timer;
            timer.setCharacterId(charId);
            timer.setType(TimerType::Character);
            timer.setCacheUntil(cacheTimer);

            mCacheTimerRepository->store(timer);
        }
    }

    void EvernusApplication::updateCharacterWalletJournal(Character::IdType id, const WalletJournal &data)
    {
        QSettings settings;
        if (settings.value(StatisticsSettings::automaticSnapshotsKey, StatisticsSettings::automaticSnapshotsDefault).toBool())
        {
            std::vector<WalletSnapshot> snapshots;
            snapshots.reserve(data.size());

            QSet<QDateTime> usedSnapshots;

            for (auto &entry : data)
            {
                const auto timestamp = entry.getTimestamp();

                if (!usedSnapshots.contains(timestamp))
                {
                    WalletSnapshot snapshot;
                    snapshot.setTimestamp(timestamp);
                    snapshot.setBalance(entry.getBalance());
                    snapshot.setCharacterId(entry.getCharacterId());

                    snapshots.emplace_back(std::move(snapshot));
                    usedSnapshots << timestamp;
                }
            }

            asyncBatchStore(*mWalletSnapshotRepository, snapshots, false);
        }

        asyncBatchStore(*mWalletJournalEntryRepository, data, true);

        saveUpdateTimer(TimerType::WalletJournal, mWalletJournalUtcUpdateTimes, id);

        emit characterWalletJournalChanged();
    }

    double EvernusApplication::getTotalAssetListValue(const AssetList &list) const
    {
        QSettings settings;
        const auto customLocationId = (settings.value(ImportSettings::useCustomAssetStationKey, ImportSettings::useCustomAssetStationDefault).toBool()) ?
                                      (EveDataProvider::getStationIdFromPath(settings.value(ImportSettings::customAssetStationKey).toList())) :
                                      (0);

        auto value = 0.;
        for (const auto &item : list)
        {
            const auto locationId = (customLocationId != 0) ? (customLocationId) : (item->getLocationId());
            if (!locationId)
                continue;

            value += getTotalItemSellValue(*item, *locationId);
        }

        return value;
    }

    double EvernusApplication::getTotalItemSellValue(const Item &item, quint64 locationId) const
    {
        // return 0 for BPC
        if (item.isBPC(*mDataProvider))
            return 0.;

        const auto customValue = item.getCustomValue();
        if (customValue)
            return *customValue;

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

    void EvernusApplication::createWalletSnapshot(Character::IdType characterId, double balance)
    {
        WalletSnapshot snapshot;
        snapshot.setTimestamp(QDateTime::currentDateTimeUtc());
        snapshot.setBalance(balance);
        snapshot.setCharacterId(characterId);

        mWalletSnapshotRepository->store(snapshot);
    }

    bool EvernusApplication::shouldImport(Character::IdType id, TimerType type) const
    {
        if (mPendingImports.find(std::make_pair(id, type)) != std::end(mPendingImports))
            return false;

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

    void EvernusApplication::markImport(Character::IdType id, TimerType type)
    {
        mPendingImports.emplace(id, type);
    }

    void EvernusApplication::unmarkImport(Character::IdType id, TimerType type)
    {
        mPendingImports.erase(std::make_pair(id, type));
    }

    template<void (EvernusApplication::* Signal)(), class Key>
    void EvernusApplication::handleIncomingContracts(const Key &key,
                                                     const Contracts &data,
                                                     Character::IdType id,
                                                     const ContractItemRepository &itemRepo,
                                                     uint task)
    {
        static size_t pendingContractItemRequests = 0;
        static APIManager::ContractItemList contractItems;

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

                ++pendingContractItemRequests;

                const auto subTask = startTask(task, tr("Fetching contract items for contract %1...").arg(contract.getId()));
                mAPIManager.fetchContractItems(*key, id, contract.getId(), [=, &itemRepo](APIManager::ContractItemList &&data, const QString &error) {
                    --pendingContractItemRequests;

                    if (error.isEmpty())
                    {
                        contractItems.reserve(contractItems.size() + data.size());
                        contractItems.insert(std::end(contractItems),
                                             std::make_move_iterator(std::begin(data)),
                                             std::make_move_iterator(std::end(data)));
                    }

                    if (pendingContractItemRequests == 0)
                    {
                        asyncBatchStore(itemRepo, contractItems, true);
                        contractItems.clear();

                        emit (this->*Signal)();
                    }

                    emit taskEnded(subTask, error);
                });
            }

            if (pendingContractItemRequests == 0)
            {
                emit(this->*Signal)();
                emit taskEnded(task, QString{});
            }
        }
    }

    template<void (EvernusApplication::* Signal)(), class Key>
    void EvernusApplication
    ::doRefreshMarketOrdersFromAPI(const Key &key, Character::IdType id, uint task, TimerType type)
    {
        markImport(id, type);
        mAPIManager.fetchMarketOrders(key, id, [=](MarketOrders &&data, const QString &error) {
            unmarkImport(id, type);

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
        asyncExecute(std::bind(&T::template batchStore<Data>, &repo, std::cref(data), hasId, true));
    }

    template<class Func, class... Args>
    void EvernusApplication::asyncExecute(Func &&func, Args && ...args)
    {
        qDebug() << "Starting async task...";

        auto future = std::async(std::launch::async, std::forward<Func>(func), std::forward<Args>(args)...);
        while (future.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
            processEvents(QEventLoop::ExcludeUserInputEvents);

        future.get();
        qDebug() << "Done.";
    }

    void EvernusApplication::showSplashMessage(const QString &message, QSplashScreen &splash)
    {
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignRight, Qt::white);
        processEvents();
    }

    QString EvernusApplication::getCharacterImportMessage(Character::IdType id)
    {
        return tr("Fetching character %1...").arg(id);
    }

    void EvernusApplication::setProxySettings()
    {
        QSettings settings;
        if (settings.value(NetworkSettings::useProxyKey, NetworkSettings::useProxyDefault).toBool())
        {
            SimpleCrypt crypt{NetworkSettings::cryptKey};

            const auto proxyType = settings.value(NetworkSettings::proxyTypeKey).toInt();
            const auto proxyHost = settings.value(NetworkSettings::proxyHostKey).toString();
            const quint16 proxyPort = settings.value(NetworkSettings::proxyPortKey).toUInt();
            const auto proxyUser = settings.value(NetworkSettings::proxyUserKey).toString();
            const auto proxyPassword = crypt.decryptToString(settings.value(NetworkSettings::proxyPasswordKey).toString());

            QNetworkProxy proxy{static_cast<QNetworkProxy::ProxyType>(proxyType), proxyHost, proxyPort, proxyUser, proxyPassword};
            QNetworkProxy::setApplicationProxy(proxy);
        }
        else
        {
            QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        }
    }

    bool EvernusApplication::shouldUseESIOverXML()
    {
        QSettings settings;
        return static_cast<ImportSettings::EveImportSource>(
            settings.value(
                ImportSettings::eveImportSourceKey,
                static_cast<int>(ImportSettings::eveImportSourceDefault)
            ).toInt()
        ) != ImportSettings::EveImportSource::XML;
    }
}
