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

#include "EveCentralExternalOrderImporter.h"

namespace Evernus
{
    EveCentralExternalOrderImporter::EveCentralExternalOrderImporter(const EveDataProvider &dataProvider, QObject *parent)
        : ExternalOrderImporter{parent}
        , mManager{dataProvider}
    {
    }

    void EveCentralExternalOrderImporter::fetchExternalOrders(Character::IdType id, const TypeLocationPairs &target) const
    {
        Q_UNUSED(id);

        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        qDebug() << "Fetching" << target.size() << "orders from eve-central.";

        mCounter.resetBatchIfEmpty();
        mCounter.addCount(mManager.aggregateAndFetchMarketOrders(target, [=](auto &&data, const auto &errorText) {
            processResult(std::move(data), errorText);
        }));

        qDebug() << "After aggregation:" << mCounter.getCount();

        if (mCounter.isEmpty())
        {
            if (mAggregatedErrors.isEmpty())
                emit externalOrdersChanged(mResult);
            else
                emit error(mAggregatedErrors.join("\n"));

            mResult.clear();
        }
    }

    void EveCentralExternalOrderImporter::processResult(std::vector<ExternalOrder> &&data, const QString &errorText) const
    {
        if (mCounter.advanceAndCheckBatch())
            emit statusChanged(tr("Waiting for %1 eve-central replies...").arg(mCounter.getCount()));

        qDebug() << "Got reply," << mCounter.getCount() << "remaining.";

        if (!errorText.isEmpty())
            mAggregatedErrors << errorText;

        if (mAggregatedErrors.isEmpty())
        {
            mResult.insert(std::end(mResult),
                           std::make_move_iterator(std::begin(data)),
                           std::make_move_iterator(std::end(data)));
        }

        if (mCounter.isEmpty())
        {
            if (mAggregatedErrors.isEmpty())
                emit externalOrdersChanged(mResult);
            else
                emit error(mAggregatedErrors.join("\n"));

            mResult.clear();
        }
    }
}
