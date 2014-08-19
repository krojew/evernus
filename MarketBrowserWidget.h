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

#include <list>

#include <QWidget>

#include "ExternalOrderSellModel.h"
#include "ExternalOrderImporter.h"
#include "ItemNameModel.h"

class QListWidgetItem;
class QListWidget;
class QPushButton;
class QTabWidget;
class QListView;

namespace Evernus
{
    class ExternalOrderRepository;
    class MarketOrderRepository;
    class ExternalOrderView;
    class ItemCostProvider;
    class EveDataProvider;

    class MarketBrowserWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        MarketBrowserWidget(const ExternalOrderRepository &externalOrderRepo,
                            const MarketOrderRepository &orderRepo,
                            const MarketOrderRepository &corpOrderRepo,
                            const CharacterRepository &characterRepo,
                            const MarketOrderProvider &orderProvider,
                            const MarketOrderProvider &corpOrderProvider,
                            const EveDataProvider &dataProvider,
                            const ItemCostProvider &costProvider,
                            QWidget *parent = nullptr);
        virtual ~MarketBrowserWidget() = default;

    signals:
        void importPricesFromWeb(const ExternalOrderImporter::TypeLocationPairs &target);
        void importPricesFromFile(const ExternalOrderImporter::TypeLocationPairs &target);
        void importPricesFromCache(const ExternalOrderImporter::TypeLocationPairs &target);

    public slots:
        void setCharacter(Character::IdType id);

        void updateData();

        void fillOrderItemNames();

    private slots:
        void prepareItemImportFromWeb();
        void prepareItemImportFromFile();
        void prepareItemImportFromCache();

        void selectRegion(QListWidgetItem *item);
        void selectSolarSystem(QListWidgetItem *item);
        void selectStation(QListWidgetItem *item);

        void changeDeviationSource(ExternalOrderModel::DeviationSourceType type, double value);

        void stepBack();
        void stepForward();

    private:
        struct NavigationState
        {
            EveType::IdType mTypeId = EveType::invalidId;
            uint mRegionId = 0;
            uint mSolarSystemId = 0;
            uint mStationId = 0;
        };

        typedef std::list<NavigationState> NavigationStack;

        static const auto knownItemsTab = 0;
        static const auto orderItemsTab = 0;

        const ExternalOrderRepository &mExternalOrderRepo;
        const MarketOrderRepository &mOrderRepo;
        const MarketOrderRepository &mCorpOrderRepo;
        const EveDataProvider &mDataProvider;

        ItemNameModel mNameModel;
        ItemNameModel mOrderNameModel;

        ExternalOrderSellModel mExternalOrderSellModel;

        QPushButton *mDeviationBtn = nullptr;

        QPushButton *mBackBtn = nullptr;
        QPushButton *mForwardBtn = nullptr;

        QTabWidget *mItemTabs = nullptr;

        QListView *mKnownItemList = nullptr;
        QListView *mOrderItemList = nullptr;

        QListWidget *mRegionList = nullptr;
        QListWidget *mSolarSystemList = nullptr;
        QListWidget *mStationList = nullptr;

        ExternalOrderView *mSellView = nullptr;
        ExternalOrderView *mBuyView = nullptr;

        NavigationStack mNavigationStack;
        NavigationStack::const_iterator mNagivationPointer = std::begin(mNavigationStack);
        bool mBlockNavigationChange = false;

        ExternalOrderImporter::TypeLocationPairs getImportTarget() const;

        void fillRegions();
        void fillKnownItemNames();
        void showOrdersForType(EveType::IdType typeId);

        QWidget *createItemNameListTab(ItemNameModel &model, QListView *&view);

        QString getDeviationButtonText(ExternalOrderModel::DeviationSourceType type) const;

        void updateNavigationButtons();
        void saveNavigationState();
        void restoreNavigationState();

        void setTypeId(EveType::IdType typeId);
    };
}
