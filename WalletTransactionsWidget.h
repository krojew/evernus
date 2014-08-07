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

#include "WalletTransactionsModel.h"
#include "CharacterBoundWidget.h"

class QSortFilterProxyModel;

namespace Evernus
{
    class WalletTransactionRepository;
    class WalletEntryFilterWidget;
    class FilterTextRepository;
    class CacheTimerProvider;
    class ItemCostProvider;
    class EveDataProvider;
    class StyledTreeView;

    class WalletTransactionsWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        typedef WalletTransactionsModel::EntryType EntryType;

        WalletTransactionsWidget(const WalletTransactionRepository &walletRepo,
                                 const FilterTextRepository &filterRepo,
                                 const CacheTimerProvider &cacheTimerProvider,
                                 const EveDataProvider &dataProvider,
                                 ItemCostProvider &itemCostProvider,
                                 bool corp,
                                 QWidget *parent = nullptr);
        virtual ~WalletTransactionsWidget() = default;

    public slots:
        void updateData();

        void updateFilter(const QDate &from, const QDate &to, const QString &filter, int type);

    private slots:
        void addItemCost();

    private:
        ItemCostProvider &mItemCostProvider;

        WalletTransactionsModel mModel;
        QSortFilterProxyModel *mFilterModel = nullptr;

        WalletEntryFilterWidget *mFilter = nullptr;

        StyledTreeView *mView = nullptr;

        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
