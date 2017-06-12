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

#include "RefType.h"

namespace Evernus
{
    RefType::RefType(IdType id, const QString &name)
        : Entity{id}
        , mName{name}
    {
    }

    RefType::RefType(IdType id, QString &&name)
        : Entity{id}
        , mName{std::move(name)}
    {
    }

    QString RefType::getName() const &
    {
        return mName;
    }

    QString &&RefType::getName() && noexcept
    {
        return std::move(mName);
    }

    void RefType::setName(const QString &name)
    {
        mName = name;
    }

    void RefType::setName(QString &&name)
    {
        mName = std::move(name);
    }
}
