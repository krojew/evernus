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

#include "MarketOrderViewWithTransactions.h"
#include "CacheTimerProvider.h"
#include "ButtonWithTimer.h"

#include "MarketOrderWidget.h"

namespace Evernus
{
    MarketOrderWidget::MarketOrderWidget(const MarketOrderProvider &orderProvider,
                                         const CacheTimerProvider &cacheTimerProvider,
                                         const EveDataProvider &dataProvider,
                                         const ItemCostProvider &itemCostProvider,
                                         QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, CacheTimerProvider::TimerType::MarketOrders),
                               parent}
        , mSellModel{orderProvider, dataProvider, itemCostProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto mainTabs = new QTabWidget{this};
        mainLayout->addWidget(mainTabs);

        auto sellView = new MarketOrderViewWithTransactions{this};
        mainTabs->addTab(sellView, QIcon{":/images/arrow_out.png"}, tr("Sell"));
        sellView->setModel(&mSellModel);
    }

    void MarketOrderWidget::updateData()
    {
        refreshImportTimer();

        mSellModel.reset();
    }

    void MarketOrderWidget::handleNewCharacter(Character::IdType id)
    {
        mSellModel.setCharacter(id);
    }
}
