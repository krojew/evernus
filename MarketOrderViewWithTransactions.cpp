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
#include <unordered_map>

#include <QHeaderView>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QAction>

#include "ItemCostProvider.h"
#include "MarketOrderModel.h"
#include "MarketOrderView.h"
#include "StyledTreeView.h"

#include "MarketOrderViewWithTransactions.h"

namespace Evernus
{
    MarketOrderViewWithTransactions::MarketOrderViewWithTransactions(const WalletTransactionRepository &transactionsRepo,
                                                                     const EveDataProvider &dataProvider,
                                                                     ItemCostProvider &costProvider,
                                                                     QWidget *parent)
        : QWidget(parent)
        , mCostProvider(costProvider)
        , mTransactionModel(transactionsRepo, dataProvider)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mOrderView = new MarketOrderView{dataProvider, this};
        mainLayout->addWidget(mOrderView, 1);
        connect(this, &MarketOrderViewWithTransactions::statusFilterChanged, mOrderView, &MarketOrderView::statusFilterChanged);
        connect(this, &MarketOrderViewWithTransactions::priceStatusFilterChanged, mOrderView, &MarketOrderView::priceStatusFilterChanged);
        connect(this, &MarketOrderViewWithTransactions::wildcardChanged, mOrderView, &MarketOrderView::wildcardChanged);
        connect(mOrderView->getSelectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MarketOrderViewWithTransactions::selectOrder);

        auto transactionGroup = new QGroupBox{tr("Transactions"), this};
        mainLayout->addWidget(transactionGroup);

        auto groupLayout = new QVBoxLayout{};
        transactionGroup->setLayout(groupLayout);

        mTransactionProxyModel.setSortRole(Qt::UserRole);
        mTransactionProxyModel.setSourceModel(&mTransactionModel);

        auto addCostAct = new QAction{tr("Add to item costs"), this};
        connect(addCostAct, &QAction::triggered, this, &MarketOrderViewWithTransactions::addItemCost);

        mTransactionsView = new StyledTreeView{this};
        groupLayout->addWidget(mTransactionsView);
        mTransactionsView->setModel(&mTransactionProxyModel);
        mTransactionsView->sortByColumn(1, Qt::DescendingOrder);
        mTransactionsView->addAction(addCostAct);
        mTransactionsView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    void MarketOrderViewWithTransactions::setModel(MarketOrderModel *model)
    {
        mOrderModel = model;
        mOrderView->setModel(mOrderModel);
    }

    void MarketOrderViewWithTransactions::setShowInfo(bool flag)
    {
        mOrderView->setShowInfo(flag);
    }

    void MarketOrderViewWithTransactions::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        mTransactionModel.clear();
    }

    void MarketOrderViewWithTransactions::expandAll()
    {
        mOrderView->expandAll();
    }

    void MarketOrderViewWithTransactions::sortByColumn(int column, Qt::SortOrder order)
    {
        mOrderView->sortByColumn(column, order);
    }

    void MarketOrderViewWithTransactions::selectOrder(const QItemSelection &selected, const QItemSelection &deselected)
    {
        Q_UNUSED(deselected);

        if (mOrderModel != nullptr && !selected.isEmpty())
        {
            const auto index = mOrderView->getProxyModel().mapToSource(selected.indexes().first());
            const auto typeId = mOrderModel->getOrderTypeId(index);

            if (typeId == EveType::invalidId)
            {
                mTransactionModel.clear();
            }
            else
            {
                auto range = mOrderModel->getOrderRange(index);

                if (!range.mTo.isValid())
                    range.mTo = QDateTime::currentDateTimeUtc();

                mTransactionModel.setFilter(
                    mCharacterId, range.mFrom.date(), range.mTo.date(), mOrderModel->getOrderTypeFilter(index), typeId);
            }
        }
    }

    void MarketOrderViewWithTransactions::addItemCost()
    {
        const auto selection = mTransactionsView->selectionModel()->selectedIndexes();

        struct ItemData
        {
            uint mQuantity = 0;
            double mPrice = 0.;
        };

        std::unordered_map<EveType::IdType, ItemData> aggrData;
        for (const auto &index : selection)
        {
            if (index.column() != 0)
                continue;

            const auto mappedIndex = mTransactionProxyModel.mapToSource(index);
            const auto row = mappedIndex.row();

            auto &data = aggrData[mTransactionModel.getTypeId(row)];

            const auto quantity = mTransactionModel.getQuantity(row);

            data.mQuantity += quantity;
            data.mPrice += quantity * mTransactionModel.getPrice(row);
        }

        for (const auto &data : aggrData)
            mCostProvider.setForCharacterAndType(mCharacterId, data.first, data.second.mPrice / data.second.mQuantity);
    }
}
