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
#pragma once

#include <QDateTime>

#include "Character.h"
#include "EveType.h"
#include "Entity.h"

namespace Evernus
{
    class MarketOrder
        : public Entity<quint64>
    {
    public:
        enum class State
        {
            Active,
            Closed,
            Fulfilled,
            Cancelled,
            Pending,
            CharacterDeleted,

            Archieved = 100
        };

        enum class Type
        {
            Buy,
            Sell
        };

        static const auto characterAccountKey = 1000;

        using Entity::Entity;

        MarketOrder() = default;
        MarketOrder(const MarketOrder &) = default;
        MarketOrder(MarketOrder &&) = default;
        virtual ~MarketOrder() = default;

        Character::IdType getCharacterId() const;
        void setCharacterId(Character::IdType id);

        quint64 getLocationId() const noexcept;
        void setLocationId(quint64 id) noexcept;

        uint getVolumeEntered() const noexcept;
        void setVolumeEntered(uint value) noexcept;

        uint getVolumeRemaining() const noexcept;
        void setVolumeRemaining(uint value) noexcept;

        uint getMinVolume() const noexcept;
        void setMinVolume(uint value) noexcept;

        uint getDelta() const noexcept;
        void setDelta(uint value) noexcept;

        State getState() const noexcept;
        void setState(State state) noexcept;

        EveType::IdType getTypeId() const;
        void setTypeId(EveType::IdType id);

        short getRange() const noexcept;
        void setRange(short value) noexcept;

        short getAccountKey() const noexcept;
        void setAccountKey(short key) noexcept;

        short getDuration() const noexcept;
        void setDuration(short value) noexcept;

        double getEscrow() const noexcept;
        void setEscrow(double value) noexcept;

        double getPrice() const noexcept;
        void setPrice(double value) noexcept;

        Type getType() const noexcept;
        void setType(Type type) noexcept;

        QDateTime getIssued() const;
        void setIssued(const QDateTime &dt);

        QDateTime getFirstSeen() const;
        void setFirstSeen(const QDateTime &dt);

        MarketOrder &operator =(const MarketOrder &) = default;
        MarketOrder &operator =(MarketOrder &&) = default;

    private:
        Character::IdType mCharacterId = Character::invalidId;
        quint64 mLocationId = 0;
        uint mVolumeEntered = 0;
        uint mVolumeRemaining = 0;
        uint mMinVolume = 0;
        uint mDelta = 0;
        State mState = State::Active;
        EveType::IdType mTypeId = EveType::invalidId;
        short mRange = 0;
        short mAccountKey = characterAccountKey;
        short mDuration = 0;
        double mEscrow = 0.;
        double mPrice = 0.;
        Type mType = Type::Buy;
        QDateTime mIssued;
        QDateTime mFirstSeen;
    };
}
