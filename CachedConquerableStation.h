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

#include "CachedConquerableStationList.h"
#include "Entity.h"

namespace Evernus
{
    class CachedConquerableStation
        : public Entity<uint>
    {
    public:
        using Entity::Entity;
        virtual ~CachedConquerableStation() = default;

        CachedConquerableStationList::IdType getListId() const noexcept;
        void setListId(CachedConquerableStationList::IdType id) noexcept;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

    private:
        CachedConquerableStationList::IdType mListId = CachedConquerableStationList::invalidId;
        QString mName;
    };
}
