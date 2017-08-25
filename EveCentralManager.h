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

#include <functional>
#include <vector>

#include <QNetworkAccessManager>

#include "TypeLocationPairs.h"
#include "ExternalOrder.h"

namespace Evernus
{
    class EveDataProvider;

    class EveCentralManager
        : public QObject
    {
        Q_OBJECT

    public:
        using Callback = std::function<void (std::vector<ExternalOrder> &&data, const QString &error)>;

        explicit EveCentralManager(const EveDataProvider &dataProvider, QObject *parent = nullptr);
        virtual ~EveCentralManager() = default;

        size_t aggregateAndFetchMarketOrders(const TypeLocationPairs &target, const Callback &callback) const;
        void fetchMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const;

    private:
        static const QString baseUrl;

        const EveDataProvider &mDataProvider;

        mutable QNetworkAccessManager mNetworkManager;

        void makeMarketOrderRequest(EveType::IdType typeId, const QNetworkRequest &request, const Callback &callback) const;
        void processResult(ExternalOrder::TypeIdType typeId, const QByteArray &data, const Callback &callback) const;
    };
}
