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
#include <boost/scope_exit.hpp>

#include <QDebug>

#include "MathUtils.h"

#include "MarketAnalysisDataFetcher.h"

namespace Evernus
{
    MarketAnalysisDataFetcher::MarketAnalysisDataFetcher(QByteArray crestClientId,
                                                         QByteArray crestClientSecret,
                                                         const EveDataProvider &dataProvider,
                                                         QObject *parent)
        : QObject{parent}
        , mManager(std::move(crestClientId), std::move(crestClientSecret), dataProvider)
    {
    }

    bool MarketAnalysisDataFetcher::hasPendingOrderRequests() const noexcept
    {
        return mOrderRequestCount != 0;
    }

    bool MarketAnalysisDataFetcher::hasPendingHistoryRequests() const noexcept
    {
        return mHistoryRequestCount != 0;
    }

    void MarketAnalysisDataFetcher::importData(const ExternalOrderImporter::TypeLocationPairs &pairs,
                                               const MarketOrderRepository::TypeLocationPairs &ignored)
    {
        mOrders = std::make_shared<OrderResultType::element_type>();
        mHistory = std::make_shared<HistoryResultType::element_type>();

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        for (const auto &pair : pairs)
        {
            if (ignored.find(pair) != std::end(ignored))
                continue;

            ++mOrderRequestCount;
            ++mHistoryRequestCount;

            mManager.fetchMarketOrders(pair.second, pair.first, [this](auto &&orders, const auto &error) {
                processOrders(std::move(orders), error);
            });
            mManager.fetchMarketHistory(pair.second, pair.first, [pair, this](auto &&history, const auto &error) {
                processHistory(pair.second, pair.first, std::move(history), error);
            });
        }

        qDebug() << "Making" << mOrderRequestCount << mHistoryRequestCount << "CREST order and history requests...";
    }

    void MarketAnalysisDataFetcher::handleNewPreferences()
    {
        mManager.handleNewPreferences();
    }

    void MarketAnalysisDataFetcher::processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText)
    {
        --mOrderRequestCount;
        ++mOrderBatchCounter;

        qDebug() << mOrderRequestCount << " orders remaining; error:" << errorText;

        if (mOrderBatchCounter >= MathUtils::batchSize(mOrderRequestCount))
        {
            mOrderBatchCounter = 0;
            emit orderStatusUpdated(tr("Waiting for %1 order server replies...").arg(mOrderRequestCount));
        }

        if (!errorText.isEmpty())
        {
            mAggregatedOrderErrors << errorText;

            if (mOrderRequestCount == 0)
            {
                emit orderImportEnded(mOrders, mAggregatedOrderErrors.join("\n"));
                mAggregatedOrderErrors.clear();
            }

            return;
        }

        mOrders->reserve(mOrders->size() + orders.size());
        mOrders->insert(std::end(*mOrders),
                        std::make_move_iterator(std::begin(orders)),
                        std::make_move_iterator(std::end(orders)));

        if (mOrderRequestCount == 0 && !mPreparingRequests)
        {
            emit orderImportEnded(mOrders, mAggregatedOrderErrors.join("\n"));
            mAggregatedOrderErrors.clear();
        }
    }

    void MarketAnalysisDataFetcher
    ::processHistory(uint regionId, EveType::IdType typeId, std::map<QDate, MarketHistoryEntry> &&history, const QString &errorText)
    {
        --mHistoryRequestCount;
        ++mHistoryBatchCounter;

        qDebug() << mHistoryRequestCount << " history remaining; error:" << errorText;

        if (mHistoryBatchCounter >= MathUtils::batchSize(mHistoryRequestCount))
        {
            mHistoryBatchCounter = 0;
            emit historyStatusUpdated(tr("Waiting for %1 history server replies...").arg(mHistoryRequestCount));
        }

        if (!errorText.isEmpty())
        {
            mAggregatedHistoryErrors << errorText;

            if (mHistoryRequestCount == 0)
            {
                emit historyImportEnded(mHistory, mAggregatedHistoryErrors.join("\n"));
                mAggregatedHistoryErrors.clear();
            }

            return;
        }

        (*mHistory)[regionId][typeId] = std::move(history);

        if (mHistoryRequestCount == 0 && !mPreparingRequests)
        {
            emit historyImportEnded(mHistory, mAggregatedHistoryErrors.join("\n"));
            mAggregatedHistoryErrors.clear();
        }
    }
}
