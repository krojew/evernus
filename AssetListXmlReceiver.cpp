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
#include <stdexcept>
#include <typeinfo>

#include <QCoreApplication>

#include "Item.h"

#include "AssetListXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>::startElement(const QXmlName &name)
    {
        auto localName = name.localName(mNamePool);
        if (localName == "row")
            mCurrentElement.reset(new AssetList::ItemType::element_type{});
        else if (localName == "rowset")
            mAdditionalData.mElementStack.emplace(std::move(mCurrentElement));

        mAdditionalData.mTagStack.emplace(std::move(localName));
    }

    template<>
    void APIXmlReceiver<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>::endElement()
    {
        if (mCurrentElement)
        {
            if (mAdditionalData.mElementStack.empty())
                mContainer.emplace_back(std::move(mCurrentElement));
            else
                mAdditionalData.mElementStack.top()->addItem(std::move(mCurrentElement));
        }
        else if (mAdditionalData.mTagStack.top() == "row")
        {
            Q_ASSERT(!mAdditionalData.mElementStack.empty());

            if (mAdditionalData.mElementStack.size() == 1)
            {
                mContainer.emplace_back(std::move(mAdditionalData.mElementStack.top()));
                mAdditionalData.mElementStack.pop();
            }
            else
            {
                auto last = std::move(mAdditionalData.mElementStack.top());
                mAdditionalData.mElementStack.pop();
                mAdditionalData.mElementStack.top()->addItem(std::move(last));
            }
        }

        mAdditionalData.mTagStack.pop();
    }

    template<>
    void APIXmlReceiver<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        if (localName == "itemID")
            mCurrentElement->setId(convert<Item::IdType>(value.toString()));
        else if (localName == "typeID")
            mCurrentElement->setTypeId(convert<ItemData::TypeIdType>(value.toString()));
        else if (localName == "locationID")
            mCurrentElement->setLocationId(convert<ItemData::LocationIdType::value_type>(value.toString()));
        else if (localName == "quantity")
            mCurrentElement->setQuantity(convert<uint>(value.toString()));
        else if (localName == "rawQuantity")
            mCurrentElement->setRawQuantity(convert<int>(value.toString()));
    }
}
