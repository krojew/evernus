#include "WalletSnapshot.h"

namespace Evernus
{
    WalletSnapshot::WalletSnapshot(const IdType &id, double balance)
        : Entity{id}
        , mBalance{balance}
    {
    }

    Character::IdType WalletSnapshot::getCharacterId() const noexcept
    {
        return mCharacterId;
    }

    void WalletSnapshot::setCharacterId(Character::IdType id) noexcept
    {
        mCharacterId = id;
    }

    double WalletSnapshot::getBalance() const noexcept
    {
        return mBalance;
    }

    void WalletSnapshot::setBalance(double balance) noexcept
    {
        mBalance = balance;
    }
}
