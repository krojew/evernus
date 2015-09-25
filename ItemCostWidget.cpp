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
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>

#include "ItemCostEditDialog.h"
#include "ItemCostProvider.h"
#include "StyledTreeView.h"

#include "ItemCostWidget.h"

namespace Evernus
{
    ItemCostWidget::ItemCostWidget(const ItemCostProvider &costProvider,
                                   const EveDataProvider &eveDataProvider,
                                   QWidget *parent)
        : QWidget(parent)
        , mCostProvider(costProvider)
        , mEveDataProvider(eveDataProvider)
        , mModel(mCostProvider, mEveDataProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        mAddBtn = new QPushButton{QIcon{":/images/add.png"}, tr("Add..."), this};
        toolBarLayout->addWidget(mAddBtn);
        mAddBtn->setFlat(true);
        connect(mAddBtn, &QPushButton::clicked, this, &ItemCostWidget::addCost);

        mEditBtn = new QPushButton{QIcon{":/images/pencil.png"}, tr("Edit..."), this};
        toolBarLayout->addWidget(mEditBtn);
        mEditBtn->setFlat(true);
        mEditBtn->setDisabled(true);
        connect(mEditBtn, &QPushButton::clicked, this, &ItemCostWidget::editCost);

        mRemoveBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        toolBarLayout->addWidget(mRemoveBtn);
        mRemoveBtn->setFlat(true);
        mRemoveBtn->setDisabled(true);
        connect(mRemoveBtn, &QPushButton::clicked, this, &ItemCostWidget::deleteCost);

        auto deleteAllBtn = new QPushButton{QIcon{":/images/cross.png"}, tr("Remove all"), this};
        toolBarLayout->addWidget(deleteAllBtn);
        deleteAllBtn->setFlat(true);
        connect(deleteAllBtn, &QPushButton::clicked, this, &ItemCostWidget::deleteAllCost);

        mFilterEdit = new QLineEdit{this};
        toolBarLayout->addWidget(mFilterEdit, 1);
        mFilterEdit->setPlaceholderText(tr("type in wildcard and press Enter"));
        mFilterEdit->setClearButtonEnabled(true);
        connect(mFilterEdit, &QLineEdit::returnPressed, this, &ItemCostWidget::applyWildcard);

        mProxy.setSortRole(Qt::UserRole);
        mProxy.setSourceModel(&mModel);
        mProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);

        mView = new StyledTreeView{"itemCostView", this};
        mainLayout->addWidget(mView, 1);
        mView->setModel(&mProxy);
        connect(mView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &ItemCostWidget::selectCost);
    }

    void ItemCostWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        mModel.setCharacter(mCharacterId);

        mView->header()->resizeSections(QHeaderView::ResizeToContents);

        mAddBtn->setDisabled(mCharacterId == Character::invalidId);
        mEditBtn->setDisabled(true);
        mRemoveBtn->setDisabled(true);
    }

    void ItemCostWidget::updateData()
    {
        if (mBlockUpdate)
            return;

        mModel.reset();
        mView->header()->resizeSections(QHeaderView::ResizeToContents);

        mEditBtn->setDisabled(true);
        mRemoveBtn->setDisabled(true);
    }

    void ItemCostWidget::addCost()
    {
        ItemCost cost;
        cost.setCharacterId(mCharacterId);

        showCostEditDialog(cost);
    }

    void ItemCostWidget::editCost()
    {
        Q_ASSERT(mSelectedCosts.count() > 0);

        const auto index = mProxy.mapToSource(mSelectedCosts.first());
        const auto id = mModel.getId(index.row());

        try
        {
            auto cost = mCostProvider.findItemCost(id);
            showCostEditDialog(*cost);
        }
        catch (const ItemCostProvider::NotFoundException &)
        {
        }
    }

    void ItemCostWidget::deleteCost()
    {
        Q_ASSERT(mSelectedCosts.count() > 0);

        mBlockUpdate = true;

        try
        {
            for (const auto &index : mSelectedCosts)
            {
                const auto realIndex = mProxy.mapToSource(index);
                if (realIndex.column() != 0)
                    continue;

                const auto id = mModel.getId(realIndex.row());
                mCostProvider.removeItemCost(id);
            }
        }
        catch (...)
        {
            mBlockUpdate = false;
            updateData();

            throw;
        }

        mBlockUpdate = false;
        updateData();
    }

    void ItemCostWidget::deleteAllCost()
    {
        mCostProvider.removeAllItemCosts(mCharacterId);
    }

    void ItemCostWidget::selectCost(const QItemSelection &selected, const QItemSelection &deselected)
    {
        Q_UNUSED(selected);
        Q_UNUSED(deselected);

        mEditBtn->setDisabled(selected.isEmpty());
        mRemoveBtn->setDisabled(selected.isEmpty());

        mSelectedCosts = mView->selectionModel()->selectedIndexes();
    }

    void ItemCostWidget::applyWildcard()
    {
        mProxy.setFilterWildcard(mFilterEdit->text());
    }

    void ItemCostWidget::showCostEditDialog(ItemCost &cost)
    {
        ItemCostEditDialog dlg{cost, mEveDataProvider, this};
        if (dlg.exec() == QDialog::Accepted)
            mCostProvider.storeItemCost(cost);
    }
}
