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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QLabel>

#include "MarketOrderViewWithTransactions.h"
#include "MarketOrderFilterWidget.h"
#include "CacheTimerProvider.h"
#include "ButtonWithTimer.h"

#include "MarketOrderWidget.h"

namespace Evernus
{
    MarketOrderWidget::MarketOrderWidget(const MarketOrderProvider &orderProvider,
                                         const CacheTimerProvider &cacheTimerProvider,
                                         const EveDataProvider &dataProvider,
                                         const ItemCostProvider &itemCostProvider,
                                         const WalletTransactionRepository &transactionsRepo,
                                         QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::MarketOrders),
                               parent}
        , mSellModel{orderProvider, dataProvider, itemCostProvider, cacheTimerProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto stateFilter = new MarketOrderFilterWidget{this};
        toolBarLayout->addWidget(stateFilter);

        toolBarLayout->addWidget(new QLabel{tr("Group by:"), this});

        mGroupingCombo = new QComboBox{this};
        toolBarLayout->addWidget(mGroupingCombo);
        mGroupingCombo->addItem(tr("- none -"), static_cast<int>(MarketOrderModel::Grouping::None));
        mGroupingCombo->addItem(tr("Type"), static_cast<int>(MarketOrderModel::Grouping::Type));
        mGroupingCombo->addItem(tr("Group"), static_cast<int>(MarketOrderModel::Grouping::Group));
        mGroupingCombo->addItem(tr("Station"), static_cast<int>(MarketOrderModel::Grouping::Station));
        connect(mGroupingCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeGrouping()));

        auto mainTabs = new QTabWidget{this};
        mainLayout->addWidget(mainTabs);

        mSellView = new MarketOrderViewWithTransactions{transactionsRepo, dataProvider, this};
        mainTabs->addTab(mSellView, QIcon{":/images/arrow_out.png"}, tr("Sell"));
        mSellView->setModel(&mSellModel);
        connect(this, &MarketOrderWidget::characterChanged, mSellView, &MarketOrderViewWithTransactions::setCharacter);
        connect(stateFilter, &MarketOrderFilterWidget::statusFilterChanged, mSellView, &MarketOrderViewWithTransactions::statusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::priceStatusFilterChanged, mSellView, &MarketOrderViewWithTransactions::priceStatusFilterChanged);

        connect(this, &MarketOrderWidget::characterChanged, &mSellModel, &MarketOrderSellModel::setCharacter);
    }

    void MarketOrderWidget::updateData()
    {
        refreshImportTimer();
        mSellModel.reset();
    }

    void MarketOrderWidget::changeGrouping()
    {
        mSellModel.setGrouping(static_cast<MarketOrderModel::Grouping>(mGroupingCombo->currentData().toInt()));
        mSellView->expandAll();
    }

    void MarketOrderWidget::handleNewCharacter(Character::IdType id)
    {
        emit characterChanged(id);
    }
}
