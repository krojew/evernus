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
    CRESTExternalOrderImporter::CRESTExternalOrderImporter(const EveDataProvider &dataProvider, QObject *parent)
        : ExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
        , mManager{mDataProvider}
    {
        connect(&mManager, &CRESTManager::error, this, &CRESTExternalOrderImporter::error);
    }

    void CRESTExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        mCounter.resetBatchIfEmpty();

        for (const auto &pair : target)
        {
            const auto regionId = mDataProvider.getStationRegionId(pair.second);
            if (regionId != 0)
            {
                mCounter.incCount();
                mManager.fetchMarketOrders(regionId, pair.first, [this](auto &&orders, const auto &error) {
                    processResult(std::move(orders), error);
                });
            }
        }

        qDebug() << "Making" << mCounter.getCount() << "CREST requests...";

        if (mCounter.isEmpty())
        {
            emit externalOrdersChanged(mResult);
            mResult.clear();
        }
    }

    void CRESTExternalOrderImporter::handleNewPreferences()
    {
        mManager.handleNewPreferences();
    }

    void CRESTExternalOrderImporter::processResult(std::vector<ExternalOrder> &&orders, const QString &errorText) const
    {
        if (mCounter.advanceAndCheckBatch())
            emit statusChanged(tr("CREST import: waiting for %1 server replies").arg(mCounter.getCount()));

        qDebug() << "Got reply," << mCounter.getCount() << "remaining.";

        if (!errorText.isEmpty())
        {
            mAggregatedErrors << errorText;

            if (mCounter.isEmpty())
            {
                mResult.clear();
                emit error(mAggregatedErrors.join("\n"));

                mAggregatedErrors.clear();
            }

            return;
        }

        mResult.reserve(mResult.size() + orders.size());
        mResult.insert(std::end(mResult),
                       std::make_move_iterator(std::begin(orders)),
                       std::make_move_iterator(std::end(orders)));

        if (mCounter.isEmpty() && !mPreparingRequests)
        {
            if (mAggregatedErrors.isEmpty())
            {
                emit externalOrdersChanged(mResult);
            }
            else
            {
                emit error(mAggregatedErrors.join("\n"));
                mAggregatedErrors.clear();
            }

            mResult.clear();
        }
    }
}
