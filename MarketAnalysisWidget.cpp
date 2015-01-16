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
#include <boost/scope_exit.hpp>

#include <QStackedWidget>
#include <QIntValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSettings>
#include <QLabel>
#include <QDebug>

#include "TypeAggregatedDetailsWidget.h"
#include "RegionTypeSelectDialog.h"
#include "MarketAnalysisSettings.h"
#include "MarketOrderRepository.h"
#include "CharacterRepository.h"
#include "EveDataProvider.h"
#include "PriceSettings.h"
#include "TaskManager.h"

#include "MarketAnalysisWidget.h"

namespace Evernus
{
    MarketAnalysisWidget::MarketAnalysisWidget(QByteArray crestClientId,
                                               QByteArray crestClientSecret,
                                               const EveDataProvider &dataProvider,
                                               TaskManager &taskManager,
                                               const MarketOrderRepository &orderRepo,
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
        , mTypeViewProxy(TypeAggregatedMarketDataModel::getVolumeColumn(), TypeAggregatedMarketDataModel::getMarginColumn())
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import data"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &MarketAnalysisWidget::prepareOrderImport);

        QSettings settings;

        mDontSaveBtn = new QCheckBox{tr("Don't save imported orders (huge performance gain)"), this};
        toolBarLayout->addWidget(mDontSaveBtn);
        mDontSaveBtn->setChecked(
            settings.value(MarketAnalysisSettings::dontSaveLargeOrdersKey, MarketAnalysisSettings::dontSaveLargeOrdersDefault).toBool());
        connect(mDontSaveBtn, &QCheckBox::toggled, [](auto checked) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::dontSaveLargeOrdersKey, checked);
        });

        mIgnoreExistingOrdersBtn = new QCheckBox{tr("Ignore types with existing orders"), this};
        toolBarLayout->addWidget(mIgnoreExistingOrdersBtn);
        mIgnoreExistingOrdersBtn->setChecked(
            settings.value(MarketAnalysisSettings::ignoreExistingOrdersKey, MarketAnalysisSettings::ignoreExistingOrdersDefault).toBool());
        connect(mIgnoreExistingOrdersBtn, &QCheckBox::toggled, [](auto checked) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::ignoreExistingOrdersKey, checked);
        });

        toolBarLayout->addWidget(new QLabel{tr("Region:"), this});

        mRegionCombo = new QComboBox{this};
        toolBarLayout->addWidget(mRegionCombo);
        mRegionCombo->setEditable(true);
        mRegionCombo->setInsertPolicy(QComboBox::NoInsert);

        const auto lastRegion = settings.value(MarketAnalysisSettings::lastRegionKey).toUInt();

        const auto regions = mDataProvider.getRegions();
        for (const auto &region : regions)
        {
            mRegionCombo->addItem(region.second, region.first);
            if (region.first == lastRegion)
                mRegionCombo->setCurrentIndex(mRegionCombo->count() - 1);
        }

        connect(mRegionCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showForCurrentRegion()));

        toolBarLayout->addWidget(new QLabel{tr("Limit to solar system:"), this});

        mSolarSystemCombo = new QComboBox{this};
        toolBarLayout->addWidget(mSolarSystemCombo);
        mSolarSystemCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        mSolarSystemCombo->setEditable(true);
        mSolarSystemCombo->setInsertPolicy(QComboBox::NoInsert);
        fillSolarSystems(mRegionCombo->currentData().toUInt());
        connect(mSolarSystemCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showForCurrentRegionAndSolarSystem()));

        auto minVolumeValidator = new QIntValidator{this};
        minVolumeValidator->setBottom(0);

        toolBarLayout->addWidget(new QLabel{tr("Volume:"), this});

        auto value = settings.value(MarketAnalysisSettings::minVolumeFilterKey);

        mMinVolumeEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinVolumeEdit);
        mMinVolumeEdit->setValidator(minVolumeValidator);

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxVolumeFilterKey);

        mMaxVolumeEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxVolumeEdit);
        mMaxVolumeEdit->setValidator(minVolumeValidator);

        auto minMarginValidator = new QIntValidator{this};

        toolBarLayout->addWidget(new QLabel{tr("Margin:"), this});

        value = settings.value(MarketAnalysisSettings::minMarginFilterKey);

        mMinMarginEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinMarginEdit);
        mMinMarginEdit->setValidator(minMarginValidator);
        mMinMarginEdit->setPlaceholderText(locale().percent());

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxMarginFilterKey);

        mMaxMarginEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxMarginEdit);
        mMaxMarginEdit->setValidator(minMarginValidator);
        mMaxMarginEdit->setPlaceholderText(locale().percent());

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &MarketAnalysisWidget::applyFilter);

        toolBarLayout->addStretch();

        mainLayout->addWidget(new QLabel{tr("Double-click an item for additional information."), this});

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
        connect(mTypeDataView, &QTableView::doubleClicked, this, &MarketAnalysisWidget::showDetails);

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
        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        mOrders.clear();
        mHistory.clear();

        const auto mainTask = mTaskManager.startTask(tr("Importing data for analysis..."));

        mOrderSubtask = mTaskManager.startTask(mainTask, tr("Making %1 CREST order requests...").arg(pairs.size()));
        mHistorySubtask = mTaskManager.startTask(mainTask, tr("Making %1 CREST history requests...").arg(pairs.size()));

        MarketOrderRepository::TypeLocationPairs ignored;
        if (mIgnoreExistingOrdersBtn->isChecked())
        {
            const auto temp = mOrderRepo.fetchActiveTypes();
            for (const auto &pair : temp)
                ignored.insert(std::make_pair(pair.first, mDataProvider.getStationRegionId(pair.second)));
        }

        for (const auto &pair : pairs)
        {
            if (ignored.find(pair) != std::end(ignored))
                continue;

            ++mOrderRequestCount;
            ++mHistoryRequestCount;

            mManager.fetchMarketOrders(pair.second, pair.first, [this](auto &&orders, const auto &error) {
                processOrders(std::move(orders), error);
            });
            mManager.fetchMarketHistory(pair.second, pair.first, [pair, this](auto &&history, const auto &error) {
                processHistory(pair.second, pair.first, std::move(history), error);
            });
        }

        qDebug() << "Making" << mOrderRequestCount << mHistoryRequestCount << "CREST order and history requests...";

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
        const auto region = getCurrentRegion();
        if (region != 0)
        {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::lastRegionKey, region);

            mDataStack->setCurrentIndex(waitingLabelIndex);
            mDataStack->repaint();

            fillSolarSystems(region);
            mTypeDataModel.setOrderData(mOrders, mHistory[region], region);

            mDataStack->setCurrentWidget(mTypeDataView);
        }
    }

    void MarketAnalysisWidget::showForCurrentRegionAndSolarSystem()
    {
        const auto region = getCurrentRegion();
        if (region != 0)
        {
            mDataStack->setCurrentIndex(waitingLabelIndex);
            mDataStack->repaint();

            const auto system = mSolarSystemCombo->currentData().toUInt();
            mTypeDataModel.setOrderData(mOrders, mHistory[region], region, system);

            mDataStack->setCurrentWidget(mTypeDataView);
        }
    }

    void MarketAnalysisWidget::applyFilter()
    {
        const auto minFilter = mMinVolumeEdit->text();
        const auto maxFilter = mMaxVolumeEdit->text();
        const auto minMargin = mMinMarginEdit->text();
        const auto maxMargin = mMaxMarginEdit->text();

        mTypeViewProxy.setFilter((minFilter.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::VolumeValueType{}) : (minFilter.toUInt()),
                                 (maxFilter.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::VolumeValueType{}) : (maxFilter.toUInt()),
                                 (minMargin.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::MarginValueType{}) : (minMargin.toDouble()),
                                 (maxMargin.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::MarginValueType{}) : (maxMargin.toDouble()));
    }

    void MarketAnalysisWidget::showDetails(const QModelIndex &item)
    {
        const auto id = mTypeDataModel.getTypeId(mTypeViewProxy.mapToSource(item));
        const auto region = getCurrentRegion();
        const auto &history = mHistory[region];
        const auto it = history.find(id);

        if (it != std::end(history))
        {
            auto widget = new TypeAggregatedDetailsWidget{it->second, this, Qt::Window};
            widget->setWindowTitle(tr("%1 in %2").arg(mDataProvider.getTypeName(id)).arg(mDataProvider.getRegionName(region)));
            widget->show();
            connect(this, &MarketAnalysisWidget::preferencesChanged, widget, &TypeAggregatedDetailsWidget::handleNewPreferences);
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

    uint MarketAnalysisWidget::getCurrentRegion() const
    {
        return mRegionCombo->currentData().toUInt();
    }
}
