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

#include "StyledTreeView.h"
#include "Character.h"

class QAbstractProxyModel;

namespace Evernus
{
    class WalletTransactionsModel;
    class ItemCostProvider;

    class WalletTransactionView
        : public StyledTreeView
    {
        Q_OBJECT

    public:
        explicit WalletTransactionView(ItemCostProvider &itemCostProvider, QWidget *parent = nullptr);
        WalletTransactionView(const QString &objectName, ItemCostProvider &itemCostProvider, QWidget *parent = nullptr);
        virtual ~WalletTransactionView() = default;

        virtual void setModel(QAbstractItemModel *model) override;

        void setModels(QAbstractProxyModel *proxy, WalletTransactionsModel *model);

        void setCharacter(Character::IdType id);

    private slots:
        void selectTransaction(const QItemSelection &selected);

        void addItemCost();
        void copySuggestedPrice() const;

    private:
        ItemCostProvider &mItemCostProvider;

        QAction *mCopySuggestedPriceAct = nullptr;

        QAbstractProxyModel *mProxy = nullptr;
        WalletTransactionsModel *mModel = nullptr;

        QModelIndex mCurrentTransaction;

        Character::IdType mCharacterId = Character::invalidId;

        double getSuggestedPrice(double price) const;

        void initialize();

        static QString getDefaultCopySuggestedPriceText();
    };
}
