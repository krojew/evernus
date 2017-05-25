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
#include <unordered_set>

#include <QStandardItemModel>
#include <QDoubleValidator>
#include <QStackedWidget>
#include <QIntValidator>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QTableView>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSettings>
#include <QAction>
#include <QLabel>
#include <QDebug>

#include "DoubleTypeAggregatedDetailsWidget.h"
#include "MarketAnalysisSettings.h"
#include "StationSelectDialog.h"
#include "AdjustableTableView.h"
#include "MarketDataProvider.h"
#include "EveDataProvider.h"
#include "ImportSettings.h"
#include "PriceSettings.h"
#include "SSOMessageBox.h"
#include "ModelUtils.h"
#include "FlowLayout.h"

#include "InterRegionAnalysisWidget.h"

namespace Evernus
{
    InterRegionAnalysisWidget::InterRegionAnalysisWidget(const QByteArray &clientId,
                                                         const QByteArray &clientSecret,
                                                         const EveDataProvider &dataProvider,
                                                         const MarketDataProvider &marketDataProvider,
                                                         QWidget *parent)
        : QWidget(parent)
        , mDataProvider(dataProvider)
        , mMarketDataProvider(marketDataProvider)
        , mInterRegionDataModel(mDataProvider)
        , mInterRegionViewProxy(InterRegionMarketDataModel::getSrcRegionColumn(),
                                InterRegionMarketDataModel::getDstRegionColumn(),
                                InterRegionMarketDataModel::getVolumeColumn(),
                                InterRegionMarketDataModel::getMarginColumn())
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto createRegionCombo = [=] {
            auto combo = new QComboBox{this};
            combo->setEditable(true);
            combo->setInsertPolicy(QComboBox::NoInsert);

            return combo;
        };

        auto getStationName = [this](auto id) {
            return (id != 0) ? (mDataProvider.getLocationName(id)) : (tr("- any station -"));
        };

        QSettings settings;

        auto list = settings.value(MarketAnalysisSettings::srcStationKey).toList();
        if (list.size() == 4)
            mSrcStation = list[3].toULongLong();
        list = settings.value(MarketAnalysisSettings::dstStationKey).toList();
        if (list.size() == 4)
            mDstStation = list[3].toULongLong();

        toolBarLayout->addWidget(new QLabel{tr("Source:"), this});
        mSourceRegionCombo = createRegionCombo();
        toolBarLayout->addWidget(mSourceRegionCombo);

        auto stationBtn = new QPushButton{getStationName(mSrcStation), this};
        toolBarLayout->addWidget(stationBtn);
        connect(stationBtn, &QPushButton::clicked, this, [=] {
            changeStation(mSrcStation, *stationBtn, MarketAnalysisSettings::srcStationKey);
        });

        toolBarLayout->addWidget(new QLabel{tr("Destination:"), this});
        mDestRegionCombo = createRegionCombo();
        toolBarLayout->addWidget(mDestRegionCombo);

        stationBtn = new QPushButton{getStationName(mDstStation), this};
        toolBarLayout->addWidget(stationBtn);
        connect(stationBtn, &QPushButton::clicked, this, [=] {
            changeStation(mDstStation, *stationBtn, MarketAnalysisSettings::dstStationKey);
        });

