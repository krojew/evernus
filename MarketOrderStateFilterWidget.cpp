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
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>

#include "MarketOrderStatesWidget.h"

#include "MarketOrderStateFilterWidget.h"

namespace Evernus
{
    MarketOrderStateFilterWidget::MarketOrderStateFilterWidget(QWidget *parent)
        : QWidget{parent}
    {
        auto mainLayout = new QHBoxLayout{};
        setLayout(mainLayout);

        auto statesWidget = new MarketOrderStatesWidget{this};
        connect(statesWidget, &MarketOrderStatesWidget::filterChanged, this, &MarketOrderStateFilterWidget::setFilter);

        auto filterAction = new QWidgetAction{this};
        filterAction->setDefaultWidget(statesWidget);

        auto filterMenu = new QMenu{this};
        filterMenu->addAction(filterAction);

        mFilterBtn = new QPushButton{QIcon{":/images/flag_green.png"}, getFilterButtonText(statesWidget->getStatusFilter()), this};
        mainLayout->addWidget(mFilterBtn);
        mFilterBtn->setFlat(true);
        mFilterBtn->setMenu(filterMenu);
    }

    void MarketOrderStateFilterWidget::setFilter(const MarketOrderFilterProxyModel::StatusFilters &filter)
    {
        mFilterBtn->setText(getFilterButtonText(filter));
        emit filterChanged(filter);
    }

    QString MarketOrderStateFilterWidget::getFilterButtonText(const MarketOrderFilterProxyModel::StatusFilters &filter) const
    {
        QStringList filters;
        if (filter & MarketOrderFilterProxyModel::Changed)
            filters << tr("Ch");
        if (filter & MarketOrderFilterProxyModel::Active)
            filters << tr("A");
        if (filter & MarketOrderFilterProxyModel::Fulfilled)
            filters << tr("F");
        if (filter & MarketOrderFilterProxyModel::Cancelled)
            filters << tr("C");
        if (filter & MarketOrderFilterProxyModel::Pending)
            filters << tr("P");
        if (filter & MarketOrderFilterProxyModel::CharacterDeleted)
            filters << tr("D");
        if (filter & MarketOrderFilterProxyModel::Expired)
            filters << tr("E");

        return (filters.isEmpty()) ? (tr("Status filter")) : (tr("Status filter [%1]").arg(filters.join(", ")));
    }
}
