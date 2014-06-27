#include "Item.h"

namespace Evernus
{
    Item::Item(ItemData::IdType id)
        : mId{id}
    {
    }

    ItemData::IdType Item::getId() const noexcept
    {
        return mId;
    }

    void Item::setId(ItemData::IdType id) noexcept
    {
        mId = id;
    }

    ItemData::TypeIdType Item::getTypeId() const
    {
        return mData.mTypeId;
    }

    void Item::setTypeId(const ItemData::TypeIdType &id)
    {
        mData.mTypeId = id;
    }

    ItemData::LocationIdType Item::getLocationId() const
    {
        return mData.mLocationId;
    }

    void Item::setLocationId(const ItemData::LocationIdType &id)
    {
        mData.mLocationId = id;
    }

    uint Item::getQuantity() const noexcept
    {
        return mData.mQuantity;
    }

    void Item::setQuantity(uint value) noexcept
    {
        mData.mQuantity = value;
    }

    Item::ItemIterator Item::begin() noexcept
    {
        return mContents.begin();
    }

    Item::ConstItemIterator Item::begin() const noexcept
    {
        return mContents.begin();
    }

    Item::ItemIterator Item::end() noexcept
    {
        return mContents.end();
    }

    Item::ConstItemIterator Item::end() const noexcept
    {
        return mContents.end();
    }

    void Item::addItem(std::unique_ptr<Item> &&item)
    {
        mContents.emplace_back(std::move(item));
    }
}
