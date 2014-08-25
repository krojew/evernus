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
#include <QTabWidget>
#include <QGroupBox>
#include <QAction>

#include "ExternalOrderSellModel.h"
#include "ExternalOrderBuyModel.h"
#include "ExternalOrderView.h"
#include "ItemCostProvider.h"
#include "MarketOrderModel.h"
#include "MarketOrderView.h"
#include "StyledTreeView.h"

#include "MarketOrderViewWithTransactions.h"

namespace Evernus
{
    MarketOrderViewWithTransactions::MarketOrderViewWithTransactions(const WalletTransactionRepository &transactionsRepo,
                                                                     const CharacterRepository &characterRepo,
                                                                     const ExternalOrderRepository &externalOrderRepo,
                                                                     const EveDataProvider &dataProvider,
                                                                     ItemCostProvider &costProvider,
                                                                     const MarketOrderProvider &orderProvider,
                                                                     const MarketOrderProvider &corpOrderProvider,
                                                                     bool corp,
                                                                     QWidget *parent)
        : QWidget(parent)
        , mCharacterRepo(characterRepo)
        , mExternalOrderRepo(externalOrderRepo)
        , mDataProvider(dataProvider)
        , mCostProvider(costProvider)
        , mOrderProvider(orderProvider)
        , mCorpOrderProvider(corpOrderProvider)
        , mTransactionModel(transactionsRepo, mCharacterRepo, mDataProvider, corp)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mOrderView = new MarketOrderView{mDataProvider, this};
        mainLayout->addWidget(mOrderView, 1);
        connect(this, &MarketOrderViewWithTransactions::statusFilterChanged, mOrderView, &MarketOrderView::statusFilterChanged);
        connect(this, &MarketOrderViewWithTransactions::priceStatusFilterChanged, mOrderView, &MarketOrderView::priceStatusFilterChanged);
        connect(this, &MarketOrderViewWithTransactions::textFilterChanged, mOrderView, &MarketOrderView::textFilterChanged);
        connect(mOrderView, &MarketOrderView::scriptError, this, &MarketOrderViewWithTransactions::scriptError);
        connect(mOrderView->getSelectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MarketOrderViewWithTransactions::selectOrder);

        auto transactionGroup = new QGroupBox{this};
        mainLayout->addWidget(transactionGroup);

        auto groupLayout = new QVBoxLayout{};
        transactionGroup->setLayout(groupLayout);

        mTransactionProxyModel.setSortRole(Qt::UserRole);
        mTransactionProxyModel.setSourceModel(&mTransactionModel);

        auto addCostAct = new QAction{tr("Add to item costs"), this};
        connect(addCostAct, &QAction::triggered, this, &MarketOrderViewWithTransactions::addItemCost);

        auto tabs = new QTabWidget{this};
        groupLayout->addWidget(tabs);
        tabs->setTabPosition(QTabWidget::West);

        mExternalOrderView = new ExternalOrderView{mCostProvider, mDataProvider, this};
        tabs->addTab(mExternalOrderView, tr("Market orders"));

        mTransactionsView = new StyledTreeView{this};
        tabs->addTab(mTransactionsView, tr("Transactions"));
        mTransactionsView->setModel(&mTransactionProxyModel);
        mTransactionsView->sortByColumn(1, Qt::DescendingOrder);
        mTransactionsView->addAction(addCostAct);
        mTransactionsView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    void MarketOrderViewWithTransactions::setModel(MarketOrderModel *model)
    {
        mOrderModel = model;
        mOrderView->setModel(mOrderModel);

        if (mOrderModel == nullptr)
        {
            mExternalOrderModel.reset();
        }
        else
        {
            switch (mOrderModel->getType()) {
            case MarketOrderModel::Type::Buy:
                mExternalOrderModel = std::make_unique<ExternalOrderBuyModel>(mDataProvider,
                                                                              mExternalOrderRepo,
                                                                              mCharacterRepo,
                                                                              mOrderProvider,
                                                                              mCorpOrderProvider,
                                                                              mCostProvider);
                break;
            case MarketOrderModel::Type::Sell:
                mExternalOrderModel = std::make_unique<ExternalOrderSellModel>(mDataProvider,
                                                                               mExternalOrderRepo,
                                                                               mCharacterRepo,
                                                                               mOrderProvider,
                                                                               mCorpOrderProvider,
                                                                               mCostProvider);
                break;
            default:
                mExternalOrderModel.reset();
            }

            if (mExternalOrderModel)
            {
                mExternalOrderModel->setPriceColorMode(ExternalOrderModel::PriceColorMode::Deviation);
                connect(mOrderModel, &MarketOrderModel::modelReset, mExternalOrderModel.get(), &ExternalOrderModel::reset);
            }
        }

        mExternalOrderView->setModel(mExternalOrderModel.get());
    }

    void MarketOrderViewWithTransactions::setShowInfo(bool flag)
    {
        mOrderView->setShowInfo(flag);
    }

    void MarketOrderViewWithTransactions::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        mTransactionModel.clear();

        if (mExternalOrderModel)
            mExternalOrderModel->setCharacter(mCharacterId);
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
            const auto order = mOrderModel->getOrder(index);

            if (order == nullptr || order->getTypeId() == EveType::invalidId)
            {
                mTransactionModel.clear();
            }
            else
            {
                auto range = mOrderModel->getOrderRange(index);

                if (!range.mTo.isValid())
                    range.mTo = QDateTime::currentDateTimeUtc();

                mTransactionModel.setFilter(
                    mCharacterId, range.mFrom.date(), range.mTo.date(), mOrderModel->getOrderTypeFilter(index), order->getTypeId());
            }

            if (mExternalOrderModel)
            {
                mExternalOrderModel->blockSignals(true);

                if (order == nullptr)
                {
                    mExternalOrderModel->setTypeId(EveType::invalidId);
                    mExternalOrderModel->setStationId(0);
                }
                else
                {
                    mExternalOrderModel->setTypeId(order->getTypeId());
                    mExternalOrderModel->setStationId(order->getStationId());
                }

                mExternalOrderModel->changeDeviationSource(ExternalOrderModel::DeviationSourceType::Fixed, order->getPrice());
                mExternalOrderModel->blockSignals(false);

                mExternalOrderModel->reset();
            }
        }
    }

    void MarketOrderViewWithTransactions::addItemCost()
    {
        const auto selection = mTransactionsView->selectionModel()->selectedIndexes();

        struct ItemData
        {
            uint mQuantity;
            double mPrice;
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
