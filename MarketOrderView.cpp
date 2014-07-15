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
#include <QHeaderView>
#include <QTreeView>

#include "MarketOrderModel.h"

#include "MarketOrderView.h"

namespace Evernus
{
    MarketOrderView::MarketOrderView(QWidget *parent)
        : QWidget{parent}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mProxy.setSortRole(Qt::UserRole);
        mProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);

        mView = new QTreeView{this};
        mainLayout->addWidget(mView, 1);
        mView->setModel(&mProxy);
        mView->setSortingEnabled(true);
        mView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    void MarketOrderView::setModel(MarketOrderModel *model)
    {
        mProxy.setSourceModel(model);
    }
}
