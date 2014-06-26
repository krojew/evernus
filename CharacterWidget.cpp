#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>
#include <QFont>

#include "DatabaseUtils.h"
#include "Repository.h"

#include "CharacterWidget.h"

namespace Evernus
{
    const char * const CharacterWidget::skillFieldProperty = "field";

    CharacterWidget::CharacterWidget(const Repository<Character> &characterRepository, QWidget *parent)
        : QWidget{parent}
        , mCharacterRepository{characterRepository}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto infoGroup = new QGroupBox{tr("Character info"), this};
        mainLayout->addWidget(infoGroup);

        auto infoLayout = new QHBoxLayout{};
        infoGroup->setLayout(infoLayout);

        mPortrait = new QLabel{this};
        infoLayout->addWidget(mPortrait);
        mPortrait->setPixmap(QPixmap{":/images/generic-portrait.jpg"});

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

        auto standingsGroup = new QGroupBox{tr("Station owner standings"), this};
        mainLayout->addWidget(standingsGroup);

        auto standingsLayout = new QHBoxLayout{};
        standingsGroup->setLayout(standingsLayout);

        standingsLayout->addWidget(new QLabel{tr("Corporation standing:"), this}, 0, Qt::AlignVCenter | Qt::AlignRight);

        mCorpStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addWidget(mCorpStandingEdit);
        mCorpStandingEdit->setSingleStep(0.01);
        connect(mCorpStandingEdit, SIGNAL(valueChanged(double)), SLOT(setCorpStanding(double)));

        standingsLayout->addWidget(new QLabel{tr("Faction standing:"), this}, 0, Qt::AlignVCenter | Qt::AlignRight);

        mFactionStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addWidget(mFactionStandingEdit);
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

    void CharacterWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        qDebug() << "Switching character to" << mCharacterId;

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

        if (mCharacterId == Character::invalidId)
        {
            mNameLabel->setText(QString{});
            mBackgroundLabel->setText(QString{});
            mCorporationLabel->setText(QString{});
            mISKLabel->setText(QString{});
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
        }
        else
        {
            try
            {
                const auto character = mCharacterRepository.find(mCharacterId);

                mNameLabel->setText(character.getName());
                mBackgroundLabel->setText(QString{"%1 %2, %3, %4"}
                    .arg(character.getGender())
                    .arg(character.getRace())
                    .arg(character.getBloodline())
                    .arg(character.getAncestry()));
                mCorporationLabel->setText(character.getCorporationName());
                mISKLabel->setText(character.getISKPresentation());

                mCorpStandingEdit->setValue(character.getCorpStanding());
                mFactionStandingEdit->setValue(character.getFactionStanding());

                const auto orderAmountSkills = character.getOrderAmountSkills();
                const auto tradeRangeSkills = character.getTradeRangeSkills();
                const auto feeSkills = character.getFeeSkills();
                const auto contractSkills = character.getContractSkills();

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
        Q_ASSERT(mCharacterId != Character::invalidId);

        const auto fieldName = sender()->property(skillFieldProperty).toString();

        auto query = mCharacterRepository.prepare(QString{"UPDATE %1 SET %2 = :level WHERE %3 = :id"}
            .arg(mCharacterRepository.getTableName())
            .arg(fieldName)
            .arg(mCharacterRepository.getIdColumn()));
        query.bindValue(":level", level);
        query.bindValue(":id", mCharacterId);
        DatabaseUtils::execQuery(query);
    }

    void CharacterWidget::updateStanding(const QString &type, double value) const
    {
        Q_ASSERT(mCharacterId != Character::invalidId);

        auto query = mCharacterRepository.prepare(QString{"UPDATE %1 SET %2 = :standing WHERE %3 = :id"}
            .arg(mCharacterRepository.getTableName())
            .arg(type)
            .arg(mCharacterRepository.getIdColumn()));
        query.bindValue(":standing", value);
        query.bindValue(":id", mCharacterId);
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
}
