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
        Base::Base(const Base &other)
        {
            mChildren.reserve(other.mChildren.size());
            for (const auto &child : other.mChildren)
                mChildren.emplace_back(child->clone());
        }

        void Base::addChild(NodePtr &&child)
        {
            mChildren.emplace_back(std::move(child));
        }

        std::vector<NodePtr> &Base::getChildren() noexcept
        {
            return mChildren;
        }

        const std::vector<NodePtr> &Base::getChildren() const noexcept
        {
            return mChildren;
        }

        NodePtr Base::clone() const
        {
            return std::make_unique<Base>(*this);
        }

        NodePtr None::clone() const
        {
            return std::make_unique<None>(*this);
        }

        Real::Real(double value)
            : Base{}
            , mValue{value}
        {
        }

        NodePtr Real::clone() const
        {
            return std::make_unique<Real>(*this);
        }

        double Real::getValue() const noexcept
        {
            return mValue;
        }

        Int::Int(qint32 value)
            : Base{}
            , mValue{value}
        {
        }

        NodePtr Int::clone() const
        {
            return std::make_unique<Int>(*this);
        }

        qint32 Int::getValue() const noexcept
        {
            return mValue;
        }

        Bool::Bool(bool value)
            : Base{}
            , mValue{value}
        {
        }

        NodePtr Bool::clone() const
        {
            return std::make_unique<Bool>(*this);
        }

        bool Bool::getValue() const noexcept
        {
            return mValue;
        }

        LongLong::LongLong(qint64 value)
            : Base{}
            , mValue{value}
        {
        }

        NodePtr LongLong::clone() const
        {
            return std::make_unique<LongLong>(*this);
        }

        qint64 LongLong::getValue() const noexcept
        {
            return mValue;
        }

        Ident::Ident(const QString &name)
            : Base{}
            , mName{name}
        {
        }

        Ident::Ident(QString &&name)
            : Base{}
            , mName{std::move(name)}
        {
        }

        NodePtr Ident::clone() const
        {
            return std::make_unique<Ident>(*this);
        }

        QString Ident::getName() const
        {
            return mName;
        }

        String::String(const QString &value)
            : Base{}
            , mValue{value}
        {
        }

        String::String(QString &&value)
            : Base{}
            , mValue{std::move(value)}
        {
        }

        NodePtr String::clone() const
        {
            return std::make_unique<String>(*this);
        }

        QString String::getValue() const
        {
            return mValue;
        }

        NodePtr Dictionary::clone() const
        {
            return std::make_unique<Dictionary>(*this);
        }

        NodePtr Tuple::clone() const
        {
            return std::make_unique<Tuple>(*this);
        }

        NodePtr Substream::clone() const
        {
            return std::make_unique<Substream>(*this);
        }

        NodePtr Object::clone() const
        {
            return std::make_unique<Object>(*this);
        }

        QString Object::getName() const
        {
            const Base *current = this;
            while (current->getChildren().size() != 0)
                current = current->getChildren().front().get();

            const auto string = dynamic_cast<const String *>(current);
            return (string != nullptr) ? (string->getValue()) : (QString{});
        }

        Marker::Marker(uchar id)
            : Base{}
            , mId{id}
        {
        }

        NodePtr Marker::clone() const
        {
            return std::make_unique<Marker>(*this);
        }

        uchar Marker::getId() const noexcept
        {
            return mId;
        }

        DBRow::DBRow(int id, const std::vector<uchar> &data)
            : Base{}
            , mId{id}
            , mData{data}
        {
        }

        DBRow::DBRow(int id, std::vector<uchar> &&data)
            : Base{}
            , mId{id}
            , mData{std::move(data)}
        {
        }

        NodePtr DBRow::clone() const
        {
            return std::make_unique<DBRow>(*this);
        }
    }
}
