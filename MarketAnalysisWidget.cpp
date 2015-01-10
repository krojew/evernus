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
#include <QStackedWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>
#include <QLabel>
#include <QDebug>

#include <boost/scope_exit.hpp>

#include "ExternalOrderRepository.h"
#include "RegionTypeSelectDialog.h"
#include "CharacterRepository.h"
#include "EveDataProvider.h"
#include "PriceSettings.h"
#include "TaskManager.h"
#include "UISettings.h"

#include "MarketAnalysisWidget.h"

namespace Evernus
{
    MarketAnalysisWidget::MarketAnalysisWidget(QByteArray crestClientId,
                                               QByteArray crestClientSecret,
                                               const EveDataProvider &dataProvider,
                                               TaskManager &taskManager,
                                               const ExternalOrderRepository &orderRepo,
                                               const EveTypeRepository &typeRepo,
                                               const MarketGroupRepository &groupRepo,
                                               const CharacterRepository &characterRepo,
                                               QWidget *parent)
        : QWidget(parent)
        , mDataProvider(dataProvider)
        , mTaskManager(taskManager)
        , mOrderRepo(orderRepo)
        , mTypeRepo(typeRepo)
        , mGroupRepo(groupRepo)
        , mCharacterRepo(characterRepo)
        , mManager(std::move(crestClientId), std::move(crestClientSecret), mDataProvider)
        , mTypeDataModel(mDataProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import orders"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &MarketAnalysisWidget::prepareOrderImport);

        QSettings settings;

        mDontSaveBtn = new QCheckBox{tr("Don't save imported orders (huge performance gain)"), this};
        toolBarLayout->addWidget(mDontSaveBtn);
        mDontSaveBtn->setChecked(
            settings.value(UISettings::dontSaveLargeOrdersKey, UISettings::dontSaveLargeOrdersDefault).toBool());
        connect(mDontSaveBtn, &QCheckBox::toggled, [](auto checked) {
            QSettings settings;
            settings.setValue(UISettings::dontSaveLargeOrdersKey, checked);
        });

        toolBarLayout->addWidget(new QLabel{tr("Region:"), this});

        mRegionCombo = new QComboBox{this};
        toolBarLayout->addWidget(mRegionCombo);
        mRegionCombo->setEditable(true);
        mRegionCombo->setInsertPolicy(QComboBox::NoInsert);

        const auto regions = mDataProvider.getRegions();
        for (const auto &region : regions)
            mRegionCombo->addItem(region.second, region.first);

        connect(mRegionCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showForCurrentRegion()));

        toolBarLayout->addWidget(new QLabel{tr("Limit to solar system:"), this});

