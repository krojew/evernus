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

#include <QWidget>

#include "MarketOrderFilterProxyModel.h"

class QItemSelectionModel;
class QActionGroup;
class QLabel;

namespace Evernus
{
    class EveDataProvider;
    class MarketOrderModel;
    class StyledTreeView;

    class MarketOrderView
        : public QWidget
    {
        Q_OBJECT

    public:
        MarketOrderView(const EveDataProvider &dataProvider, const QString &objectName, QWidget *parent = nullptr);
        virtual ~MarketOrderView() = default;

        QItemSelectionModel *getSelectionModel() const;
        const QAbstractProxyModel &getProxyModel() const;

        void setModel(MarketOrderModel *model);
        void setShowInfo(bool flag);

        void expandAll();

        void sortByColumn(int column, Qt::SortOrder order);

    signals:
        void closeOrderInfo();

        void statusFilterChanged(const MarketOrderFilterProxyModel::StatusFilters &filter);
        void priceStatusFilterChanged(const MarketOrderFilterProxyModel::PriceStatusFilters &filter);

        void textFilterChanged(const QString &text, bool script);

        void scriptError(const QString &message);

    public slots:
        void updateInfo();

    private slots:
        void showPriceInfo(const QModelIndex &index);

        void lookupOnEveMarketdata();
        void lookupOnEveCentral();

        void selectOrder(const QItemSelection &selected);
        void removeOrders();

    private:
        StyledTreeView *mView = nullptr;

        QLabel *mTotalOrdersLabel = nullptr;
        QLabel *mVolumeLabel = nullptr;
        QLabel *mTotalISKLabel = nullptr;
        QLabel *mTotalSizeLabel = nullptr;
        QWidget *mInfoWidget = nullptr;

        MarketOrderModel *mSource = nullptr;
        MarketOrderFilterProxyModel mProxy;

        QAction *mRemoveOrderAct = nullptr;
        QActionGroup *mLookupGroup = nullptr;

        void lookupOnWeb(const QString &baseUrl) const;
    };
}
