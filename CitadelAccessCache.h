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

#include <unordered_set>

#include <boost/functional/hash.hpp>

#include <QDateTime>

#include "Character.h"
#include "Citadel.h"

class QDataStream;

namespace Evernus
{
    class CitadelAccessCache final
    {
    public:
        CitadelAccessCache() = default;
        CitadelAccessCache(const CitadelAccessCache &) = default;
        CitadelAccessCache(CitadelAccessCache &&) = default;
        ~CitadelAccessCache() = default;

        CitadelAccessCache &operator =(const CitadelAccessCache &) = default;
        CitadelAccessCache &operator =(CitadelAccessCache &&) = default;

    private:
        using CharacterCitadelPair = std::pair<Character::IdType, Citadel::IdType>;

        QDateTime mTimeStamp = QDateTime::currentDateTimeUtc();

        std::unordered_set<CharacterCitadelPair, boost::hash<CharacterCitadelPair>> mBlacklisted;

        friend QDataStream &operator <<(QDataStream &stream, const CitadelAccessCache &cache);
        friend QDataStream &operator >>(QDataStream &stream, CitadelAccessCache &cache);
    };
}
