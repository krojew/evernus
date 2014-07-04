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
    class ConquerableStation
        : public Entity<uint>
    {
    public:
        using Entity::Entity;

        ConquerableStation() = default;
        ConquerableStation(IdType id, const QString &name);
        ConquerableStation(IdType id, QString &&name);
        virtual ~ConquerableStation() = default;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

    private:
        QString mName;
    };
}
