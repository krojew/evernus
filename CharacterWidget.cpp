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
#include <QSettings>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QUrl>

#include "MarketOrderRepository.h"
#include "CacheTimerProvider.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"
#include "DatabaseUtils.h"
#include "Repository.h"

#include "CharacterWidget.h"

namespace Evernus
{
    const char * const CharacterWidget::skillFieldProperty = "field";
    const char * const CharacterWidget::downloadIdProperty = "downloadId";

    const QString CharacterWidget::defaultPortrait = ":/images/generic-portrait.jpg";

    CharacterWidget::CharacterWidget(const Repository<Character> &characterRepository,
                                     const MarketOrderRepository &marketOrderRepository,
                                     const CacheTimerProvider &cacheTimerProvider,
                                     QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::Character),
                               parent}
        , mCharacterRepository{characterRepository}
        , mMarketOrderRepository{marketOrderRepository}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();

        auto infoGroup = new QGroupBox{tr("Character info"), this};
        mainLayout->addWidget(infoGroup);

        auto infoLayout = new QHBoxLayout{};
        infoGroup->setLayout(infoLayout);

        mPortrait = new QLabel{this};
        infoLayout->addWidget(mPortrait);
        mPortrait->setPixmap(defaultPortrait);

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

        auto underInfoLayout = new QHBoxLayout{};
        mainLayout->addLayout(underInfoLayout);

        auto addInfoGroup = new QGroupBox{tr("Orders"), this};
        underInfoLayout->addWidget(addInfoGroup);

        auto addInfoLayout = new QGridLayout{};
        addInfoGroup->setLayout(addInfoLayout);

        addInfoLayout->addWidget(new QLabel{tr("Buy orders:"), this}, 0, 0);

        mBuyOrderCountLabel = new QLabel{this};
        addInfoLayout->addWidget(mBuyOrderCountLabel, 0, 1);

        mBuyOrderValueLabel = new QLabel{this};
        addInfoLayout->addWidget(mBuyOrderValueLabel, 0, 2);

        addInfoLayout->addWidget(new QLabel{tr("Sell orders:"), this}, 1, 0);

        mSellOrderCountLabel = new QLabel{this};
        addInfoLayout->addWidget(mSellOrderCountLabel, 1, 1);

        mSellOrderValueLabel = new QLabel{this};
        addInfoLayout->addWidget(mSellOrderValueLabel, 1, 2);

        addInfoLayout->addWidget(new QLabel{tr("Total:"), this}, 2, 0);

        mTotalOrderCountLabel = new QLabel{this};
        addInfoLayout->addWidget(mTotalOrderCountLabel, 2, 1);

        mTotalOrderValueLabel = new QLabel{this};
        addInfoLayout->addWidget(mTotalOrderValueLabel, 2, 2);

        auto standingsGroup = new QGroupBox{tr("Station owner standings"), this};
        underInfoLayout->addWidget(standingsGroup);

        auto standingsLayout = new QFormLayout{};
        standingsGroup->setLayout(standingsLayout);

        mCorpStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addRow(tr("Corporation standing:"), mCorpStandingEdit);
        mCorpStandingEdit->setSingleStep(0.01);
        connect(mCorpStandingEdit, SIGNAL(valueChanged(double)), SLOT(setCorpStanding(double)));

        mFactionStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addRow(tr("Faction standing:"), mFactionStandingEdit);
        mFactionStandingEdit->setSingleStep(0.01);
        connect(mFactionStandingEdit, SIGNAL(valueChanged(double)), SLOT(setFactionStanding(double)));

        auto skillsGroup = new QGroupBox{tr("Trade skills"), this};
        mainLayout->addWidget(skillsGroup);

        auto skillsLayout = new QHBoxLayout{};
        skillsGroup->setLayout(skillsLayout);

        auto orderAmountGroup = new QGroupBox{tr("Order amount skills"), this};
        skillsLayout->addWidget(orderAmountGroup);

        auto orderAmountLayout = new QFormLayout{};
        orderAmountGroup->setLayout(orderAmountLayout);

        orderAmountLayout->addRow(tr("Trade:"), createSkillEdit(mTradeSkillEdit, "trade_skill"));
        orderAmountLayout->addRow(tr("Retail:"), createSkillEdit(mRetailSkillEdit, "retail_skill"));
        orderAmountLayout->addRow(tr("Wholesale:"), createSkillEdit(mWholesaleSkillEdit, "wholesale_skill"));
        orderAmountLayout->addRow(tr("Tycoon:"), createSkillEdit(mTycoonSkillEdit, "tycoon_skill"));

        auto tradeRangeGroup = new QGroupBox{tr("Trade range skills"), this};
        skillsLayout->addWidget(tradeRangeGroup);

        auto tradeRangeLayout = new QFormLayout{};
        tradeRangeGroup->setLayout(tradeRangeLayout);

        tradeRangeLayout->addRow(tr("Marketing:"), createSkillEdit(mMarketingSkillEdit, "marketing_skill"));
        tradeRangeLayout->addRow(tr("Procurement:"), createSkillEdit(mProcurementSkillEdit, "procurement_skill"));
        tradeRangeLayout->addRow(tr("Daytrading:"), createSkillEdit(mDaytradingSkillEdit, "daytrading_skill"));
        tradeRangeLayout->addRow(tr("Visibility:"), createSkillEdit(mVisibilitySkillEdit, "visibility_skill"));

        auto feeGroup = new QGroupBox{tr("Fee skills"), this};
        skillsLayout->addWidget(feeGroup);

        auto feeLayout = new QFormLayout{};
        feeGroup->setLayout(feeLayout);

        feeLayout->addRow(tr("Accounting:"), createSkillEdit(mAccountingSkillEdit, "accounting_skill"));
        feeLayout->addRow(tr("Broker relations:"), createSkillEdit(mBrokerRelationsSkillEdit, "broker_relations_skill"));
        feeLayout->addRow(tr("Margin trading:"), createSkillEdit(mMarginTradingSkillEdit, "margin_trading_skill"));

        auto contractingGroup = new QGroupBox{tr("Contracting skills"), this};
        skillsLayout->addWidget(contractingGroup);

        auto contractingLayout = new QFormLayout{};
        contractingGroup->setLayout(contractingLayout);

        contractingLayout->addRow(tr("Contracting:"), createSkillEdit(mContractingSkillEdit, "contracting_skill"));
        contractingLayout->addRow(tr("Corporation contracting:"),
                                  createSkillEdit(mCorporationContractingSkillEdit, "corporation_contracting_skill"));

        mainLayout->addStretch();
    }

    void CharacterWidget::updateData()
    {
        refreshImportTimer();
        handleNewCharacter(getCharacterId());
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

        auto query = mCharacterRepository.prepare(QString{"UPDATE %1 SET %2 = :level WHERE %3 = :id"}
            .arg(mCharacterRepository.getTableName())
            .arg(fieldName)
            .arg(mCharacterRepository.getIdColumn()));
        query.bindValue(":level", level);
        query.bindValue(":id", id);
        DatabaseUtils::execQuery(query);
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

                const auto orderAmountSkills = character.getOrderAmountSkills();
                const auto tradeRangeSkills = character.getTradeRangeSkills();
                const auto feeSkills = character.getFeeSkills();
                const auto contractSkills = character.getContractSkills();

                mNameLabel->setText(character.getName());
                mBackgroundLabel->setText(QString{"%1 %2, %3, %4"}
                    .arg(character.getGender())
                    .arg(character.getRace())
                    .arg(character.getBloodline())
                    .arg(character.getAncestry()));
                mCorporationLabel->setText(character.getCorporationName());
                mISKLabel->setText(character.getISKPresentation());

                const auto aggrData = mMarketOrderRepository.getAggregatedData(id);
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

                mBuyOrderValueLabel->setText(curLocale.toCurrencyString(aggrData.mBuyData.mPriceSum, "ISK"));
                mSellOrderValueLabel->setText(curLocale.toCurrencyString(aggrData.mSellData.mPriceSum, "ISK"));
                mTotalOrderValueLabel->setText(tr("<strong>%1</strong>")
                    .arg(curLocale.toCurrencyString(aggrData.mBuyData.mPriceSum + aggrData.mSellData.mPriceSum, "ISK")));

                mCorpStandingEdit->setValue(character.getCorpStanding());
                mFactionStandingEdit->setValue(character.getFactionStanding());

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
                if (settings.value(ImportSettings::importPortraitKey, true).toBool())
                {
                    const auto portraitPath = getPortraitPath(id);

                    QFile portrait{portraitPath};
                    if (portrait.exists())
                    {
                        mPortrait->setPixmap(portraitPath);
                    }
                    else if (mPortraitDownloads.find(id) == std::end(mPortraitDownloads))
                    {
                        try
                        {
                            auto download = new FileDownload{QUrl{QString{"https://image.eveonline.com/Character/%1_128.jpg"}.arg(id)},
                                                             portraitPath,
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

    void CharacterWidget::downloadFinished()
    {
        const auto id = sender()->property(downloadIdProperty).value<Character::IdType>();
        const auto it = mPortraitDownloads.find(id);
        Q_ASSERT(it != std::end(mPortraitDownloads));

        QPixmap px{getPortraitPath(id)};
        if (!px.isNull())
            mPortrait->setPixmap(px);

        mPortraitDownloads.erase(it);
    }

    void CharacterWidget::updateStanding(const QString &type, double value) const
    {
        const auto id = getCharacterId();
        Q_ASSERT(id != Character::invalidId);

        auto query = mCharacterRepository.prepare(QString{"UPDATE %1 SET %2 = :standing WHERE %3 = :id"}
            .arg(mCharacterRepository.getTableName())
            .arg(type)
            .arg(mCharacterRepository.getIdColumn()));
        query.bindValue(":standing", value);
        query.bindValue(":id", id);
        DatabaseUtils::execQuery(query);
    }

    QSpinBox *CharacterWidget::createSkillEdit(QSpinBox *&target, const QString &skillField)
    {
        target = new QSpinBox{this};
        target->setMaximum(5);
        target->setProperty(skillFieldProperty, skillField);
        connect(target, SIGNAL(valueChanged(int)), SLOT(setSkillLevel(int)));

        return target;
    }

    QString CharacterWidget::getPortraitPath(Character::IdType id)
    {
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) %
            "/portrait/" %
            QString::number(id) %
            ".jpg";
    }
}
