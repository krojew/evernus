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
#include "EveCacheNodes.h"

namespace Evernus
{
    namespace EveCacheNode
    {
        void Base::addChild(NodePtr &&child)
        {
            mChildren.emplace_back(std::move(child));
        }

        Real::Real(double value)
            : Base{}
            , mValue{value}
        {
        }

        double Real::getValue() const noexcept
        {
            return mValue;
        }

        Int::Int(int32_t value)
            : Base{}
            , mValue{value}
        {
        }

        int32_t Int::getValue() const noexcept
        {
            return mValue;
        }

        Bool::Bool(bool value)
            : Base{}
            , mValue{value}
        {
        }

        bool Bool::getValue() const noexcept
        {
            return mValue;
        }

        LongLong::LongLong(int64_t value)
            : Base{}
            , mValue{value}
        {
        }

        int64_t LongLong::getValue() const noexcept
        {
            return mValue;
        }

        Ident::Ident(const std::string &name)
            : Base{}
            , mName{name}
        {
        }

        Ident::Ident(std::string &&name)
            : Base{}
            , mName{std::move(name)}
        {
        }

        std::string Ident::getName() const
        {
            return mName;
        }

        String::String(const std::string &value)
            : Base{}
            , mValue{value}
        {
        }

        String::String(std::string &&value)
            : Base{}
            , mValue{std::move(value)}
        {
        }

        std::string String::getValue() const
        {
            return mValue;
        }
    }
}
