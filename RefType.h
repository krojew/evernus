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

#include <QString>

#include "Entity.h"

namespace Evernus
{
    class RefType
        : public Entity<uint>
    {
    public:
        using Entity::Entity;

        RefType() = default;
        RefType(IdType id, const QString &name);
        RefType(IdType id, QString &&name);
        RefType(const RefType &) = default;
        RefType(RefType &&) = default;
        virtual ~RefType() = default;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

        RefType &operator =(const RefType &) = default;
        RefType &operator =(RefType &&) = default;

    private:
        QString mName;
    };
}
