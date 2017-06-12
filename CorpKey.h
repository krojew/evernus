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

#include "Character.h"

namespace Evernus
{
    class CorpKey
        : public Entity<uint>
    {
    public:
        using Entity::Entity;

        CorpKey() = default;

        template<class T>
        CorpKey(IdType id, T &&code)
            : Entity{id}
            , mCode{std::forward<T>(code)}
        {
        }

        CorpKey(const CorpKey &) = default;
        CorpKey(CorpKey &&) = default;
        virtual ~CorpKey() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        QString getCode() const &;
        QString &&getCode() && noexcept;
        void setCode(const QString &code);
        void setCode(QString &&code);

        CorpKey &operator =(const CorpKey &) = default;
        CorpKey &operator =(CorpKey &&) = default;

    private:
        Character::IdType mCharacterId = Character::invalidId;
        QString mCode;
    };
}
