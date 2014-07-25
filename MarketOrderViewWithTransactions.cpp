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
#include <QHeaderView>
#include <QVBoxLayout>
#include <QGroupBox>

#include "MarketOrderModel.h"
#include "MarketOrderView.h"
#include "StyledTreeView.h"

#include "MarketOrderViewWithTransactions.h"

namespace Evernus
{
    MarketOrderViewWithTransactions::MarketOrderViewWithTransactions(const WalletTransactionRepository &transactionsRepo,
                                                                     const EveDataProvider &dataProvider,
                                                                     QWidget *parent)
        : QWidget{parent}
        , mTransactionModel{transactionsRepo, dataProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mOrderView = new MarketOrderView{dataProvider, this};
        mainLayout->addWidget(mOrderView, 1);
        connect(this, &MarketOrderViewWithTransactions::statusFilterChanged, mOrderView, &MarketOrderView::statusFilterChanged);
        connect(this, &MarketOrderViewWithTransactions::priceStatusFilterChanged, mOrderView, &MarketOrderView::priceStatusFilterChanged);
        connect(this, &MarketOrderViewWithTransactions::wildcardChanged, mOrderView, &MarketOrderView::wildcardChanged);

        auto transactionGroup = new QGroupBox{tr("Transactions"), this};
        mainLayout->addWidget(transactionGroup);

        auto groupLayout = new QVBoxLayout{};
        transactionGroup->setLayout(groupLayout);

        mTransactionProxyModel.setSortRole(Qt::UserRole);
        mTransactionProxyModel.setSourceModel(&mTransactionModel);

        auto transactionView = new StyledTreeView{this};
        groupLayout->addWidget(transactionView);
        transactionView->setModel(&mTransactionProxyModel);
        transactionView->sortByColumn(1, Qt::DescendingOrder);
        transactionView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        connect(mOrderView->getSelectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MarketOrderViewWithTransactions::selectOrder);
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

        if (mOrderModel != nullptr)
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

                mTransactionModel.setFilter(mCharacterId, range.mFrom.date(), range.mTo.date(), mOrderModel->getOrderTypeFilter(), typeId);
            }
        }
    }
}
