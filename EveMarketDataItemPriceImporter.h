#pragma once

#include <QNetworkAccessManager>

#include "ItemPriceImporter.h"

namespace Evernus
{
    class EveMarketDataItemPriceImporter
        : public ItemPriceImporter
    {
    public:
        using ItemPriceImporter::ItemPriceImporter;
        virtual ~EveMarketDataItemPriceImporter() = default;

        virtual void fetchItemPrices(const TypeLocationPairs &target) const override;

    private:
        mutable QNetworkAccessManager mNetworkManager;

        void processReply(const TypeLocationPairs &target) const;
    };
}
