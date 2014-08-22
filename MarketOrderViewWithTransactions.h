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

#include <QSortFilterProxyModel>
#include <QWidget>

#include "MarketOrderFilterProxyModel.h"
#include "WalletTransactionsModel.h"

class QItemSelection;

namespace Evernus
{
    class WalletTransactionRepository;
    class CharacterRepository;
    class ItemCostProvider;
    class MarketOrderModel;
    class EveDataProvider;
    class MarketOrderView;
    class StyledTreeView;

    class MarketOrderViewWithTransactions
        : public QWidget
    {
        Q_OBJECT

    public:
        MarketOrderViewWithTransactions(const WalletTransactionRepository &transactionsRepo,
                                        const CharacterRepository &characterRepository,
                                        const EveDataProvider &dataProvider,
                                        ItemCostProvider &costProvider,
                                        bool corp,
                                        QWidget *parent = nullptr);
        virtual ~MarketOrderViewWithTransactions() = default;

        void setModel(MarketOrderModel *model);
        void setShowInfo(bool flag);
        void setCharacter(Character::IdType id);

        void expandAll();

        void sortByColumn(int column, Qt::SortOrder order);

    signals:
        void statusFilterChanged(const MarketOrderFilterProxyModel::StatusFilters &filter);
        void priceStatusFilterChanged(const MarketOrderFilterProxyModel::PriceStatusFilters &filter);

        void textFilterChanged(const QString &text, bool script);

        void scriptError(const QString &message);

    private slots:
        void selectOrder(const QItemSelection &selected, const QItemSelection &deselected);

        void addItemCost();

    private:
        ItemCostProvider &mCostProvider;

        MarketOrderView *mOrderView = nullptr;
        StyledTreeView *mTransactionsView = nullptr;

        MarketOrderModel *mOrderModel = nullptr;
        WalletTransactionsModel mTransactionModel;
        QSortFilterProxyModel mTransactionProxyModel;

        Character::IdType mCharacterId = Character::invalidId;
    };
}
