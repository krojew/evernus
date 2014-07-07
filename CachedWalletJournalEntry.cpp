#include "CachedWalletJournalEntry.h"

namespace Evernus
{
    Character::IdType CachedWalletJournalEntry::getCharacterId() const noexcept
    {
        return mData.mCharacterId;
    }

    void CachedWalletJournalEntry::setCharacterId(Character::IdType id) noexcept
    {
        mData.mCharacterId = id;
    }

    QDateTime CachedWalletJournalEntry::getTimestamp() const
    {
        return mData.mTimestamp;
    }

    void CachedWalletJournalEntry::setTimestamp(const QDateTime &dt)
    {
        mData.mTimestamp = dt;
    }

    uint CachedWalletJournalEntry::getRefTypeId() const noexcept
    {
        return mData.mRefTypeId;
    }

    void CachedWalletJournalEntry::setRefTypeId(uint id) noexcept
    {
        mData.mRefTypeId = id;
    }

    QString CachedWalletJournalEntry::getOwnerName1() const &
    {
        return mData.mOwnerName1;
    }

    QString &&CachedWalletJournalEntry::getOwnerName1() && noexcept
    {
        return std::move(mData.mOwnerName1);
    }

    void CachedWalletJournalEntry::setOwnerName1(const QString &name)
    {
        mData.mOwnerName1 = name;
    }

    void CachedWalletJournalEntry::setOwnerName1(QString &&name)
    {
        mData.mOwnerName1 = std::move(name);
    }

    quint64 CachedWalletJournalEntry::getOwnerId1() const noexcept
    {
        return mData.mOwnerId1;
    }

    void CachedWalletJournalEntry::setOwnerId1(quint64 id) noexcept
    {
        mData.mOwnerId1 = id;
    }

    QString CachedWalletJournalEntry::getOwnerName2() const &
    {
        return mData.mOwnerName2;
    }

    QString &&CachedWalletJournalEntry::getOwnerName2() && noexcept
    {
        return std::move(mData.mOwnerName2);
    }

    void CachedWalletJournalEntry::setOwnerName2(const QString &name)
    {
        mData.mOwnerName2 = name;
    }

    void CachedWalletJournalEntry::setOwnerName2(QString &&name)
    {
        mData.mOwnerName2 = std::move(name);
    }

    quint64 CachedWalletJournalEntry::getOwnerId2() const noexcept
    {
        return mData.mOwnerId2;
    }

    void CachedWalletJournalEntry::setOwnerId2(quint64 id) noexcept
    {
        mData.mOwnerId2 = id;
    }

    CachedWalletJournalEntry::ArgType CachedWalletJournalEntry::getArgName() const &
    {
        return mData.mArgName;
    }

    CachedWalletJournalEntry::ArgType &&CachedWalletJournalEntry::getArgName() && noexcept
    {
        return std::move(mData.mArgName);
    }

    void CachedWalletJournalEntry::setArgName(const ArgType &name)
    {
        mData.mArgName = name;
    }

    void CachedWalletJournalEntry::setArgName(ArgType &&name)
    {
        mData.mArgName = std::move(name);
    }

    quint64 CachedWalletJournalEntry::getArgId() const noexcept
    {
        return mData.mArgId;
    }

    void CachedWalletJournalEntry::setArgId(quint64 id) noexcept
    {
        mData.mArgId = id;
    }

    double CachedWalletJournalEntry::getAmount() const noexcept
    {
        return mData.mAmount;
    }

    void CachedWalletJournalEntry::setAmount(double value) noexcept
    {
        mData.mAmount = value;
    }

    double CachedWalletJournalEntry::getBalance() const noexcept
    {
        return mData.mBalance;
    }

    void CachedWalletJournalEntry::setBalance(double value) noexcept
    {
        mData.mBalance = value;
    }

    CachedWalletJournalEntry::ReasonType CachedWalletJournalEntry::getReason() const &
    {
        return mData.mReason;
    }

    CachedWalletJournalEntry::ReasonType &&CachedWalletJournalEntry::getReason() && noexcept
    {
        return std::move(mData.mReason);
    }

    void CachedWalletJournalEntry::setReason(const ReasonType &reason)
    {
        mData.mReason = reason;
    }

    void CachedWalletJournalEntry::setReason(ReasonType &&reason)
    {
        mData.mReason = std::move(reason);
    }

    CachedWalletJournalEntry::TaxReceiverType CachedWalletJournalEntry::getTaxReceiverId() const noexcept
    {
        return mData.mTaxReceiverId;
    }

    void CachedWalletJournalEntry::setTaxReceiverId(TaxReceiverType id) noexcept
    {
        mData.mTaxReceiverId = id;
    }

    CachedWalletJournalEntry::TaxAmountType CachedWalletJournalEntry::getTaxAmount() const noexcept
    {
        return mData.mTaxAmount;
    }

    void CachedWalletJournalEntry::setTaxAmount(TaxAmountType amount) noexcept
    {
        mData.mTaxAmount = amount;
    }
}
