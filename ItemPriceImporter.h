#pragma once

#include <unordered_set>
#include <vector>

#include <boost/functional/hash.hpp>

#include <QObject>

#include "EveType.h"

namespace Evernus
{
    class ItemPrice;

    class ItemPriceImporter
        : public QObject
    {
        Q_OBJECT

    public:
        typedef std::pair<EveType::IdType, quint64> TypeLocationPair;
        typedef std::unordered_set<TypeLocationPair, boost::hash<TypeLocationPair>> TypeLocationPairs;

        using QObject::QObject;
        virtual ~ItemPriceImporter() = default;

        virtual void fetchItemPrices(const TypeLocationPairs &target) const = 0;

    signals:
        void itemPricesChanged(const std::vector<ItemPrice> &prices) const;
        void error(const QString &info) const;
    };
}
