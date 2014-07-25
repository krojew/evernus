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

#include "MarketOrderArchiveModel.h"
#include "MarketOrderSellModel.h"
#include "CharacterBoundWidget.h"
#include "MarketOrderBuyModel.h"
#include "ItemPriceImporter.h"

class QPushButton;
class QComboBox;

namespace Evernus
{
    class MarketOrderViewWithTransactions;
    class WalletTransactionRepository;
    class MarketOrderProvider;
    class CacheTimerProvider;
    class ItemCostProvider;
    class EveDataProvider;
    class DateRangeWidget;
    class MarketOrderView;

    class MarketOrderWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        MarketOrderWidget(const MarketOrderProvider &orderProvider,
                          const CacheTimerProvider &cacheTimerProvider,
                          const EveDataProvider &dataProvider,
                          const ItemCostProvider &itemCostProvider,
                          const WalletTransactionRepository &transactionsRepo,
                          QWidget *parent = nullptr);
        virtual ~MarketOrderWidget() = default;

    signals:
        void characterChanged(Character::IdType id);

        void importFromLogs(Character::IdType id);

        void importPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target);
        void importPricesFromFile(const ItemPriceImporter::TypeLocationPairs &target);

    public slots:
        void updateData();

    private slots:
        void changeGrouping();
        void saveChosenTab(int index);

        void prepareItemImportFromWeb();
        void prepareItemImportFromFile();

        void setArchiveRange(const QDate &from, const QDate &to);

    private:
        static const QString settingsLastTabkey;

        const MarketOrderProvider &mOrderProvider;

        MarketOrderViewWithTransactions *mSellView = nullptr;
        MarketOrderViewWithTransactions *mBuyView = nullptr;
        MarketOrderViewWithTransactions *mArchiveView = nullptr;
        MarketOrderView *mCombinedSellView = nullptr;
        MarketOrderView *mCombinedBuyView = nullptr;

        MarketOrderSellModel mSellModel;
        MarketOrderBuyModel mBuyModel;
        MarketOrderArchiveModel mArchiveModel;

        QPushButton *mLogImportBtn = nullptr;
        QComboBox *mGroupingCombo = nullptr;
        DateRangeWidget *mArchiveRangeEdit = nullptr;

        virtual void handleNewCharacter(Character::IdType id) override;

        ItemPriceImporter::TypeLocationPairs getImportTarget() const;
    };
}
