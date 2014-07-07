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
#include "ItemPrice.h"

#include "MarketLogItemPriceImporter.h"

namespace Evernus
{
    MarketLogItemPriceImporter::~MarketLogItemPriceImporter()
    {
        try
        {
            for (auto &thread : mScanningThreads)
            {
                thread->requestInterruption();
                thread->wait(10 * 1000);
                if (thread->isRunning())
                    thread->terminate();
            }
        }
        catch (...)
        {
            std::terminate();
        }
    }

    void MarketLogItemPriceImporter::fetchItemPrices(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit itemPricesChanged(std::vector<ItemPrice>{});
            return;
        }

        auto thread = std::make_unique<MarketLogItemPriceImporterThread>();
        connect(thread.get(), &MarketLogItemPriceImporterThread::finished, this, &MarketLogItemPriceImporter::threadFinished);
        connect(thread.get(), &MarketLogItemPriceImporterThread::error, this, &MarketLogItemPriceImporter::threadError);
        thread->start();

        mScanningThreads.emplace_back(std::move(thread));
    }

    void MarketLogItemPriceImporter::threadFinished(const ItemPriceList &prices)
    {
        deleteThread(static_cast<MarketLogItemPriceImporterThread *>(sender()));
        emit itemPricesChanged(prices);
    }

    void MarketLogItemPriceImporter::threadError(const QString &info)
    {
        deleteThread(static_cast<MarketLogItemPriceImporterThread *>(sender()));
        emit error(info);
    }

    void MarketLogItemPriceImporter::deleteThread(MarketLogItemPriceImporterThread *thread)
    {
        thread->deleteLater();
        mScanningThreads.erase(std::find_if(std::begin(mScanningThreads), std::end(mScanningThreads), [thread](const auto &entry) {
            return entry.get() == thread;
        }));
    }
}
