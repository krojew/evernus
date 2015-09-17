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

#include <QStandardItemModel>
#include <QDoubleValidator>
#include <QStackedWidget>
#include <QIntValidator>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QClipboard>
#include <QTabWidget>
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
        , mInterRegionDataModel(mDataProvider)
        , mInterRegionViewProxy(InterRegionMarketDataModel::getSrcRegionColumn(),
                                InterRegionMarketDataModel::getDstRegionColumn())
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

        toolBarLayout->addStretch();

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs);

        tabs->addTab(createRegionAnalysisWidget(), tr("Region"));
        tabs->addTab(createInterRegionAnalysisWidget(), tr("Inter-Region"));
    }

    void MarketAnalysisWidget::setCharacter(Character::IdType id)
    {
        const auto character = mCharacterRepo.find(id);
        mTypeDataModel.setCharacter(character);
        mInterRegionDataModel.setCharacter(character);
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

            mRegionDataStack->setCurrentIndex(waitingLabelIndex);
            mRegionDataStack->repaint();

            fillSolarSystems(region);
            mTypeDataModel.setOrderData(mOrders, mHistory[region], region);

            mRegionDataStack->setCurrentWidget(mRegionTypeDataView);
        }
    }

    void MarketAnalysisWidget::showForCurrentRegionAndSolarSystem()
    {
        const auto region = getCurrentRegion();
        if (region != 0)
        {
            mRegionDataStack->setCurrentIndex(waitingLabelIndex);
            mRegionDataStack->repaint();

            const auto system = mSolarSystemCombo->currentData().toUInt();
            mTypeDataModel.setOrderData(mOrders, mHistory[region], region, system);

            mRegionDataStack->setCurrentWidget(mRegionTypeDataView);
        }
    }

    void MarketAnalysisWidget::applyRegionFilter()
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

    void MarketAnalysisWidget::applyInterRegionFilter()
    {
        InterRegionMarketDataFilterProxyModel::RegionList srcRegions, dstRegions;

        auto fillRegions = [](const auto model, auto &list) {
            if (model->item(allRegionsIndex)->checkState() == Qt::Checked)
            {
                for (auto i = allRegionsIndex + 1; i < model->rowCount(); ++i)
                    list.emplace(model->item(i)->data().toUInt());
            }
            else
            {
                for (auto i = allRegionsIndex + 1; i < model->rowCount(); ++i)
                {
                    if (model->item(i)->checkState() == Qt::Checked)
                        list.emplace(model->item(i)->data().toUInt());
                }
            }
        };

        if (!mRefreshedInterRegionData)
        {
            qDebug() << "Recomputing inter-region data...";

            mInterRegionDataStack->setCurrentIndex(waitingLabelIndex);
            mInterRegionDataStack->repaint();

            mInterRegionDataModel.setOrderData(mOrders, mHistory);
        }

        fillRegions(static_cast<const QStandardItemModel *>(mSourceRegionCombo->model()), srcRegions);
        fillRegions(static_cast<const QStandardItemModel *>(mDestRegionCombo->model()), dstRegions);

        mInterRegionViewProxy.setFilter(srcRegions, dstRegions);
        mInterRegionTypeDataView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

        if (!mRefreshedInterRegionData)
        {
            mInterRegionDataStack->setCurrentWidget(mInterRegionTypeDataView);
            mRefreshedInterRegionData = true;
        }
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

    void MarketAnalysisWidget::selectRegionType(const QItemSelection &selected)
    {
        const auto enabled = !selected.isEmpty();
        mShowDetailsAct->setEnabled(enabled);
        mShowInEveRegionAct->setEnabled(enabled);
        mCopyRegionRowsAct->setEnabled(enabled);
    }

    void MarketAnalysisWidget::selectInterRegionType(const QItemSelection &selected)
    {
        const auto enabled = !selected.isEmpty();
        mShowInEveInterRegionAct->setEnabled(enabled);
        mCopyInterRegionRowsAct->setEnabled(enabled);
    }

    void MarketAnalysisWidget::showDetailsForCurrent()
    {
        showDetails(mRegionTypeDataView->currentIndex());
    }

    void MarketAnalysisWidget::showInEveForCurrentRegion()
    {
        const auto id = mTypeDataModel.getTypeId(mTypeViewProxy.mapToSource(mRegionTypeDataView->currentIndex()));
        if (id != EveType::invalidId)
            emit showInEve(id);
    }

    void MarketAnalysisWidget::showInEveForCurrentInterRegion()
    {
        const auto id = mInterRegionDataModel.getTypeId(mInterRegionViewProxy.mapToSource(mInterRegionTypeDataView->currentIndex()));
        if (id != EveType::invalidId)
            emit showInEve(id);
    }

    void MarketAnalysisWidget::copyRows(const QAbstractItemView &view, const QAbstractItemModel &model) const
    {
        const auto indexes = view.selectionModel()->selectedIndexes();
        if (indexes.isEmpty())
            return;

        QString result;

        auto prevRow = indexes.first().row();
        for (const auto &index : indexes)
        {
            if (prevRow != index.row())
            {
                prevRow = index.row();
                result[result.size() - 1] = '\n';
            }

            result.append(model.data(index).toString());
            result.append('\t');
        }

        result.chop(1);
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
        {
            showForCurrentRegion();
            mRefreshedInterRegionData = false;
        }
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

    QWidget *MarketAnalysisWidget::createRegionAnalysisWidget()
    {
        auto container = new QWidget{this};
        auto mainLayout = new QVBoxLayout{container};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        toolBarLayout->addWidget(new QLabel{tr("Region:"), this});

        mRegionCombo = new QComboBox{this};
        toolBarLayout->addWidget(mRegionCombo);
        mRegionCombo->setEditable(true);
        mRegionCombo->setInsertPolicy(QComboBox::NoInsert);

        QSettings settings;
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

        toolBarLayout->addWidget(new QLabel{tr("Buy price:"), this});

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
        connect(filterBtn, &QPushButton::clicked, this, &MarketAnalysisWidget::applyRegionFilter);

        mainLayout->addWidget(new QLabel{tr("Double-click an item for additional information. \"Show in EVE\" is available via the right-click menu."), this});

        mRegionDataStack = new QStackedWidget{this};
        mainLayout->addWidget(mRegionDataStack);

        auto waitingLabel = new QLabel{tr("Calculating data..."), this};
        mRegionDataStack->addWidget(waitingLabel);
        waitingLabel->setAlignment(Qt::AlignCenter);

        mTypeViewProxy.setSortRole(Qt::UserRole);
        mTypeViewProxy.setSourceModel(&mTypeDataModel);

        mRegionTypeDataView = new QTableView{this};
        mRegionDataStack->addWidget(mRegionTypeDataView);
        mRegionTypeDataView->setSortingEnabled(true);
        mRegionTypeDataView->setAlternatingRowColors(true);
        mRegionTypeDataView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mRegionTypeDataView->setModel(&mTypeViewProxy);
        mRegionTypeDataView->setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(mRegionTypeDataView, &QTableView::doubleClicked, this, &MarketAnalysisWidget::showDetails);
        connect(mRegionTypeDataView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MarketAnalysisWidget::selectRegionType);

        mRegionDataStack->setCurrentWidget(mRegionTypeDataView);

        mShowDetailsAct = new QAction{tr("Show details"), this};
        mShowDetailsAct->setEnabled(false);
        mRegionTypeDataView->addAction(mShowDetailsAct);
        connect(mShowDetailsAct, &QAction::triggered, this, &MarketAnalysisWidget::showDetailsForCurrent);

        mShowInEveRegionAct = new QAction{tr("Show in EVE"), this};
        mShowInEveRegionAct->setEnabled(false);
        mRegionTypeDataView->addAction(mShowInEveRegionAct);
        connect(mShowInEveRegionAct, &QAction::triggered, this, &MarketAnalysisWidget::showInEveForCurrentRegion);

        mCopyRegionRowsAct = new QAction{tr("&Copy"), this};
        mCopyRegionRowsAct->setEnabled(false);
        mCopyRegionRowsAct->setShortcut(QKeySequence::Copy);
        connect(mCopyRegionRowsAct, &QAction::triggered, this, [=] {
            copyRows(*mRegionTypeDataView, mTypeViewProxy);
        });
        mRegionTypeDataView->addAction(mCopyRegionRowsAct);

        return container;
    }

    QWidget *MarketAnalysisWidget::createInterRegionAnalysisWidget()
    {
        auto container = new QWidget{this};
        auto mainLayout = new QVBoxLayout{container};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto createRegionCombo = [=] {
            auto combo = new QComboBox{this};
            combo->setEditable(true);
            combo->setInsertPolicy(QComboBox::NoInsert);

            return combo;
        };

        toolBarLayout->addWidget(new QLabel{tr("Source region:"), this});
        mSourceRegionCombo = createRegionCombo();
        toolBarLayout->addWidget(mSourceRegionCombo);

        toolBarLayout->addWidget(new QLabel{tr("Destination region:"), this});
        mDestRegionCombo = createRegionCombo();
        toolBarLayout->addWidget(mDestRegionCombo);

        auto fillRegionCombo = [this](auto combo) {
            auto model = new QStandardItemModel{this};

            const auto regions = mDataProvider.getRegions();
            for (const auto &region : regions)
            {
                auto item = new QStandardItem{region.second};
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                item->setCheckState(Qt::Checked);
                item->setData(region.first);

                model->appendRow(item);
            }

            combo->setModel(model);

            connect(model, &QStandardItemModel::itemChanged, this, [=] {
                if (model->item(allRegionsIndex)->checkState() == Qt::Checked)
                {
                    combo->setCurrentText(tr("- all -"));
                    return;
                }

                auto hasChecked = false;
                for (auto i = allRegionsIndex + 1; i < model->rowCount(); ++i)
                {
                    const auto item = model->item(i);
                    if (item->checkState() == Qt::Checked)
                    {
                        if (hasChecked)
                        {
                            combo->setCurrentText(tr("- multiple -"));
                            return;
                        }

                        hasChecked = true;
                        combo->setCurrentText(item->text());
                    }
                }

                if (!hasChecked)
                    combo->setCurrentText(tr("- none -"));
            }, Qt::QueuedConnection);

            auto item = new QStandardItem{tr("- all -")};
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setCheckState(Qt::Checked);

            model->insertRow(allRegionsIndex, item);
            combo->setCurrentText(tr("- all -"));
        };

        fillRegionCombo(mSourceRegionCombo);
        fillRegionCombo(mDestRegionCombo);

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &MarketAnalysisWidget::applyInterRegionFilter);

        toolBarLayout->addWidget(new QLabel{tr("\"Show in EVE\" is available via the right-click menu."), this});

        mInterRegionDataStack = new QStackedWidget{this};
        mainLayout->addWidget(mInterRegionDataStack);

        auto waitingLabel = new QLabel{tr("Calculating data..."), this};
        mInterRegionDataStack->addWidget(waitingLabel);
        waitingLabel->setAlignment(Qt::AlignCenter);

        mInterRegionViewProxy.setSortRole(Qt::UserRole);
        mInterRegionViewProxy.setSourceModel(&mInterRegionDataModel);

        mInterRegionTypeDataView = new QTableView{this};
        mInterRegionDataStack->addWidget(mInterRegionTypeDataView);
        mInterRegionTypeDataView->setSortingEnabled(true);
        mInterRegionTypeDataView->setAlternatingRowColors(true);
        mInterRegionTypeDataView->setModel(&mInterRegionViewProxy);
        mInterRegionTypeDataView->setContextMenuPolicy(Qt::ActionsContextMenu);
        connect(mInterRegionTypeDataView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MarketAnalysisWidget::selectInterRegionType);

        mInterRegionDataStack->setCurrentWidget(mInterRegionTypeDataView);

        mShowInEveInterRegionAct = new QAction{tr("Show in EVE"), this};
        mShowInEveInterRegionAct->setEnabled(false);
        mInterRegionTypeDataView->addAction(mShowInEveInterRegionAct);
        connect(mShowInEveInterRegionAct, &QAction::triggered, this, &MarketAnalysisWidget::showInEveForCurrentInterRegion);

        mCopyInterRegionRowsAct = new QAction{tr("&Copy"), this};
        mCopyInterRegionRowsAct->setEnabled(false);
        mCopyInterRegionRowsAct->setShortcut(QKeySequence::Copy);
        connect(mCopyInterRegionRowsAct, &QAction::triggered, this, [=] {
            copyRows(*mInterRegionTypeDataView, mInterRegionViewProxy);
        });
        mInterRegionTypeDataView->addAction(mCopyInterRegionRowsAct);

        return container;
    }
}
