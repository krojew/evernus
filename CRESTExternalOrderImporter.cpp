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

#include <boost/scope_exit.hpp>

#include "EveDataProvider.h"

#include "CRESTExternalOrderImporter.h"

namespace Evernus
{
    CRESTExternalOrderImporter::CRESTExternalOrderImporter(QByteArray clientId, QByteArray clientSecret, const EveDataProvider &dataProvider, QObject *parent)
        : ExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
        , mManager{std::move(clientId), std::move(clientSecret), mDataProvider}
    {
    }

    void CRESTExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        mResult.clear();

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

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

        if (mRequestCount == 0)
        {
            emit externalOrdersChanged(mResult);
            mResult.clear();
        }
    }

    void CRESTExternalOrderImporter::processResult(std::vector<ExternalOrder> &&orders, const QString &errorText) const
    {
        --mRequestCount;

        qDebug() << "Got reply," << mRequestCount << "remaining.";

        if (!errorText.isEmpty())
        {
            if (mRequestCount == 0)
                mResult.clear();

            emit error(errorText);
            return;
        }

        mResult.reserve(mResult.size() + orders.size());
        mResult.insert(std::end(mResult),
                       std::make_move_iterator(std::begin(orders)),
                       std::make_move_iterator(std::end(orders)));

        if (mRequestCount == 0)
        {
            if (!mPreparingRequests)
            {
                emit externalOrdersChanged(mResult);
                mResult.clear();
            }
        }
        else
        {
            emit statusChanged(tr("CREST import: waiting for %1 server replies").arg(mRequestCount));
        }
    }
}
