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

#include <QWidget>

#include "InterRegionMarketDataFilterProxyModel.h"
#include "InterRegionMarketDataModel.h"
#include "MarketAnalysisDataFetcher.h"
#include "ExternalOrderImporter.h"
#include "MarketDataProvider.h"
#include "ExternalOrder.h"
#include "TaskConstants.h"

class QAbstractItemView;
class QStackedWidget;
class QPushButton;
class QTableView;
class QComboBox;
class QCheckBox;
class QLineEdit;

namespace Evernus
{
    class MarketOrderRepository;
    class MarketGroupRepository;
    class RegionAnalysisWidget;
    class CharacterRepository;
    class AdjustableTableView;
    class EveTypeRepository;
    class EveDataProvider;
    class TaskManager;

    class MarketAnalysisWidget
        : public QWidget
        , public MarketDataProvider
    {
        Q_OBJECT

    public:
        MarketAnalysisWidget(const QByteArray &clientId,
                             const QByteArray &clientSecret,
                             const EveDataProvider &dataProvider,
                             TaskManager &taskManager,
                             const MarketOrderRepository &orderRepo,
                             const MarketOrderRepository &corpOrderRepo,
                             const EveTypeRepository &typeRepo,
                             const MarketGroupRepository &groupRepo,
                             const CharacterRepository &characterRepo,
                             QWidget *parent = nullptr);
        virtual ~MarketAnalysisWidget() = default;

        virtual const HistoryMap *getHistory(uint regionId) const override;
        virtual const OrderResultType *getOrders() const override;

    signals:
        void updateExternalOrders(const std::vector<ExternalOrder> &orders);
        void preferencesChanged();

        void showInEve(EveType::IdType id, Character::IdType ownerId);

        void handleNewPreferences();

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void prepareOrderImport();

        void importData(const ExternalOrderImporter::TypeLocationPairs &pairs);
        void storeOrders();

        void showForCurrentRegion();

        void applyInterRegionFilter();

        void selectInterRegionType(const QItemSelection &selected);

        void showInEveForCurrentInterRegion();

        void copyRows(const QAbstractItemView &view, const QAbstractItemModel &model) const;

        void updateOrderTask(const QString &text);
        void updateHistoryTask(const QString &text);

        void endOrderTask(const MarketAnalysisDataFetcher::OrderResultType &orders, const QString &error);
        void endHistoryTask(const MarketAnalysisDataFetcher::HistoryResultType &history, const QString &error);

    private:
        static const auto waitingLabelIndex = 0;
        static const auto allRegionsIndex = 0;

        const EveDataProvider &mDataProvider;
        TaskManager &mTaskManager;
        const MarketOrderRepository &mOrderRepo;
        const MarketOrderRepository &mCorpOrderRepo;
        const EveTypeRepository &mTypeRepo;
        const MarketGroupRepository &mGroupRepo;
        const CharacterRepository &mCharacterRepo;

        QAction *mShowInEveInterRegionAct = nullptr;
        QAction *mCopyInterRegionRowsAct = nullptr;

        RegionAnalysisWidget *mRegionAnalysisWidget = nullptr;

        QCheckBox *mDontSaveBtn = nullptr;
        QCheckBox *mIgnoreExistingOrdersBtn = nullptr;
        QComboBox *mSrcPriceTypeCombo = nullptr;
        QComboBox *mDstPriceTypeCombo = nullptr;

        QComboBox *mSourceRegionCombo = nullptr;
        QComboBox *mDestRegionCombo = nullptr;
        QLineEdit *mMinInterRegionVolumeEdit = nullptr;
        QLineEdit *mMaxInterRegionVolumeEdit = nullptr;
        QLineEdit *mMinInterRegionMarginEdit = nullptr;
        QLineEdit *mMaxInterRegionMarginEdit = nullptr;
        QStackedWidget *mInterRegionDataStack = nullptr;
        AdjustableTableView *mInterRegionTypeDataView = nullptr;

        uint mOrderSubtask = TaskConstants::invalidTask;
        uint mHistorySubtask = TaskConstants::invalidTask;

        MarketAnalysisDataFetcher::OrderResultType mOrders;
        MarketAnalysisDataFetcher::HistoryResultType mHistory;

        InterRegionMarketDataModel mInterRegionDataModel;
        InterRegionMarketDataFilterProxyModel mInterRegionViewProxy;
        bool mRefreshedInterRegionData = false;

        quint64 mSrcStation = 0;
        quint64 mDstStation = 0;

        MarketAnalysisDataFetcher mDataFetcher;

        Character::IdType mCharacterId = Character::invalidId;

        void checkCompletion();
        void changeStation(quint64 &destination, QPushButton &btn, const QString &settingName);
        void recalculateInterRegionData();
        void recalculateAllData();

        QWidget *createInterRegionAnalysisWidget();

        static PriceType getPriceType(const QComboBox &combo);
    };
}
