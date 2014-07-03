#include "Item.h"

namespace Evernus
{
    Item::Item(const Item &other)
        : Entity{other}
        , mParentId{other.mParentId}
        , mListId{other.mListId}
        , mData(other.mData)
    {
        for (const auto &item : other)
            addItem(std::make_unique<Item>(*item));
    }

    Item::ParentIdType Item::getParentId() const noexcept
    {
        return mParentId;
    }

    void Item::setParentId(const ParentIdType &id) noexcept
    {
        mParentId = id;
    }

    AssetList::IdType Item::getListId() const noexcept
    {
        return mListId;
    }

    void Item::setListId(AssetList::IdType id) noexcept
    {
        mListId = id;

        for (auto &item : mContents)
            item->setListId(mListId);
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

    ItemData Item::getItemData() const &
    {
        return mData;
    }

    ItemData &&Item::getItemData() && noexcept
    {
        return std::move(mData);
    }

    void Item::setItemData(const ItemData &data)
    {
        mData = data;
    }

    void Item::setItemData(ItemData &&data)
    {
        mData = std::move(data);
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

    size_t Item::getChildCount() const noexcept
    {
        return mContents.size();
    }

    void Item::addItem(std::unique_ptr<Item> &&item)
    {
        Q_ASSERT(item);

        item->setParentId(getId());
        item->setListId(getListId());
        mContents.emplace_back(std::move(item));
    }

    Item &Item::operator =(const Item &other)
    {
        using std::swap;

        Item copy{other};
        swap(*this, copy);

        return *this;
    }
}
