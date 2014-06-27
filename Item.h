#pragma once

#include <vector>
#include <memory>

#include <boost/optional.hpp>

#include "EveType.h"

namespace Evernus
{
    class Item
    {
    public:
        typedef boost::optional<quint64> LocationIdType;
        typedef EveType::IdType TypeIdType;

        typedef std::vector<std::unique_ptr<Item>> ItemList;
        typedef ItemList::iterator ItemIterator;
        typedef ItemList::const_iterator ConstItemIterator;

        Item() = default;
        explicit Item(quint64 id);
        virtual ~Item() = default;

        quint64 getId() const noexcept;
        void setId(quint64 id) noexcept;

        TypeIdType getTypeId() const;
        void setTypeId(const TypeIdType &id);

        LocationIdType getLocationId() const;
        void setLocationId(const LocationIdType &id);

        uint getQuantity() const noexcept;
        void setQuantity(uint value) noexcept;

        ItemIterator begin() noexcept;
        ConstItemIterator begin() const noexcept;
        ItemIterator end() noexcept;
        ConstItemIterator end() const noexcept;

        void addItem(std::unique_ptr<Item> &&item);

    private:
        quint64 mId = 0;
        TypeIdType mTypeId = 0;
        LocationIdType mLocationId;
        uint mQuantity = 0;
        ItemList mContents;
    };
}
