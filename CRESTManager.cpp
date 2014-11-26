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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "ExternalOrder.h"

#include "CRESTManager.h"

namespace Evernus
{
    void CRESTManager::fetchMarketOrders(uint regionId,
                                         EveType::IdType typeId,
                                         const Callback<std::vector<ExternalOrder>> &callback) const
    {
#if defined(EVERNUS_CREST_CLIENT_ID) and defined(EVERNUS_CREST_SECRET)
#ifdef Q_OS_WIN
        mInterface.fetchBuyMarketOrders(regionId, typeId, [=](auto &&buyData, const auto &error) {
#else
        mInterface.fetchBuyMarketOrders(regionId, typeId, [=, callback = callback](auto &&buyData, const auto &error) {
#endif
            if (!error.isEmpty())
            {
                callback(std::vector<ExternalOrder>{}, error);
                return;
            }

#ifdef Q_OS_WIN
            mInterface.fetchSellMarketOrders(regionId, typeId, [=](auto &&sellData, const auto &error) {
#else
            mInterface.fetchSellMarketOrders(regionId, typeId, [=, callback = callback](auto &&sellData, const auto &error) {
#endif
                if (!error.isEmpty())
                {
                    callback(std::vector<ExternalOrder>{}, error);
                    return;
                }

    //            const auto object = data.object();
    //            const auto items = object.value("items").toArray();
    //
                std::vector<ExternalOrder> orders;
    //            orders.reserve(items.size());
    //
    //            for (const auto &item : items)
    //            {
    //                const auto itemObject = item.toObject();
    //
    //                ExternalOrder order;
    //            }

                callback(std::move(orders), QString{});
            });
        });
#else
        callback(std::vector<ExternalOrder>{}, "Evernus has been compiled without CREST support.");
#endif
    }
}
