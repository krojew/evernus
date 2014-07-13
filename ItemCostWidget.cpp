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
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>

#include "ItemCostWidget.h"

namespace Evernus
{
    ItemCostWidget::ItemCostWidget(const ItemCostRepository &itemCostRepo,
                                   const EveDataProvider &eveDataProvider,
                                   QWidget *parent)
        : QWidget{parent}
        , mEveDataProvider{eveDataProvider}
        , mModel{itemCostRepo, mEveDataProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolbarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolbarLayout);

        auto addBtn = new QPushButton{QIcon{":/images/add.png"}, tr("Add..."), this};
        toolbarLayout->addWidget(addBtn);
        addBtn->setFlat(true);
        connect(addBtn, &QPushButton::clicked, this, &ItemCostWidget::addCost);

        mEditBtn = new QPushButton{QIcon{":/images/pencil.png"}, tr("Edit..."), this};
        toolbarLayout->addWidget(mEditBtn);
        mEditBtn->setFlat(true);
        mEditBtn->setDisabled(true);
        connect(mEditBtn, &QPushButton::clicked, this, &ItemCostWidget::editCost);

        mRemoveBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        toolbarLayout->addWidget(mRemoveBtn);
        mRemoveBtn->setFlat(true);
        mRemoveBtn->setDisabled(true);
        connect(mRemoveBtn, &QPushButton::clicked, this, &ItemCostWidget::deleteCost);

        toolbarLayout->addStretch();

        auto proxy = new QSortFilterProxyModel{this};
        proxy->setSortRole(Qt::UserRole);
        proxy->setSourceModel(&mModel);

        auto view = new QTreeView{this};
        mainLayout->addWidget(view, 1);
        view->setModel(proxy);
        view->setSortingEnabled(true);
        view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    void ItemCostWidget::setCharacter(Character::IdType id)
    {
        mModel.setCharacter(id);
    }

    void ItemCostWidget::addCost()
    {

    }

    void ItemCostWidget::editCost()
    {

    }

    void ItemCostWidget::deleteCost()
    {

    }
}
