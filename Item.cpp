#include "Item.h"

namespace Evernus
{
    Item::Item(quint64 id)
        : mId{id}
    {
    }

    quint64 Item::getId() const noexcept
    {
        return mId;
    }

    void Item::setId(quint64 id) noexcept
    {
        mId = id;
    }

    Item::TypeIdType Item::getTypeId() const
    {
        return mTypeId;
    }

    void Item::setTypeId(const TypeIdType &id)
    {
        mTypeId = id;
    }

    Item::LocationIdType Item::getLocationId() const
    {
        return mLocationId;
    }

    void Item::setLocationId(const LocationIdType &id)
    {
        mLocationId = id;
    }

    uint Item::getQuantity() const noexcept
    {
        return mQuantity;
    }

    void Item::setQuantity(uint value) noexcept
    {
        mQuantity = value;
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
