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
#include <memory>
#include <map>

#include <QTimer>
#include <QDate>

#include "MarketHistoryEntry.h"
#include "CRESTInterface.h"
#include "EveType.h"

namespace Evernus
{
    class EveDataProvider;
    class ExternalOrder;

    class CRESTManager
        : public QObject
    {
        Q_OBJECT

    public:
        template<class T>
        using Callback = std::function<void (T &&data, const QString &error)>;

        CRESTManager(const EveDataProvider &dataProvider,
                     QObject *parent = nullptr);
        virtual ~CRESTManager() = default;

        void fetchMarketOrders(uint regionId,
                               EveType::IdType typeId,
                               const Callback<std::vector<ExternalOrder>> &callback) const;
        void fetchMarketHistory(uint regionId,
                                EveType::IdType typeId,
                                const Callback<std::map<QDate, MarketHistoryEntry>> &callback) const;

    signals:
        void error(const QString &text);

    public slots:
        void handleNewPreferences();

    private:
        const EveDataProvider &mDataProvider;

        CRESTInterface mInterface;

        QNetworkAccessManager mNetworkManager;

        CRESTInterface::EndpointMap mEndpoints;
        QTimer mEndpointTimer;

        void fetchEndpoints();
        bool hasEndpoints() const;

        static QString getMissingEnpointsError();
    };
}
