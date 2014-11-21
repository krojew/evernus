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
#include <QDebug>

#include "EveDataProvider.h"
#include "ExternalOrder.h"

#include "CRESTExternalOrderImporter.h"

namespace Evernus
{
    CRESTExternalOrderImporter::CRESTExternalOrderImporter(const EveDataProvider &dataProvider, QObject *parent)
        : ExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
    {
    }

    void CRESTExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        for (const auto &pair : target)
        {
            const auto regionId = mDataProvider.getStationRegionId(pair.second);
            if (regionId != 0)
            {
                ++mRequestCount;
                mManager.fetchMarketOrders(regionId, pair.first, [this](auto &&orders, const auto &error) {
                    processResult(std::move(orders), error);
                });
            }
        }

        qDebug() << "Making" << mRequestCount << "CREST requests...";
    }

    void CRESTExternalOrderImporter::processResult(std::vector<ExternalOrder> &&orders, const QString &error) const
    {
        --mRequestCount;

        if (mRequestCount == 0)
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
    }
}
