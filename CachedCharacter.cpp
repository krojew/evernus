#include "CachedCharacter.h"

namespace Evernus
{
    QString CachedCharacter::getName() const &
    {
        return mData.mName;
    }

    QString &&CachedCharacter::getName() && noexcept
    {
        return std::move(mData.mName);
    }

    void CachedCharacter::setName(const QString &name)
    {
        mData.mName = name;
    }

    void CachedCharacter::setName(QString &&name)
    {
        mData.mName = std::move(name);
    }

    QString CachedCharacter::getCorporationName() const &
    {
        return mData.mCorporationName;
    }

    QString &&CachedCharacter::getCorporationName() && noexcept
    {
        return std::move(mData.mCorporationName);
    }

    void CachedCharacter::setCorporationName(const QString &name)
    {
        mData.mCorporationName = name;
    }

    void CachedCharacter::setCorporationName(QString &&name)
    {
        mData.mCorporationName = std::move(name);
    }

    QString CachedCharacter::getRace() const &
    {
        return mData.mRace;
    }

    QString &&CachedCharacter::getRace() && noexcept
    {
        return std::move(mData.mRace);
    }

    void CachedCharacter::setRace(const QString &race)
    {
        mData.mRace = race;
    }

    void CachedCharacter::setRace(QString &&race)
    {
        mData.mRace = std::move(race);
    }

    QString CachedCharacter::getBloodline() const &
    {
        return mData.mBloodline;
    }

    QString &&CachedCharacter::getBloodline() && noexcept
    {
        return std::move(mData.mBloodline);
    }

    void CachedCharacter::setBloodline(const QString &bloodline)
    {
        mData.mBloodline = bloodline;
    }

    void CachedCharacter::setBloodline(QString &&bloodline)
    {
        mData.mBloodline = std::move(bloodline);
    }

    QString CachedCharacter::getAncestry() const &
    {
        return mData.mAncestry;
    }

    QString &&CachedCharacter::getAncestry() && noexcept
    {
        return std::move(mData.mAncestry);
    }

    void CachedCharacter::setAncestry(const QString &ancestry)
    {
        mData.mAncestry = ancestry;
    }

    void CachedCharacter::setAncestry(QString &&ancestry)
    {
        mData.mAncestry = std::move(ancestry);
    }

    QString CachedCharacter::getGender() const &
    {
        return mData.mGender;
    }

    QString &&CachedCharacter::getGender() && noexcept
    {
        return std::move(mData.mGender);
    }

    void CachedCharacter::setGender(const QString &gender)
    {
        mData.mGender = gender;
    }

    void CachedCharacter::setGender(QString &&gender)
    {
        mData.mGender = std::move(gender);
    }

    CharacterData::ISKType CachedCharacter::getISK() const
    {
        return mData.mISK;
    }

    void CachedCharacter::setISK(CharacterData::ISKType isk)
    {
        mData.mISK = isk;
    }

    float CachedCharacter::getCorpStanding() const noexcept
    {
        return mData.mCorpStanding;
    }

    void CachedCharacter::setCorpStanding(float standing) noexcept
    {
        mData.mCorpStanding = standing;
    }

    float CachedCharacter::getFactionStanding() const noexcept
    {
        return mData.mFactionStanding;
    }

    void CachedCharacter::setFactionStanding(float standing) noexcept
    {
        mData.mFactionStanding = standing;
    }

    CharacterData::OrderAmountSkills CachedCharacter::getOrderAmountSkills() const noexcept
    {
        return mData.mOrderAmountSkills;
    }

    void CachedCharacter::setOrderAmountSkills(const CharacterData::OrderAmountSkills &skills) noexcept
    {
        mData.mOrderAmountSkills = skills;
    }

    void CachedCharacter::setOrderAmountSkills(CharacterData::OrderAmountSkills &&skills) noexcept
    {
        mData.mOrderAmountSkills = std::move(skills);
    }

    CharacterData::TradeRangeSkills CachedCharacter::getTradeRangeSkills() const noexcept
    {
        return mData.mTradeRangeSkills;
    }

    void CachedCharacter::setTradeRangeSkills(const CharacterData::TradeRangeSkills &skills) noexcept
    {
        mData.mTradeRangeSkills = skills;
    }

    void CachedCharacter::setTradeRangeSkills(CharacterData::TradeRangeSkills &&skills) noexcept
    {
        mData.mTradeRangeSkills = std::move(skills);
    }

    CharacterData::FeeSkills CachedCharacter::getFeeSkills() const noexcept
    {
        return mData.mFeeSkills;
    }

    void CachedCharacter::setFeeSkills(const CharacterData::FeeSkills &skills) noexcept
    {
        mData.mFeeSkills = skills;
    }

    void CachedCharacter::setFeeSkills(CharacterData::FeeSkills &&skills) noexcept
    {
        mData.mFeeSkills = std::move(skills);
    }

    CharacterData::ContractSkills CachedCharacter::getContractSkills() const noexcept
    {
        return mData.mContractSkills;
    }

    void CachedCharacter::setContractSkills(const CharacterData::ContractSkills &skills) noexcept
    {
        mData.mContractSkills = skills;
    }

    void CachedCharacter::setContractSkills(CharacterData::ContractSkills &&skills) noexcept
    {
        mData.mContractSkills = std::move(skills);
    }

    CharacterData CachedCharacter::getCharacterData() const &
    {
        return mData;
    }

    CharacterData &&CachedCharacter::getCharacterData() && noexcept
    {
        return std::move(mData);
    }

    void CachedCharacter::setCharacterData(const CharacterData &data)
    {
        mData = data;
    }

    void CachedCharacter::setCharacterData(CharacterData &&data)
    {
        mData = std::move(data);
    }
}
