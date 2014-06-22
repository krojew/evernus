#include "Character.h"

namespace Evernus
{
    QString Character::getName() const &
    {
        return mName;
    }

    QString &&Character::getName() && noexcept
    {
        return std::move(mName);
    }

    void Character::setName(const QString &name)
    {
        mName = name;
    }

    void Character::setName(QString &&name)
    {
        mName = std::move(name);
    }

    Character::KeyIdType Character::getKeyId() const noexcept
    {
        return mKeyId;
    }

    void Character::setKeyId(Key::IdType id)
    {
        mKeyId = id;
    }

    QString Character::getCorporationName() const &
    {
        return mCorporationName;
    }

    QString &&Character::getCorporationName() && noexcept
    {
        return std::move(mCorporationName);
    }

    void Character::setCorporationName(const QString &name)
    {
        mCorporationName = name;
    }

    void Character::setCorporationName(QString &&name)
    {
        mCorporationName = std::move(name);
    }

    QString Character::getRace() const &
    {
        return mRace;
    }

    QString &&Character::getRace() && noexcept
    {
        return std::move(mRace);
    }

    void Character::setRace(const QString &race)
    {
        mRace = race;
    }

    void Character::setRace(QString &&race)
    {
        mRace = std::move(race);
    }

    QString Character::getBloodline() const &
    {
        return mBloodline;
    }

    QString &&Character::getBloodline() && noexcept
    {
        return std::move(mBloodline);
    }

    void Character::setBloodline(const QString &bloodline)
    {
        mBloodline = bloodline;
    }

    void Character::setBloodline(QString &&bloodline)
    {
        mBloodline = std::move(bloodline);
    }

    QString Character::getAncestry() const &
    {
        return mAncestry;
    }

    QString &&Character::getAncestry() && noexcept
    {
        return std::move(mAncestry);
    }

    void Character::setAncestry(const QString &ancestry)
    {
        mAncestry = ancestry;
    }

    void Character::setAncestry(QString &&ancestry)
    {
        mAncestry = std::move(ancestry);
    }

    QString Character::getGender() const &
    {
        return mGender;
    }

    QString &&Character::getGender() && noexcept
    {
        return std::move(mGender);
    }

    void Character::setGender(const QString &gender)
    {
        mGender = gender;
    }

    void Character::setGender(QString &&gender)
    {
        mGender = std::move(gender);
    }

    uint Character::getSkillPoints() const noexcept
    {
        return mSkillPoints;
    }

    void Character::setSkillPoints(uint points) noexcept
    {
        mSkillPoints = points;
    }

    Character::ISKType Character::getISK() const
    {
        return mISK;
    }

    void Character::setISK(ISKType isk)
    {
        mISK = isk;
    }

    float Character::getCorpStanding() const noexcept
    {
        return mCorpStanding;
    }

    void Character::setCorpStanding(float standing) noexcept
    {
        mCorpStanding = standing;
    }

    float Character::getFactionStanding() const noexcept
    {
        return mFactionStanding;
    }

    void Character::setFactionStanding(float standing) noexcept
    {
        mFactionStanding = standing;
    }

    Character::OrderAmountSkills Character::getOrderAmountSkills() const noexcept
    {
        return mOrderAmountSkills;
    }

    void Character::setOrderAmountSkills(const OrderAmountSkills &skills) noexcept
    {
        mOrderAmountSkills = skills;
    }

    void Character::setOrderAmountSkills(OrderAmountSkills &&skills) noexcept
    {
        mOrderAmountSkills = std::move(skills);
    }

    Character::TradeRangeSkills Character::getTradeRangeSkills() const noexcept
    {
        return mTradeRangeSkills;
    }

    void Character::setTradeRangeSkills(const TradeRangeSkills &skills) noexcept
    {
        mTradeRangeSkills = skills;
    }

    void Character::setTradeRangeSkills(TradeRangeSkills &&skills) noexcept
    {
        mTradeRangeSkills = std::move(skills);
    }

    Character::FeeSkills Character::getFeeSkills() const noexcept
    {
        return mFeeSkills;
    }

    void Character::setFeeSkills(const FeeSkills &skills) noexcept
    {
        mFeeSkills = skills;
    }

    void Character::setFeeSkills(FeeSkills &&skills) noexcept
    {
        mFeeSkills = std::move(skills);
    }

    Character::ContractSkills Character::getContractSkills() const noexcept
    {
        return mContractSkills;
    }

    void Character::setContractSkills(const ContractSkills &skills) noexcept
    {
        mContractSkills = skills;
    }

    void Character::setContractSkills(ContractSkills &&skills) noexcept
    {
        mContractSkills = std::move(skills);
    }

    bool Character::isEnabled() const noexcept
    {
        return mEnabled;
    }

    void Character::setEnabled(bool flag) noexcept
    {
        mEnabled = flag;
    }
}