        mSolarSystemCombo = new QComboBox{this};
        toolBarLayout->addWidget(mSolarSystemCombo);
        mSolarSystemCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        mSolarSystemCombo->setEditable(true);
        mSolarSystemCombo->setInsertPolicy(QComboBox::NoInsert);
        fillSolarSystems(mRegionCombo->currentData().toUInt());
        connect(mSolarSystemCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showForCurrentRegionAndSolarSystem()));

        toolBarLayout->addStretch();

        mDataStack = new QStackedWidget{this};
        mainLayout->addWidget(mDataStack);

        auto waitingLabel = new QLabel{tr("Calculating data..."), this};
        mDataStack->addWidget(waitingLabel);
        waitingLabel->setAlignment(Qt::AlignCenter);

        mTypeViewProxy.setSortRole(Qt::UserRole);
        mTypeViewProxy.setSourceModel(&mTypeDataModel);

        mTypeDataView = new QTableView{this};
        mDataStack->addWidget(mTypeDataView);
        mTypeDataView->setSortingEnabled(true);
        mTypeDataView->setAlternatingRowColors(true);
        mTypeDataView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mTypeDataView->setModel(&mTypeViewProxy);

        mDataStack->setCurrentWidget(mTypeDataView);
    }

    void MarketAnalysisWidget::setCharacter(Character::IdType id)
    {
        mTypeDataModel.setCharacter(mCharacterRepo.find(id));
    }

    void MarketAnalysisWidget::prepareOrderImport()
    {
        RegionTypeSelectDialog dlg{mDataProvider, mTypeRepo, mGroupRepo, this};
        connect(&dlg, &RegionTypeSelectDialog::selected, this, &MarketAnalysisWidget::importData);

        dlg.exec();
    }

    void MarketAnalysisWidget::importData(const ExternalOrderImporter::TypeLocationPairs &pairs)
    {
        if (pairs.empty())
        {
            QMessageBox::information(this, tr("Order import"), tr("Please select at least one region and type."));
            return;
        }

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        mOrders.clear();
        mHistory.clear();

        const auto mainTask = mTaskManager.startTask(tr("Importing data for analysis..."));

        mOrderSubtask = mTaskManager.startTask(mainTask, tr("Making %1 CREST order requests...").arg(pairs.size()));
        mHistorySubtask = mTaskManager.startTask(mainTask, tr("Making %1 CREST history requests...").arg(pairs.size()));

        for (const auto &pair : pairs)
        {
            ++mOrderRequestCount;
            ++mHistoryRequestCount;

            mManager.fetchMarketOrders(pair.second, pair.first, [this](auto &&orders, const auto &error) {
                processOrders(std::move(orders), error);
            });
            mManager.fetchMarketHistory(pair.second, pair.first, [pair, this](auto &&history, const auto &error) {
                processHistory(pair.second, pair.first, std::move(history), error);
            });
        }

        qDebug() << "Making" << mOrderRequestCount << "CREST order and history requests...";

        if (mOrderRequestCount == 0)
            mTaskManager.endTask(mOrderSubtask);
        if (mHistoryRequestCount == 0)
            mTaskManager.endTask(mHistorySubtask);

        checkCompletion();
    }

    void MarketAnalysisWidget::storeOrders()
    {
        emit updateExternalOrders(mOrders);

        mTaskManager.endTask(mOrderSubtask);
        checkCompletion();
    }

    void MarketAnalysisWidget::showForCurrentRegion()
    {
        const auto region = mRegionCombo->currentData().toUInt();
        if (region != 0)
        {
            mDataStack->setCurrentIndex(waitingLabelIndex);
            mDataStack->repaint();

            fillSolarSystems(region);
            mTypeDataModel.setData(mOrders, mHistory[region], region);

            mDataStack->setCurrentWidget(mTypeDataView);
        }
    }

    void MarketAnalysisWidget::showForCurrentRegionAndSolarSystem()
    {
        const auto region = mRegionCombo->currentData().toUInt();
        if (region != 0)
        {
            mDataStack->setCurrentIndex(waitingLabelIndex);
            mDataStack->repaint();

            const auto system = mSolarSystemCombo->currentData().toUInt();
            mTypeDataModel.setData(mOrders, mHistory[region], region, system);

            mDataStack->setCurrentWidget(mTypeDataView);
        }
    }

    void MarketAnalysisWidget::processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText)
    {
        --mOrderRequestCount;

        qDebug() << mOrderRequestCount << " order remaining; error:" << errorText;

        if ((mOrderRequestCount % 10) == 0)
            mTaskManager.updateTask(mOrderSubtask, tr("Waiting for %1 order server replies...").arg(mOrderRequestCount));

        if (!errorText.isEmpty())
        {
            if (mOrderRequestCount == 0)
            {
                mOrders.clear();
                mTaskManager.endTask(mOrderSubtask, mAggregatedOrderErrors.join("\n"));
                mAggregatedOrderErrors.clear();
            }
            else
            {
                mAggregatedOrderErrors << errorText;
            }

            return;
        }

        mOrders.reserve(mOrders.size() + orders.size());
        mOrders.insert(std::end(mOrders),
                       std::make_move_iterator(std::begin(orders)),
                       std::make_move_iterator(std::end(orders)));

        if (mOrderRequestCount == 0)
        {
            if (!mPreparingRequests)
            {
                if (mAggregatedOrderErrors.isEmpty())
                {
                    if (!mDontSaveBtn->isChecked())
                    {
                        mTaskManager.updateTask(mOrderSubtask, tr("Saving %1 imported orders...").arg(mOrders.size()));
                        QMetaObject::invokeMethod(this, "storeOrders", Qt::QueuedConnection);
                    }
                    else
                    {
                        mTaskManager.endTask(mOrderSubtask);
                        checkCompletion();
                    }
                }
                else
                {
                    mTaskManager.endTask(mOrderSubtask, mAggregatedOrderErrors.join("\n"));
                    mAggregatedOrderErrors.clear();
                }
            }
        }
    }

    void MarketAnalysisWidget
    ::processHistory(uint regionId, EveType::IdType typeId, std::map<QDate, MarketHistoryEntry> &&history, const QString &errorText)
    {
        --mHistoryRequestCount;

        qDebug() << mHistoryRequestCount << " history remaining; error:" << errorText;

        if ((mHistoryRequestCount % 10) == 0)
            mTaskManager.updateTask(mHistorySubtask, tr("Waiting for %1 history server replies...").arg(mHistoryRequestCount));

        if (!errorText.isEmpty())
        {
            if (mHistoryRequestCount == 0)
            {
                mTaskManager.endTask(mHistorySubtask, mAggregatedHistoryErrors.join("\n"));
                mAggregatedHistoryErrors.clear();
            }
            else
            {
                mAggregatedHistoryErrors << errorText;
            }

            return;
        }

        mHistory[regionId][typeId] = std::move(history);

        if (mHistoryRequestCount == 0)
        {
            if (!mPreparingRequests)
            {
                if (mAggregatedHistoryErrors.isEmpty())
                {
                    mTaskManager.endTask(mHistorySubtask);
                    checkCompletion();
                }
                else
                {
                    mTaskManager.endTask(mHistorySubtask, mAggregatedHistoryErrors.join("\n"));
                    mAggregatedHistoryErrors.clear();
                }
            }
        }
    }

    void MarketAnalysisWidget::checkCompletion()
    {
        if (mOrderRequestCount == 0 && mHistoryRequestCount == 0)
            showForCurrentRegion();
    }

    void MarketAnalysisWidget::fillSolarSystems(uint regionId)
    {
        mSolarSystemCombo->blockSignals(true);
        mSolarSystemCombo->clear();

        mSolarSystemCombo->addItem(tr("- all -"));

        const auto &systems = mDataProvider.getSolarSystemsForRegion(regionId);
        for (const auto &system : systems)
            mSolarSystemCombo->addItem(system.second, system.first);

        mSolarSystemCombo->setCurrentIndex(0);
        mSolarSystemCombo->blockSignals(false);
    }
}
