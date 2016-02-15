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
#include <QStringBuilder>
#include <QStandardPaths>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QListWidget>
#include <QSettings>
#include <QGroupBox>
#include <QSpinBox>
#include <QAction>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QUrl>

#include "MarketOrderRepository.h"
#include "CharacterRepository.h"
#include "CacheTimerProvider.h"
#include "CorpKeyRepository.h"
#include "WarningBarWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"
#include "TextUtils.h"

#include "CharacterWidget.h"

namespace Evernus
{
    const char * const CharacterWidget::skillFieldProperty = "field";
    const char * const CharacterWidget::downloadIdProperty = "downloadId";

    const QString CharacterWidget::defaultPortrait = ":/images/generic-portrait.jpg";

    CharacterWidget::CharacterWidget(const CharacterRepository &characterRepository,
                                     const MarketOrderRepository &marketOrderRepository,
                                     const CorpKeyRepository &corpKeyRepository,
                                     const CacheTimerProvider &cacheTimerProvider,
                                     QWidget *parent)
        : CharacterBoundWidget(std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::Character),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::Character),
                               ImportSettings::maxCharacterAgeKey,
                               parent)
        , mCharacterRepository(characterRepository)
        , mMarketOrderRepository(marketOrderRepository)
        , mCorpKeyRepository(corpKeyRepository)
        , mCacheTimerProvider(cacheTimerProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);

        auto characterLayout = new QHBoxLayout{};
        mainLayout->addLayout(characterLayout);

        auto infoGroup = new QGroupBox{tr("Character info"), this};
        characterLayout->addWidget(infoGroup, 1);

        auto infoLayout = new QHBoxLayout{infoGroup};

        auto downloadPortraitAction = new QAction{tr("Download portrait"), this};
        connect(downloadPortraitAction, &QAction::triggered, this, &CharacterWidget::downloadPortrait);

        mPortrait = new QLabel{this};
        infoLayout->addWidget(mPortrait);
        mPortrait->setPixmap(defaultPortrait);
        mPortrait->setContextMenuPolicy(Qt::ActionsContextMenu);
        mPortrait->addAction(downloadPortraitAction);

        auto backgroundLayout = new QVBoxLayout{};
        infoLayout->addLayout(backgroundLayout, 1);

        mNameLabel = new QLabel{this};
        backgroundLayout->addWidget(mNameLabel);

        QFont font;
        font.setBold(true);
        font.setPointSize(20);

        mNameLabel->setFont(font);

        mBackgroundLabel = new QLabel{this};
        backgroundLayout->addWidget(mBackgroundLabel);

        mCorporationLabel = new QLabel{this};
        backgroundLayout->addWidget(mCorporationLabel);

        mISKLabel = new QLabel{this};
        backgroundLayout->addWidget(mISKLabel);

        mUpdateTimersGroup = new QGroupBox{tr("Data age"), this};
        characterLayout->addWidget(mUpdateTimersGroup);
        mUpdateTimersGroup->setVisible(false);

        auto timersLayout = new QVBoxLayout{mUpdateTimersGroup};

        mUpdateTimersList = new QListWidget{this};
        timersLayout->addWidget(mUpdateTimersList, 0, Qt::AlignTop | Qt::AlignHCenter);
        mUpdateTimersList->setMaximumHeight(100);

        auto importAllBtn = new QPushButton{QIcon{":/images/arrow_refresh.png"}, tr("Import all"), this};
        timersLayout->addWidget(importAllBtn);
        connect(importAllBtn, &QPushButton::clicked, this, &CharacterWidget::importAll);

        auto underInfoLayout = new QHBoxLayout{};
        mainLayout->addLayout(underInfoLayout);

        auto addInfoGroup = new QGroupBox{tr("Orders"), this};
        underInfoLayout->addWidget(addInfoGroup);

        auto addInfoLayout = new QGridLayout{addInfoGroup};

        addInfoLayout->addWidget(new QLabel{tr("Buy orders:"), this}, 0, 0, Qt::AlignRight);

        mBuyOrderCountLabel = new QLabel{this};
        addInfoLayout->addWidget(mBuyOrderCountLabel, 0, 1, Qt::AlignLeft);

        addInfoLayout->addWidget(new QLabel{tr("Volume:"), this}, 0, 2, Qt::AlignRight);

        mBuyOrderVolumeLabel = new QLabel{this};
        addInfoLayout->addWidget(mBuyOrderVolumeLabel, 0, 3, Qt::AlignLeft);

        mBuyOrderValueLabel = new QLabel{this};
        addInfoLayout->addWidget(mBuyOrderValueLabel, 0, 4);

        addInfoLayout->addWidget(new QLabel{tr("Sell orders:"), this}, 1, 0, Qt::AlignRight);

        mSellOrderCountLabel = new QLabel{this};
        addInfoLayout->addWidget(mSellOrderCountLabel, 1, 1, Qt::AlignLeft);

        addInfoLayout->addWidget(new QLabel{tr("Volume:"), this}, 1, 2, Qt::AlignRight);

        mSellOrderVolumeLabel = new QLabel{this};
        addInfoLayout->addWidget(mSellOrderVolumeLabel, 1, 3, Qt::AlignLeft);

        mSellOrderValueLabel = new QLabel{this};
        addInfoLayout->addWidget(mSellOrderValueLabel, 1, 4);

        addInfoLayout->addWidget(new QLabel{tr("Total:"), this}, 2, 0, Qt::AlignRight);

        mTotalOrderCountLabel = new QLabel{this};
        addInfoLayout->addWidget(mTotalOrderCountLabel, 2, 1, Qt::AlignLeft);

        addInfoLayout->addWidget(new QLabel{tr("Volume:"), this}, 2, 2, Qt::AlignRight);

        mTotalOrderVolumeLabel = new QLabel{this};
        addInfoLayout->addWidget(mTotalOrderVolumeLabel, 2, 3, Qt::AlignLeft);

        mTotalOrderValueLabel = new QLabel{this};
        addInfoLayout->addWidget(mTotalOrderValueLabel, 2, 4);

        auto standingsGroup = new QGroupBox{tr("Station owner standings"), this};
        underInfoLayout->addWidget(standingsGroup);

        auto standingsLayout = new QFormLayout{standingsGroup};

        mCorpStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addRow(tr("Corporation standing:"), mCorpStandingEdit);
        mCorpStandingEdit->setMinimum(-10.0);
        mCorpStandingEdit->setMaximum(10.0);
        mCorpStandingEdit->setSingleStep(0.01);
        connect(mCorpStandingEdit, SIGNAL(valueChanged(double)), SLOT(setCorpStanding(double)));

        mFactionStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addRow(tr("Faction standing:"), mFactionStandingEdit);
        mFactionStandingEdit->setMinimum(-10.0);
        mFactionStandingEdit->setMaximum(10.0);
        mFactionStandingEdit->setSingleStep(0.01);
        connect(mFactionStandingEdit, SIGNAL(valueChanged(double)), SLOT(setFactionStanding(double)));

        auto skillsGroup = new QGroupBox{tr("Trade skills"), this};
        mainLayout->addWidget(skillsGroup);

        auto skillsLayout = new QHBoxLayout{skillsGroup};

        auto orderAmountGroup = new QGroupBox{tr("Order amount skills"), this};
        skillsLayout->addWidget(orderAmountGroup);

        auto orderAmountLayout = new QFormLayout{orderAmountGroup};

        orderAmountLayout->addRow(tr("Trade:"), createSkillEdit(mTradeSkillEdit, "trade_skill"));
        orderAmountLayout->addRow(tr("Retail:"), createSkillEdit(mRetailSkillEdit, "retail_skill"));
        orderAmountLayout->addRow(tr("Wholesale:"), createSkillEdit(mWholesaleSkillEdit, "wholesale_skill"));
        orderAmountLayout->addRow(tr("Tycoon:"), createSkillEdit(mTycoonSkillEdit, "tycoon_skill"));

        auto tradeRangeGroup = new QGroupBox{tr("Trade range skills"), this};
        skillsLayout->addWidget(tradeRangeGroup);

        auto tradeRangeLayout = new QFormLayout{tradeRangeGroup};

        tradeRangeLayout->addRow(tr("Marketing:"), createSkillEdit(mMarketingSkillEdit, "marketing_skill"));
        tradeRangeLayout->addRow(tr("Procurement:"), createSkillEdit(mProcurementSkillEdit, "procurement_skill"));
        tradeRangeLayout->addRow(tr("Daytrading:"), createSkillEdit(mDaytradingSkillEdit, "daytrading_skill"));
        tradeRangeLayout->addRow(tr("Visibility:"), createSkillEdit(mVisibilitySkillEdit, "visibility_skill"));

        auto feeGroup = new QGroupBox{tr("Fee skills"), this};
        skillsLayout->addWidget(feeGroup);

        auto feeLayout = new QFormLayout{feeGroup};

        feeLayout->addRow(tr("Accounting:"), createSkillEdit(mAccountingSkillEdit, "accounting_skill"));
        feeLayout->addRow(tr("Broker relations:"), createSkillEdit(mBrokerRelationsSkillEdit, "broker_relations_skill"));
        feeLayout->addRow(tr("Margin trading:"), createSkillEdit(mMarginTradingSkillEdit, "margin_trading_skill"));

        auto contractingGroup = new QGroupBox{tr("Contracting skills"), this};
        skillsLayout->addWidget(contractingGroup);

        auto contractingLayout = new QFormLayout{contractingGroup};

        contractingLayout->addRow(tr("Contracting:"), createSkillEdit(mContractingSkillEdit, "contracting_skill"));
        contractingLayout->addRow(tr("Corporation contracting:"),
                                  createSkillEdit(mCorporationContractingSkillEdit, "corporation_contracting_skill"));

        mainLayout->addStretch();

        connect(&mUpdateTimer, &QTimer::timeout, this, &CharacterWidget::updateTimerList);

        mUpdateTimer.start(1000 * 60);
    }

    void CharacterWidget::updateData()
    {
        refreshImportTimer();
        handleNewCharacter(getCharacterId());
    }

    void CharacterWidget::updateTimerList()
    {
        const auto id = getCharacterId();
        if (id == Character::invalidId)
        {
            mUpdateTimersGroup->hide();
            return;
        }

        const auto curLocale = locale();
        auto show = false;

        QSettings settings;
        const auto importContracts = settings.value(ImportSettings::importContractsKey, ImportSettings::importContractsDefault).toBool();

        mUpdateTimersList->clear();

        const auto checker = [&, this](TimerType timer, const QString &settingsKey, const QString &text) {
            const auto curTime
                = QDateTime::currentDateTime().addSecs(-60 * settings.value(settingsKey, Evernus::ImportSettings::importTimerDefault).toInt());
            const auto dt = mCacheTimerProvider.getLocalUpdateTimer(id, timer);

            if (dt < curTime)
            {
                show = true;
                new QListWidgetItem{QIcon{":/images/error.png"},
                                    text.arg((dt.isValid()) ? (TextUtils::dateTimeToString(dt, curLocale)) : (tr("never imported"))),
                                    mUpdateTimersList};
            }
        };

        checker(TimerType::Character, ImportSettings::maxCharacterAgeKey, tr("Character sheet: %1"));
        checker(TimerType::AssetList, ImportSettings::maxAssetListAgeKey, tr("Asset list: %1"));
        checker(TimerType::MarketOrders, ImportSettings::maxMarketOrdersAgeKey, tr("Market orders: %1"));
        checker(TimerType::WalletJournal, ImportSettings::maxWalletAgeKey, tr("Wallet journal: %1"));
        checker(TimerType::WalletTransactions, ImportSettings::maxWalletAgeKey, tr("Wallet transactions: %1"));

        if (importContracts)
            checker(TimerType::Contracts, ImportSettings::maxWalletAgeKey, tr("Contracts: %1"));

        try
        {
            mCorpKeyRepository.fetchForCharacter(id);

            checker(TimerType::CorpAssetList, ImportSettings::maxAssetListAgeKey, tr("Corp. asset list: %1"));
            checker(TimerType::CorpMarketOrders, ImportSettings::maxMarketOrdersAgeKey, tr("Corp. market orders: %1"));
            checker(TimerType::CorpWalletJournal, ImportSettings::maxWalletAgeKey, tr("Corp. wallet journal: %1"));
            checker(TimerType::CorpWalletTransactions, ImportSettings::maxWalletAgeKey, tr("Corp. wallet transactions: %1"));

            if (importContracts)
                checker(TimerType::CorpContracts, ImportSettings::maxWalletAgeKey, tr("Corp. contracts: %1"));
        }
        catch (const CorpKeyRepository::NotFoundException &)
        {
        }

        mUpdateTimersGroup->setVisible(show);
    }

    void CharacterWidget::updateMarketData()
    {
        try
        {
            const auto character = mCharacterRepository.find(getCharacterId());
            updateCharacterMarketData(character->getOrderAmountSkills());
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }

        updateTimerList();
    }

    void CharacterWidget::setCorpStanding(double value)
    {
        updateStanding("corp_standing", value);
    }

    void CharacterWidget::setFactionStanding(double value)
    {
        updateStanding("faction_standing", value);
    }

    void CharacterWidget::setSkillLevel(int level)
    {
        const auto id = getCharacterId();

        Q_ASSERT(id != Character::invalidId);

        const auto fieldName = sender()->property(skillFieldProperty).toString();
        mCharacterRepository.updateSkill(id, fieldName, level);
    }

    void CharacterWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching character to" << id;

        mCorpStandingEdit->blockSignals(true);
        mFactionStandingEdit->blockSignals(true);
        mTradeSkillEdit->blockSignals(true);
        mRetailSkillEdit->blockSignals(true);
        mWholesaleSkillEdit->blockSignals(true);
        mTycoonSkillEdit->blockSignals(true);
        mMarketingSkillEdit->blockSignals(true);
        mProcurementSkillEdit->blockSignals(true);
        mDaytradingSkillEdit->blockSignals(true);
        mVisibilitySkillEdit->blockSignals(true);
        mAccountingSkillEdit->blockSignals(true);
        mBrokerRelationsSkillEdit->blockSignals(true);
        mMarginTradingSkillEdit->blockSignals(true);
        mContractingSkillEdit->blockSignals(true);
        mCorporationContractingSkillEdit->blockSignals(true);

        if (id == Character::invalidId)
        {
            mNameLabel->setText(QString{});
            mBackgroundLabel->setText(QString{});
            mCorporationLabel->setText(QString{});
            mISKLabel->setText(QString{});

            mBuyOrderCountLabel->clear();
            mSellOrderCountLabel->clear();
            mTotalOrderCountLabel->clear();

            mBuyOrderValueLabel->clear();
            mSellOrderValueLabel->clear();
            mTotalOrderValueLabel->clear();

            mBuyOrderVolumeLabel->clear();
            mSellOrderVolumeLabel->clear();
            mTotalOrderVolumeLabel->clear();

            mCorpStandingEdit->setValue(0.);
            mFactionStandingEdit->setValue(0.);

            mTradeSkillEdit->setValue(0);
            mRetailSkillEdit->setValue(0);
            mWholesaleSkillEdit->setValue(0);
            mTycoonSkillEdit->setValue(0);
            mMarketingSkillEdit->setValue(0);
            mProcurementSkillEdit->setValue(0);
            mDaytradingSkillEdit->setValue(0);
            mVisibilitySkillEdit->setValue(0);
            mAccountingSkillEdit->setValue(0);
            mBrokerRelationsSkillEdit->setValue(0);
            mMarginTradingSkillEdit->setValue(0);
            mContractingSkillEdit->setValue(0);
            mCorporationContractingSkillEdit->setValue(0);

            mPortrait->setPixmap(defaultPortrait);
        }
        else
        {
            try
            {
                const auto character = mCharacterRepository.find(id);

                const auto orderAmountSkills = character->getOrderAmountSkills();
                const auto tradeRangeSkills = character->getTradeRangeSkills();
                const auto feeSkills = character->getFeeSkills();
                const auto contractSkills = character->getContractSkills();

                mNameLabel->setText(character->getName());
                mBackgroundLabel->setText(QString{"%1 %2, %3, %4"}
                    .arg(character->getGender())
                    .arg(character->getRace())
                    .arg(character->getBloodline())
                    .arg(character->getAncestry()));
                mCorporationLabel->setText(character->getCorporationName());
                mISKLabel->setText(character->getISKPresentation());

                updateCharacterMarketData(orderAmountSkills);

                mCorpStandingEdit->setValue(character->getCorpStanding());
                mFactionStandingEdit->setValue(character->getFactionStanding());

                mTradeSkillEdit->setValue(orderAmountSkills.mTrade);
                mRetailSkillEdit->setValue(orderAmountSkills.mRetail);
                mWholesaleSkillEdit->setValue(orderAmountSkills.mWholesale);
                mTycoonSkillEdit->setValue(orderAmountSkills.mTycoon);
                mMarketingSkillEdit->setValue(tradeRangeSkills.mMarketing);
                mProcurementSkillEdit->setValue(tradeRangeSkills.mProcurement);
                mDaytradingSkillEdit->setValue(tradeRangeSkills.mDaytrading);
                mVisibilitySkillEdit->setValue(tradeRangeSkills.mVisibility);
                mAccountingSkillEdit->setValue(feeSkills.mAccounting);
                mBrokerRelationsSkillEdit->setValue(feeSkills.mBrokerRelations);
                mMarginTradingSkillEdit->setValue(feeSkills.mMarginTrading);
                mContractingSkillEdit->setValue(contractSkills.mContracting);
                mCorporationContractingSkillEdit->setValue(contractSkills.mCorporationContracting);

                QSettings settings;
                if (settings.value(ImportSettings::importPortraitKey, ImportSettings::importPortraitDefault).toBool())
                {
                    const auto portrait = getPortraitPixmap(id);

                    if (!portrait.isNull() && portrait.devicePixelRatio() >= devicePixelRatio())
                        mPortrait->setPixmap(portrait);
                    else
                        downloadPortrait();
                }
                else
                {
                    mPortrait->setPixmap(defaultPortrait);
                }
            }
            catch (const Repository<Character>::NotFoundException &)
            {
                QMessageBox::warning(this, tr("Character error"), tr("Character not found in DB. Refresh characters."));
            }
        }

        updateTimerList();

        mCorpStandingEdit->blockSignals(false);
        mFactionStandingEdit->blockSignals(false);
        mTradeSkillEdit->blockSignals(false);
        mRetailSkillEdit->blockSignals(false);
        mWholesaleSkillEdit->blockSignals(false);
        mTycoonSkillEdit->blockSignals(false);
        mMarketingSkillEdit->blockSignals(false);
        mProcurementSkillEdit->blockSignals(false);
        mDaytradingSkillEdit->blockSignals(false);
        mVisibilitySkillEdit->blockSignals(false);
        mAccountingSkillEdit->blockSignals(false);
        mBrokerRelationsSkillEdit->blockSignals(false);
        mMarginTradingSkillEdit->blockSignals(false);
        mContractingSkillEdit->blockSignals(false);
        mCorporationContractingSkillEdit->blockSignals(false);
    }

    void CharacterWidget::downloadPortrait()
    {
        const auto id = getCharacterId();
        if (mPortraitDownloads.find(id) == std::end(mPortraitDownloads))
        {
            try
            {
                const auto savePath = devicePixelRatio() >= 2 ? getPortraitHighResPath(id) : getPortraitPath(id);
                const auto resolution = devicePixelRatio() >= 2 ? 256 : 128;
                auto download = new FileDownload{QUrl{QString{"https://image.eveonline.com/Character/%1_%2.jpg"}
                                                      .arg(id).arg(resolution)},
                                                 savePath,
                                                 this};
                download->setProperty(downloadIdProperty, id);
                connect(download, &FileDownload::finished, this, &CharacterWidget::downloadFinished);

                mPortraitDownloads.emplace(id, download);
            }
            catch (const std::exception &e)
            {
                qWarning() << e.what();
            }
        }
    }

    void CharacterWidget::downloadFinished()
    {
        const auto id = sender()->property(downloadIdProperty).value<Character::IdType>();
        const auto it = mPortraitDownloads.find(id);
        Q_ASSERT(it != std::end(mPortraitDownloads));

        const auto portrait = getPortraitPixmap(id);
        if (!portrait.isNull() && getCharacterId() == id)
            mPortrait->setPixmap(portrait);

        mPortraitDownloads.erase(it);
    }

    void CharacterWidget::updateStanding(const QString &type, double value) const
    {
        const auto id = getCharacterId();
        Q_ASSERT(id != Character::invalidId);

        mCharacterRepository.updateStanding(id, type, value);
    }

    void CharacterWidget::updateCharacterMarketData(const CharacterData::OrderAmountSkills &orderAmountSkills)
    {
        const auto aggrData = mMarketOrderRepository.getAggregatedData(getCharacterId());
        const auto curLocale = locale();

        const auto maxBuyOrders = orderAmountSkills.mTrade * 4 +
                                  orderAmountSkills.mRetail * 8 +
                                  orderAmountSkills.mWholesale * 16 +
                                  orderAmountSkills.mTycoon * 32 + 5;

        mBuyOrderCountLabel->setText(curLocale.toString(aggrData.mBuyData.mCount));
        mSellOrderCountLabel->setText(curLocale.toString(aggrData.mSellData.mCount));
        mTotalOrderCountLabel->setText(tr("<strong>%1 of %2</strong>")
            .arg(curLocale.toString(aggrData.mBuyData.mCount + aggrData.mSellData.mCount))
            .arg(curLocale.toString(maxBuyOrders)));

        mBuyOrderVolumeLabel->setText(curLocale.toString(aggrData.mBuyData.mVolume));
        mSellOrderVolumeLabel->setText(curLocale.toString(aggrData.mSellData.mVolume));
        mTotalOrderVolumeLabel->setText(curLocale.toString(aggrData.mBuyData.mVolume + aggrData.mSellData.mVolume));

        mBuyOrderValueLabel->setText(TextUtils::currencyToString(aggrData.mBuyData.mPriceSum, curLocale));
        mSellOrderValueLabel->setText(TextUtils::currencyToString(aggrData.mSellData.mPriceSum, curLocale));
        mTotalOrderValueLabel->setText(tr("<strong>%1</strong>")
            .arg(TextUtils::currencyToString(aggrData.mBuyData.mPriceSum + aggrData.mSellData.mPriceSum, curLocale)));
    }

    QSpinBox *CharacterWidget::createSkillEdit(QSpinBox *&target, const QString &skillField)
    {
        target = new QSpinBox{this};
        target->setMaximum(5);
        target->setProperty(skillFieldProperty, skillField);
        connect(target, SIGNAL(valueChanged(int)), SLOT(setSkillLevel(int)));

        return target;
    }

    QPixmap CharacterWidget::getPortraitPixmap(Character::IdType id, int size)
    {
        return QIcon{getPortraitPath(id)}.pixmap(size);
    }

    QString CharacterWidget::getPortraitPath(Character::IdType id)
    {
        return getPortraitLowResPath(id);
    }

    QString CharacterWidget::getPortraitLowResPath(Character::IdType id)
    {
        return getPortraitPathTemplate(id).arg("");
    }

    QString CharacterWidget::getPortraitHighResPath(Character::IdType id)
    {
        return getPortraitPathTemplate(id).arg("@2x");
    }

    QString CharacterWidget::getPortraitPathTemplate(Character::IdType id)
    {
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) %
            "/portrait/" %
            QString::number(id) %
            "%1.jpg";
    }
}
