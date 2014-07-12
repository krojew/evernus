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
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>

#include "ItemCostRepository.h"

#include "ItemCostWidget.h"

namespace Evernus
{
    ItemCostWidget::ItemCostWidget(const ItemCostRepository &itemCostRepo,
                                   const EveDataProvider &eveDataProvider,
                                   QWidget *parent)
        : QWidget{parent}
        , mItemCostRepo{itemCostRepo}
        , mEveDataProvider{eveDataProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolbarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolbarLayout);

        auto proxy = new QSortFilterProxyModel{this};
        proxy->setSourceModel(&mModel);

        auto view = new QTreeView{this};
        mainLayout->addWidget(view, 1);
        view->setModel(proxy);
        view->setSortingEnabled(true);
    }

    void ItemCostWidget::setCharacter(Character::IdType id)
    {
        mModel.setQuery(mItemCostRepo.prepareQueryForCharacter(id));
    }
}
