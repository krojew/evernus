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

#include <memory>

#include <QSortFilterProxyModel>
#include <QWidget>

#include "MarketOrderFilterProxyModel.h"
#include "WalletTransactionsModel.h"
#include "ExternalOrderModel.h"

class QItemSelection;

namespace Evernus
{
    class WalletTransactionRepository;
    class ExternalOrderRepository;
    class MarketOrderProvider;
    class CharacterRepository;
    class ExternalOrderView;
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
                                        const CharacterRepository &characterRepo,
                                        const ExternalOrderRepository &externalOrderRepo,
                                        const EveDataProvider &dataProvider,
                                        ItemCostProvider &costProvider,
                                        const MarketOrderProvider &orderProvider,
                                        const MarketOrderProvider &corpOrderProvider,
                                        bool corp,
                                        const QString &objectName,
                                        bool showExternalOrders,
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
        const CharacterRepository &mCharacterRepo;
        const ExternalOrderRepository &mExternalOrderRepo;
        const EveDataProvider &mDataProvider;
        ItemCostProvider &mCostProvider;
        const MarketOrderProvider &mOrderProvider;
        const MarketOrderProvider &mCorpOrderProvider;

        MarketOrderView *mOrderView = nullptr;
        ExternalOrderView *mExternalOrderView = nullptr;
        StyledTreeView *mTransactionsView = nullptr;

        MarketOrderModel *mOrderModel = nullptr;
        WalletTransactionsModel mTransactionModel;
        QSortFilterProxyModel mTransactionProxyModel;

        std::unique_ptr<ExternalOrderModel> mExternalOrderModel;

        Character::IdType mCharacterId = Character::invalidId;
    };
}
