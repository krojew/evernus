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

#include <vector>

#include <QThread>

#include "ItemPrice.h"

namespace Evernus
{
    class ItemPrice;

    class MarketLogItemPriceImporterThread
        : public QThread
    {
        Q_OBJECT

    public:
        typedef std::vector<ItemPrice> ItemPriceList;

        using QThread::QThread;
        virtual ~MarketLogItemPriceImporterThread() = default;

    signals:
        void finished(const ItemPriceList &prices);
        void error(const QString &info);

    protected:
        virtual void run() override;

    private:
        static void getItemPrice(const QString &logPath, ItemPriceList &prices, bool deleteLog);
    };
}

Q_DECLARE_METATYPE(Evernus::MarketLogItemPriceImporterThread::ItemPriceList);
