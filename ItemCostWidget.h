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
#pragma once

#include <QSqlQueryModel>
#include <QWidget>

#include "ItemCostModel.h"
#include "Character.h"

class QItemSelection;
class QPushButton;

namespace Evernus
{
    class ItemCostRepository;
    class EveDataProvider;
    class ItemCost;
    class EveType;

    class ItemCostWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        ItemCostWidget(const ItemCostRepository &itemCostRepo,
                       const EveDataProvider &eveDataProvider,
                       QWidget *parent = nullptr);
        virtual ~ItemCostWidget() = default;

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void addCost();
        void editCost();
        void deleteCost();

        void selectCost(const QItemSelection &selected, const QItemSelection &deselected);

    private:
        const ItemCostRepository &mItemCostRepo;
        const EveDataProvider &mEveDataProvider;

        QPushButton *mAddBtn = nullptr;
        QPushButton *mEditBtn = nullptr;
        QPushButton *mRemoveBtn = nullptr;

        ItemCostModel mModel;

        Character::IdType mCharacterId = Character::invalidId;

        void showCostEditDialog(ItemCost &cost);
    };
}
