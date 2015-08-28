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

#include <QDoubleValidator>
#include <QStackedWidget>
#include <QIntValidator>
#include <QApplication>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QClipboard>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSettings>
#include <QAction>
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
#include "FlowLayout.h"

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
        , mTypeViewProxy(TypeAggregatedMarketDataModel::getVolumeColumn(),
                         TypeAggregatedMarketDataModel::getMarginColumn(),
                         TypeAggregatedMarketDataModel::getBuyPriceColumn(),
                         TypeAggregatedMarketDataModel::getSellPriceColumn())
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new FlowLayout{};
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

        auto volumeValidator = new QIntValidator{this};
        volumeValidator->setBottom(0);

        toolBarLayout->addWidget(new QLabel{tr("Volume:"), this});

        auto value = settings.value(MarketAnalysisSettings::minVolumeFilterKey);

        mMinVolumeEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinVolumeEdit);
        mMinVolumeEdit->setValidator(volumeValidator);

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxVolumeFilterKey);

        mMaxVolumeEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxVolumeEdit);
        mMaxVolumeEdit->setValidator(volumeValidator);

        auto marginValidator = new QIntValidator{this};

        toolBarLayout->addWidget(new QLabel{tr("Margin:"), this});

        value = settings.value(MarketAnalysisSettings::minMarginFilterKey);

        mMinMarginEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinMarginEdit);
        mMinMarginEdit->setValidator(marginValidator);
        mMinMarginEdit->setPlaceholderText(locale().percent());

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxMarginFilterKey);

        mMaxMarginEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxMarginEdit);
        mMaxMarginEdit->setValidator(marginValidator);
        mMaxMarginEdit->setPlaceholderText(locale().percent());

        auto priceValidator = new QDoubleValidator{this};
        priceValidator->setBottom(0.);

        auto lblblblb = new QLabel{tr("Buy price:"), this};
        toolBarLayout->addWidget(lblblblb);
        toolBarLayout->setAlignment(lblblblb, Qt::AlignRight | Qt::AlignVCenter);

        value = settings.value(MarketAnalysisSettings::minBuyPriceFilterKey);

        mMinBuyPriceEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinBuyPriceEdit);
        mMinBuyPriceEdit->setValidator(priceValidator);

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxBuyPriceFilterKey);

        mMaxBuyPriceEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxBuyPriceEdit);
        mMaxBuyPriceEdit->setValidator(priceValidator);

        toolBarLayout->addWidget(new QLabel{tr("Sell price:"), this});

        value = settings.value(MarketAnalysisSettings::minSellPriceFilterKey);

        mMinSellPriceEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinSellPriceEdit);
        mMinSellPriceEdit->setValidator(priceValidator);

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxSellPriceFilterKey);

        mMaxSellPriceEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxSellPriceEdit);
        mMaxSellPriceEdit->setValidator(priceValidator);

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &MarketAnalysisWidget::applyFilter);

        mainLayout->addWidget(new QLabel{tr("Double-click an item for additional information. \"Show in EVE\" is available via the right-click menu."), this});

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
        mTypeDataView->setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(mTypeDataView, &QTableView::doubleClicked, this, &MarketAnalysisWidget::showDetails);
        connect(mTypeDataView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MarketAnalysisWidget::selectType);

        mDataStack->setCurrentWidget(mTypeDataView);

        mShowDetailsAct = new QAction{tr("Show details"), this};
        mShowDetailsAct->setEnabled(false);
        mTypeDataView->addAction(mShowDetailsAct);
        connect(mShowDetailsAct, &QAction::triggered, this, &MarketAnalysisWidget::showDetailsForCurrent);

        mShowInEveAct = new QAction{tr("Show in EVE"), this};
        mShowInEveAct->setEnabled(false);
        mTypeDataView->addAction(mShowInEveAct);
        connect(mShowInEveAct, &QAction::triggered, this, &MarketAnalysisWidget::showInEveForCurrent);

        mCopyRowsAct = new QAction{tr("&Copy"), this};
        mCopyRowsAct->setEnabled(false);
        mCopyRowsAct->setShortcut(QKeySequence::Copy);
        connect(mCopyRowsAct, &QAction::triggered, this, &MarketAnalysisWidget::copyRows);
        mTypeDataView->addAction(mCopyRowsAct);
    }

    void MarketAnalysisWidget::setCharacter(Character::IdType id)
    {
        mTypeDataModel.setCharacter(mCharacterRepo.find(id));
    }

    void MarketAnalysisWidget::handleNewPreferences()
    {
        mManager.handleNewPreferences();
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
        const auto minVolume = mMinVolumeEdit->text();
        const auto maxVolume = mMaxVolumeEdit->text();
        const auto minMargin = mMinMarginEdit->text();
        const auto maxMargin = mMaxMarginEdit->text();
        const auto minBuyPrice = mMinBuyPriceEdit->text();
        const auto maxBuyPrice = mMaxBuyPriceEdit->text();
        const auto minSellPrice = mMinSellPriceEdit->text();
        const auto maxSellPrice = mMaxSellPriceEdit->text();

        QSettings settings;
        settings.setValue(MarketAnalysisSettings::minVolumeFilterKey, minVolume);
        settings.setValue(MarketAnalysisSettings::maxVolumeFilterKey, maxVolume);
        settings.setValue(MarketAnalysisSettings::minMarginFilterKey, minMargin);
        settings.setValue(MarketAnalysisSettings::maxMarginFilterKey, maxMargin);
        settings.setValue(MarketAnalysisSettings::minBuyPriceFilterKey, minBuyPrice);
        settings.setValue(MarketAnalysisSettings::maxBuyPriceFilterKey, maxBuyPrice);
        settings.setValue(MarketAnalysisSettings::minSellPriceFilterKey, minSellPrice);
        settings.setValue(MarketAnalysisSettings::maxSellPriceFilterKey, maxSellPrice);

        mTypeViewProxy.setFilter((minVolume.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::VolumeValueType{}) : (minVolume.toUInt()),
                                 (maxVolume.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::VolumeValueType{}) : (maxVolume.toUInt()),
                                 (minMargin.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::MarginValueType{}) : (minMargin.toDouble()),
                                 (maxMargin.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::MarginValueType{}) : (maxMargin.toDouble()),
                                 (minBuyPrice.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::PriceValueType{}) : (minBuyPrice.toDouble()),
                                 (maxBuyPrice.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::PriceValueType{}) : (maxBuyPrice.toDouble()),
                                 (minSellPrice.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::PriceValueType{}) : (minSellPrice.toDouble()),
                                 (maxSellPrice.isEmpty()) ? (TypeAggregatedMarketDataFilterProxyModel::PriceValueType{}) : (maxSellPrice.toDouble()));
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

    void MarketAnalysisWidget::selectType(const QItemSelection &selected)
    {
        const auto enabled = !selected.isEmpty();
        mShowDetailsAct->setEnabled(enabled);
        mShowInEveAct->setEnabled(enabled);
        mCopyRowsAct->setEnabled(enabled);
    }

    void MarketAnalysisWidget::showDetailsForCurrent()
    {
        showDetails(mTypeDataView->currentIndex());
    }

    void MarketAnalysisWidget::showInEveForCurrent()
    {
        const auto id = mTypeDataModel.getTypeId(mTypeViewProxy.mapToSource(mTypeDataView->currentIndex()));
        if (id != EveType::invalidId)
            emit showInEve(id);
    }

    void MarketAnalysisWidget::copyRows() const
    {
        const auto indexes = mTypeDataView->selectionModel()->selectedIndexes();
        if (indexes.isEmpty())
            return;

        QString result;

        auto prevRow = indexes.first().row();
        for (const auto &index : indexes)
        {
            if (prevRow != index.row())
            {
                prevRow = index.row();
                result.append('\n');
            }

            result.append(mTypeViewProxy.data(index).toString());
            result.append('\t');
        }

        QApplication::clipboard()->setText(result);
    }

    void MarketAnalysisWidget::processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText)
    {
        --mOrderRequestCount;

        qDebug() << mOrderRequestCount << " orders remaining; error:" << errorText;

        if ((mOrderRequestCount % 10) == 0)
            mTaskManager.updateTask(mOrderSubtask, tr("Waiting for %1 order server replies...").arg(mOrderRequestCount));

        if (!errorText.isEmpty())
        {
            mAggregatedOrderErrors << errorText;

            if (mOrderRequestCount == 0)
            {
                mOrders.clear();
                mTaskManager.endTask(mOrderSubtask, mAggregatedOrderErrors.join("\n"));
                mAggregatedOrderErrors.clear();
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
