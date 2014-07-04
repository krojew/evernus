#pragma once

#include <QDateTime>

#include "Character.h"
#include "Entity.h"

namespace Evernus
{
    class WalletSnapshot
        : public Entity<QDateTime>
    {
    public:
        using Entity::Entity;

        WalletSnapshot(const IdType &id, double balance);
        virtual ~WalletSnapshot() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        double getBalance() const noexcept;
        void setBalance(double balance) noexcept;

    private:
        Character::IdType mCharacterId = Character::invalidId;
        double mBalance = 0.;
    };
}
