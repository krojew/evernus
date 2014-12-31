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

#include "ExternalOrderImporter.h"
#include "ExternalOrder.h"
#include "TaskConstants.h"
#include "CRESTManager.h"

class QCheckBox;

namespace Evernus
{
    class ExternalOrderRepository;
    class MarketGroupRepository;
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
                             const ExternalOrderRepository &orderRepo,
                             const EveTypeRepository &typeRepo,
                             const MarketGroupRepository &groupRepo,
                             QWidget *parent = nullptr);
        virtual ~MarketAnalysisWidget() = default;

    signals:
        void updateExternalOrders(const std::vector<ExternalOrder> &orders);

    private slots:
        void prepareOrderImport();

        void importOrders(const ExternalOrderImporter::TypeLocationPairs &pairs);
        void storeOrders();

    private:
        const EveDataProvider &mDataProvider;
        TaskManager &mTaskManager;
        const ExternalOrderRepository &mOrderRepo;
        const EveTypeRepository &mTypeRepo;
        const MarketGroupRepository &mGroupRepo;

        CRESTManager mManager;

        QCheckBox *mIgnoreExistingOrdersBtn = nullptr;
        QCheckBox *mDontSaveBtn = nullptr;

        uint mOrderRequestCount = 0, mHistoryRequestCount = 0;
        bool mPreparingRequests = false;

        uint mOrderSubtask = TaskConstants::invalidTask;
        uint mHistorySubtask = TaskConstants::invalidTask;

        std::vector<ExternalOrder> mResult;
        QStringList mAggregatedOrderErrors, mAggregatedHistoryErrors;

        void processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText);
        void processHistory(std::map<QDate, MarketHistoryEntry> &&history, const QString &errorText);
    };
}
