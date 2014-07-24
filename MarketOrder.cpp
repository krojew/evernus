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
#include "MarketOrder.h"

namespace Evernus
{
    Character::IdType MarketOrder::getCharacterId() const
    {
        return mCharacterId;
    }

    void MarketOrder::setCharacterId(Character::IdType id)
    {
        mCharacterId = id;
    }

    quint64 MarketOrder::getLocationId() const noexcept
    {
        return mLocationId;
    }

    void MarketOrder::setLocationId(quint64 id) noexcept
    {
        mLocationId = id;
    }

    uint MarketOrder::getVolumeEntered() const noexcept
    {
        return mVolumeEntered;
    }

    void MarketOrder::setVolumeEntered(uint value) noexcept
    {
        mVolumeEntered = value;
    }

    uint MarketOrder::getVolumeRemaining() const noexcept
    {
        return mVolumeRemaining;
    }

    void MarketOrder::setVolumeRemaining(uint value) noexcept
    {
        mVolumeRemaining = value;
    }

    uint MarketOrder::getMinVolume() const noexcept
    {
        return mMinVolume;
    }

    void MarketOrder::setMinVolume(uint value) noexcept
    {
        mMinVolume = value;
    }

    int MarketOrder::getDelta() const noexcept
    {
        return mDelta;
    }

    void MarketOrder::setDelta(int value) noexcept
    {
        mDelta = value;
    }

    MarketOrder::State MarketOrder::getState() const noexcept
    {
        return mState;
    }

    void MarketOrder::setState(State state) noexcept
    {
        mState = state;
    }

    EveType::IdType MarketOrder::getTypeId() const
    {
        return mTypeId;
    }

    void MarketOrder::setTypeId(EveType::IdType id)
    {
        mTypeId = id;
    }

    short MarketOrder::getRange() const noexcept
    {
        return mRange;
    }

    void MarketOrder::setRange(short value) noexcept
    {
        mRange = value;
    }

    short MarketOrder::getAccountKey() const noexcept
    {
        return mAccountKey;
    }

    void MarketOrder::setAccountKey(short key) noexcept
    {
        mAccountKey = key;
    }

    short MarketOrder::getDuration() const noexcept
    {
        return mDuration;
    }

    void MarketOrder::setDuration(short value) noexcept
    {
        mDuration = value;
    }

    double MarketOrder::getEscrow() const noexcept
    {
        return mEscrow;
    }

    void MarketOrder::setEscrow(double value) noexcept
    {
        mEscrow = value;
    }

    double MarketOrder::getPrice() const noexcept
    {
        return mPrice;
    }

    void MarketOrder::setPrice(double value) noexcept
    {
        mPrice = value;
    }

    MarketOrder::Type MarketOrder::getType() const noexcept
    {
        return mType;
    }

    void MarketOrder::setType(Type type) noexcept
    {
        mType = type;
    }

    QDateTime MarketOrder::getIssued() const
    {
        return mIssued;
    }

    void MarketOrder::setIssued(const QDateTime &dt)
    {
        mIssued = dt;
    }

    QDateTime MarketOrder::getFirstSeen() const
    {
        return mFirstSeen;
    }

    void MarketOrder::setFirstSeen(const QDateTime &dt)
    {
        mFirstSeen = dt;
    }

    QDateTime MarketOrder::getLastSeen() const
    {
        return mLastSeen;
    }

    void MarketOrder::setLastSeen(const QDateTime &dt)
    {
        mLastSeen = dt;
    }

    bool MarketOrder::isArchived() const
    {
        return !mLastSeen.isNull();
    }
}
