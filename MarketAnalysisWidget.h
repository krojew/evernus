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

#include "TypeAggregatedMarketDataFilterProxyModel.h"
#include "TypeAggregatedMarketDataModel.h"
#include "ExternalOrderImporter.h"
#include "ExternalOrder.h"
#include "TaskConstants.h"
#include "CRESTManager.h"

class QStackedWidget;
class QTableView;
class QComboBox;
class QCheckBox;
class QLineEdit;

namespace Evernus
{
    class MarketOrderRepository;
    class MarketGroupRepository;
    class CharacterRepository;
    class EveTypeRepository;
    class EveDataProvider;
    class TaskManager;

    class MarketAnalysisWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        MarketAnalysisWidget(QByteArray crestClientId,
                             QByteArray crestClientSecret,
                             const EveDataProvider &dataProvider,
                             TaskManager &taskManager,
                             const MarketOrderRepository &orderRepo,
                             const EveTypeRepository &typeRepo,
                             const MarketGroupRepository &groupRepo,
                             const CharacterRepository &characterRepo,
                             QWidget *parent = nullptr);
        virtual ~MarketAnalysisWidget() = default;

    signals:
        void updateExternalOrders(const std::vector<ExternalOrder> &orders);
        void preferencesChanged();

        void showInEve(EveType::IdType id);

    public slots:
        void setCharacter(Character::IdType id);

        void handleNewPreferences();

    private slots:
        void prepareOrderImport();

        void importData(const ExternalOrderImporter::TypeLocationPairs &pairs);
        void storeOrders();

        void showForCurrentRegion();
        void showForCurrentRegionAndSolarSystem();

        void applyFilter();

        void showDetails(const QModelIndex &item);
        void selectType(const QItemSelection &selected);

        void showDetailsForCurrent();
        void showInEveForCurrent();

    private:
        static const auto waitingLabelIndex = 0;

        const EveDataProvider &mDataProvider;
        TaskManager &mTaskManager;
        const MarketOrderRepository &mOrderRepo;
        const EveTypeRepository &mTypeRepo;
        const MarketGroupRepository &mGroupRepo;
        const CharacterRepository &mCharacterRepo;

        CRESTManager mManager;

        QAction *mShowDetailsAct = nullptr;
        QAction *mShowInEveAct = nullptr;

        QCheckBox *mDontSaveBtn = nullptr;
        QCheckBox *mIgnoreExistingOrdersBtn = nullptr;
        QComboBox *mRegionCombo = nullptr;
        QComboBox *mSolarSystemCombo = nullptr;
        QStackedWidget *mDataStack = nullptr;
        QTableView *mTypeDataView = nullptr;
        QLineEdit *mMinVolumeEdit = nullptr;
        QLineEdit *mMaxVolumeEdit = nullptr;
        QLineEdit *mMinMarginEdit = nullptr;
        QLineEdit *mMaxMarginEdit = nullptr;

        uint mOrderRequestCount = 0, mHistoryRequestCount = 0;
        bool mPreparingRequests = false;

        uint mOrderSubtask = TaskConstants::invalidTask;
        uint mHistorySubtask = TaskConstants::invalidTask;

        std::vector<ExternalOrder> mOrders;
        std::unordered_map<uint, TypeAggregatedMarketDataModel::HistoryMap> mHistory;

        QStringList mAggregatedOrderErrors, mAggregatedHistoryErrors;

        TypeAggregatedMarketDataModel mTypeDataModel;
        TypeAggregatedMarketDataFilterProxyModel mTypeViewProxy;

        void processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText);
        void processHistory(uint regionId, EveType::IdType typeId, std::map<QDate, MarketHistoryEntry> &&history, const QString &errorText);

        void checkCompletion();

        void fillSolarSystems(uint regionId);

        uint getCurrentRegion() const;
    };
}