        auto fillRegionCombo = [this](auto combo, const auto savedKey) {
            std::unordered_set<uint> saved;
            QSettings settings;

            const auto savedList = settings.value(savedKey).toList();
            for (const auto &value : savedList)
                saved.emplace(value.toUInt());

            auto model = new QStandardItemModel{this};

            const auto regions = mDataProvider.getRegions();
            for (const auto &region : regions)
            {
                auto item = new QStandardItem{region.second};
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                item->setData(region.first);
                item->setCheckState((saved.find(region.first) != std::end(saved)) ? (Qt::Checked) : (Qt::Unchecked));

                model->appendRow(item);
            }

            combo->setModel(model);

            connect(model, &QStandardItemModel::itemChanged, this, [=] {
                QVariantList saved;
                for (auto i = 0; i < model->rowCount(); ++i)
                {
                    const auto item = model->item(i);
                    if (item->checkState() == Qt::Checked)
                        saved.append(item->data().toUInt());
                }

                QSettings settings;
                settings.setValue(savedKey, saved);

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
            item->setCheckState((saved.empty() || saved.find(0) != std::end(saved)) ? (Qt::Checked) : (Qt::Unchecked));

            model->insertRow(allRegionsIndex, item);
            combo->setCurrentText(tr("- all -"));
        };

        fillRegionCombo(mSourceRegionCombo, MarketAnalysisSettings::srcRegionKey);
        fillRegionCombo(mDestRegionCombo, MarketAnalysisSettings::dstRegionKey);

        auto volumeValidator = new QIntValidator{this};
        volumeValidator->setBottom(0);
        toolBarLayout->addWidget(new QLabel{tr("Volume:"), this});

        auto value = settings.value(MarketAnalysisSettings::minVolumeFilterKey);

        mMinInterRegionVolumeEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinInterRegionVolumeEdit);
        mMinInterRegionVolumeEdit->setValidator(volumeValidator);

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxVolumeFilterKey);

        mMaxInterRegionVolumeEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxInterRegionVolumeEdit);
        mMaxInterRegionVolumeEdit->setValidator(volumeValidator);

        auto marginValidator = new QIntValidator{this};

        toolBarLayout->addWidget(new QLabel{tr("Margin:"), this});

        value = settings.value(MarketAnalysisSettings::minMarginFilterKey);

        mMinInterRegionMarginEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMinInterRegionMarginEdit);
        mMinInterRegionMarginEdit->setValidator(marginValidator);
        mMinInterRegionMarginEdit->setPlaceholderText(locale().percent());

        toolBarLayout->addWidget(new QLabel{"-", this});

        value = settings.value(MarketAnalysisSettings::maxMarginFilterKey);

        mMaxInterRegionMarginEdit = new QLineEdit{(value.isValid()) ? (value.toString()) : (QString{}), this};
        toolBarLayout->addWidget(mMaxInterRegionMarginEdit);
        mMaxInterRegionMarginEdit->setValidator(marginValidator);
        mMaxInterRegionMarginEdit->setPlaceholderText(locale().percent());

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &InterRegionAnalysisWidget::applyInterRegionFilter);

        toolBarLayout->addWidget(new QLabel{tr("Press \"Apply\" to show results. \"Show in EVE\" is available via the right-click menu."), this});

        mInterRegionDataStack = new QStackedWidget{this};
        mainLayout->addWidget(mInterRegionDataStack);

        auto waitingWidget = new QWidget{this};

        auto waitingLayout = new QVBoxLayout{waitingWidget};
        waitingLayout->setAlignment(Qt::AlignCenter);

        auto waitingLabel = new QLabel{tr("Calculating data..."), this};
        waitingLayout->addWidget(waitingLabel);
        waitingLabel->setAlignment(Qt::AlignCenter);

        auto waitingProgress = new QProgressBar{this};
        waitingLayout->addWidget(waitingProgress);
        waitingProgress->setRange(0, 0);

        mInterRegionDataStack->addWidget(waitingWidget);

        mInterRegionViewProxy.setSortRole(Qt::UserRole);
        mInterRegionViewProxy.setSourceModel(&mInterRegionDataModel);

        mInterRegionTypeDataView = new AdjustableTableView{"marketAnalysisInterRegionView", this};
        mInterRegionDataStack->addWidget(mInterRegionTypeDataView);
        mInterRegionTypeDataView->setSortingEnabled(true);
        mInterRegionTypeDataView->setAlternatingRowColors(true);
        mInterRegionTypeDataView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mInterRegionTypeDataView->setModel(&mInterRegionViewProxy);
        mInterRegionTypeDataView->setContextMenuPolicy(Qt::ActionsContextMenu);
        mInterRegionTypeDataView->restoreHeaderState();
        connect(mInterRegionTypeDataView, &QTableView::doubleClicked, this, &InterRegionAnalysisWidget::showDetails);
        connect(mInterRegionTypeDataView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &InterRegionAnalysisWidget::selectInterRegionType);

        mInterRegionDataStack->setCurrentWidget(mInterRegionTypeDataView);

        mShowDetailsAct = new QAction{tr("Show details"), this};
        mShowDetailsAct->setEnabled(false);
        mInterRegionTypeDataView->addAction(mShowDetailsAct);
        connect(mShowDetailsAct, &QAction::triggered, this, &InterRegionAnalysisWidget::showDetailsForCurrent);

        mShowInEveInterRegionAct = new QAction{tr("Show in EVE"), this};
        mShowInEveInterRegionAct->setEnabled(false);
        mInterRegionTypeDataView->addAction(mShowInEveInterRegionAct);
        connect(mShowInEveInterRegionAct, &QAction::triggered, this, &InterRegionAnalysisWidget::showInEveForCurrentInterRegion);

        mCopyInterRegionRowsAct = new QAction{tr("&Copy"), this};
        mCopyInterRegionRowsAct->setEnabled(false);
        mCopyInterRegionRowsAct->setShortcut(QKeySequence::Copy);
        connect(mCopyInterRegionRowsAct, &QAction::triggered, this, &InterRegionAnalysisWidget::copyRows);
        mInterRegionTypeDataView->addAction(mCopyInterRegionRowsAct);
    }

    void InterRegionAnalysisWidget::setPriceTypes(PriceType src, PriceType dst) noexcept
    {
        mSrcPriceType = src;
        mDstPriceType = dst;
    }

    void InterRegionAnalysisWidget::setBogusOrderThreshold(double value) noexcept
    {
        mInterRegionDataModel.setBogusOrderThreshold(value);
    }

    void InterRegionAnalysisWidget::discardBogusOrders(bool flag) noexcept
    {
        mInterRegionDataModel.discardBogusOrders(flag);
    }

    void InterRegionAnalysisWidget::setCharacter(const std::shared_ptr<Character> &character)
    {
        mInterRegionDataModel.setCharacter(character);
    }

    void InterRegionAnalysisWidget::recalculateAllData()
    {
        mRefreshedInterRegionData = false;
        applyInterRegionFilter();
    }

    void InterRegionAnalysisWidget::completeImport()
    {
        mRefreshedInterRegionData = false;
    }

    void InterRegionAnalysisWidget::clearData()
    {
        mInterRegionDataModel.reset();
    }

    void InterRegionAnalysisWidget::applyInterRegionFilter()
    {
        InterRegionMarketDataFilterProxyModel::RegionList srcRegions, dstRegions;

        auto fillRegions = [](const QStandardItemModel *model, InterRegionMarketDataFilterProxyModel::RegionList &list) {
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
            recalculateInterRegionData();

        fillRegions(static_cast<const QStandardItemModel *>(mSourceRegionCombo->model()), srcRegions);
        fillRegions(static_cast<const QStandardItemModel *>(mDestRegionCombo->model()), dstRegions);

        const auto minVolume = mMinInterRegionVolumeEdit->text();
        const auto maxVolume = mMaxInterRegionVolumeEdit->text();
        const auto minMargin = mMinInterRegionMarginEdit->text();
        const auto maxMargin = mMaxInterRegionMarginEdit->text();

        QSettings settings;
        settings.setValue(MarketAnalysisSettings::minVolumeFilterKey, minVolume);
        settings.setValue(MarketAnalysisSettings::maxVolumeFilterKey, maxVolume);
        settings.setValue(MarketAnalysisSettings::minMarginFilterKey, minMargin);
        settings.setValue(MarketAnalysisSettings::maxMarginFilterKey, maxMargin);

        mInterRegionViewProxy.setFilter(srcRegions,
                                        dstRegions,
                                        (minVolume.isEmpty()) ? (InterRegionMarketDataFilterProxyModel::VolumeValueType{}) : (minVolume.toUInt()),
                                        (maxVolume.isEmpty()) ? (InterRegionMarketDataFilterProxyModel::VolumeValueType{}) : (maxVolume.toUInt()),
                                        (minMargin.isEmpty()) ? (InterRegionMarketDataFilterProxyModel::MarginValueType{}) : (minMargin.toDouble()),
                                        (maxMargin.isEmpty()) ? (InterRegionMarketDataFilterProxyModel::MarginValueType{}) : (maxMargin.toDouble()));

        mInterRegionTypeDataView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

        if (!mRefreshedInterRegionData)
        {
            mInterRegionDataStack->setCurrentWidget(mInterRegionTypeDataView);
            mRefreshedInterRegionData = true;
        }
    }

    void InterRegionAnalysisWidget::showDetails(const QModelIndex &item)
    {
        const auto mappedItem = mInterRegionViewProxy.mapToSource(item);
        const auto id = mInterRegionDataModel.getTypeId(mappedItem);
        const auto srcRegion = mInterRegionDataModel.getSrcRegionId(mappedItem);
        const auto dstRegion = mInterRegionDataModel.getDstRegionId(mappedItem);

        const auto srcHistory = mMarketDataProvider.getHistory(srcRegion);
        if (srcHistory == nullptr)
            return;

        const auto dstHistory = mMarketDataProvider.getHistory(dstRegion);
        if (dstHistory == nullptr)
            return;

        const auto srcIt = srcHistory->find(id);
        const auto dstIt = dstHistory->find(id);
        if (srcIt != std::end(*srcHistory) || dstIt != std::end(*dstHistory))
        {
            auto widget = new DoubleTypeAggregatedDetailsWidget{srcIt->second,
                                                                dstIt->second,
                                                                mDataProvider.getRegionName(srcRegion),
                                                                mDataProvider.getRegionName(dstRegion),
                                                                this,
                                                                Qt::Window};
            widget->setWindowTitle(mDataProvider.getTypeName(id));
            widget->show();
            connect(this, &InterRegionAnalysisWidget::preferencesChanged, widget, &DoubleTypeAggregatedDetailsWidget::handleNewPreferences);
        }
    }

    void InterRegionAnalysisWidget::showDetailsForCurrent()
    {
        showDetails(mInterRegionTypeDataView->currentIndex());
    }

    void InterRegionAnalysisWidget::selectInterRegionType(const QItemSelection &selected)
    {
        const auto enabled = !selected.isEmpty();
        mShowDetailsAct->setEnabled(enabled);
        mShowInEveInterRegionAct->setEnabled(enabled);
        mCopyInterRegionRowsAct->setEnabled(enabled);
    }

    void InterRegionAnalysisWidget::showInEveForCurrentInterRegion()
    {
        const auto index = mInterRegionViewProxy.mapToSource(mInterRegionTypeDataView->currentIndex());
        const auto id = mInterRegionDataModel.getTypeId(index);
        if (id != EveType::invalidId)
            emit showInEve(id, mInterRegionDataModel.getOwnerId(index));
    }

    void InterRegionAnalysisWidget::copyRows() const
    {
        ModelUtils::copyRowsToClipboard(mInterRegionTypeDataView->selectionModel()->selectedIndexes(), mInterRegionViewProxy);
    }

    void InterRegionAnalysisWidget::changeStation(quint64 &destination, QPushButton &btn, const QString &settingName)
    {
        StationSelectDialog dlg{mDataProvider, true, this};

        QSettings settings;
        dlg.selectPath(settings.value(settingName).toList());

        if (dlg.exec() != QDialog::Accepted)
            return;

        settings.setValue(settingName, dlg.getSelectedPath());

        destination = dlg.getStationId();
        if (destination == 0)
            btn.setText(tr("- any station -"));
        else
            btn.setText(mDataProvider.getLocationName(destination));

        if (QMessageBox::question(this, tr("Station change"), tr("Changing station requires data recalculation. Do you wish to do it now?")) == QMessageBox::No)
            return;

        recalculateInterRegionData();
        mInterRegionDataStack->setCurrentWidget(mInterRegionTypeDataView);
    }

    void InterRegionAnalysisWidget::recalculateInterRegionData()
    {
        qDebug() << "Recomputing inter-region data...";

        const auto history = mMarketDataProvider.getHistory();
        if (history == nullptr)
            return;

        const auto orders = mMarketDataProvider.getOrders();
        if (orders == nullptr)
            return;

        mInterRegionDataModel.setOrderData(*orders,
                                           *history,
                                           mSrcStation,
                                           mDstStation,
                                           mSrcPriceType,
                                           mDstPriceType);

        mInterRegionDataStack->setCurrentIndex(waitingLabelIndex);
        mInterRegionDataStack->repaint();
    }
}
