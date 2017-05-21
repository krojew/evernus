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
#include <QApplication>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
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

#include "RegionTypeSelectDialog.h"
#include "MarketAnalysisSettings.h"
#include "MarketOrderRepository.h"
#include "RegionAnalysisWidget.h"
#include "CharacterRepository.h"
#include "StationSelectDialog.h"
#include "AdjustableTableView.h"
#include "EveDataProvider.h"
#include "ImportSettings.h"
#include "PriceSettings.h"
#include "SSOMessageBox.h"
#include "TaskManager.h"
#include "FlowLayout.h"
#include "UISettings.h"

#include "MarketAnalysisWidget.h"

namespace Evernus
{
    MarketAnalysisWidget::MarketAnalysisWidget(const QByteArray &clientId,
                                               const QByteArray &clientSecret,
                                               const EveDataProvider &dataProvider,
                                               TaskManager &taskManager,
                                               const MarketOrderRepository &orderRepo,
                                               const MarketOrderRepository &corpOrderRepo,
                                               const EveTypeRepository &typeRepo,
                                               const MarketGroupRepository &groupRepo,
                                               const CharacterRepository &characterRepo,
                                               QWidget *parent)
        : QWidget(parent)
        , MarketDataProvider()
        , mDataProvider(dataProvider)
        , mTaskManager(taskManager)
        , mOrderRepo(orderRepo)
        , mCorpOrderRepo(corpOrderRepo)
        , mTypeRepo(typeRepo)
        , mGroupRepo(groupRepo)
        , mCharacterRepo(characterRepo)
        , mOrders(std::make_shared<MarketAnalysisDataFetcher::OrderResultType::element_type>())
        , mHistory(std::make_shared<MarketAnalysisDataFetcher::HistoryResultType::element_type>())
        , mInterRegionDataModel(mDataProvider)
        , mInterRegionViewProxy(InterRegionMarketDataModel::getSrcRegionColumn(),
                                InterRegionMarketDataModel::getDstRegionColumn(),
                                InterRegionMarketDataModel::getVolumeColumn(),
                                InterRegionMarketDataModel::getMarginColumn())
        , mDataFetcher(clientId, clientSecret, mDataProvider, mCharacterRepo)
    {
        connect(&mDataFetcher, &MarketAnalysisDataFetcher::orderStatusUpdated,
                this, &MarketAnalysisWidget::updateOrderTask);
        connect(&mDataFetcher, &MarketAnalysisDataFetcher::historyStatusUpdated,
                this, &MarketAnalysisWidget::updateHistoryTask);
        connect(&mDataFetcher, &MarketAnalysisDataFetcher::orderImportEnded,
                this, &MarketAnalysisWidget::endOrderTask);
        connect(&mDataFetcher, &MarketAnalysisDataFetcher::historyImportEnded,
                this, &MarketAnalysisWidget::endHistoryTask);
        connect(&mDataFetcher, &MarketAnalysisDataFetcher::genericError,
                this, [=](const auto &text) {
            SSOMessageBox::showMessage(text, this);
        });

        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import data"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &MarketAnalysisWidget::prepareOrderImport);

        QSettings settings;

        auto list = settings.value(MarketAnalysisSettings::srcStationKey).toList();
        if (list.size() == 4)
            mSrcStation = list[3].toUInt();
        list = settings.value(MarketAnalysisSettings::dstStationKey).toList();
        if (list.size() == 4)
            mDstStation = list[3].toUInt();

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

        const auto discardBogusOrders = settings.value(MarketAnalysisSettings::discardBogusOrdersKey, MarketAnalysisSettings::discardBogusOrdersDefault).toBool();
        mInterRegionDataModel.discardBogusOrders(discardBogusOrders);

        auto discardBogusOrdersBtn = new QCheckBox{tr("Discard bogus orders (causes recalculation)"), this};
        toolBarLayout->addWidget(discardBogusOrdersBtn);
        discardBogusOrdersBtn->setChecked(discardBogusOrders);
        connect(discardBogusOrdersBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::discardBogusOrdersKey, checked);

            mRegionAnalysisWidget->discardBogusOrders(checked);
            mInterRegionDataModel.discardBogusOrders(checked);

            recalculateAllData();
        });

        toolBarLayout->addWidget(new QLabel{tr("Bogus order threshold:"), this});

        auto thresholdValidator = new QDoubleValidator{this};
        thresholdValidator->setBottom(0.);

        const auto bogusThreshold
            = settings.value(MarketAnalysisSettings::bogusOrderThresholdKey, MarketAnalysisSettings::bogusOrderThresholdDefault).toString();
        const auto bogusThresholdValue = bogusThreshold.toDouble();

        mInterRegionDataModel.setBogusOrderThreshold(bogusThresholdValue);

        auto bogusThresholdEdit = new QLineEdit{bogusThreshold, this};
        toolBarLayout->addWidget(bogusThresholdEdit);
        bogusThresholdEdit->setValidator(thresholdValidator);
        connect(bogusThresholdEdit, &QLineEdit::textEdited, this, [this](const auto &text) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::bogusOrderThresholdKey, text);

            const auto value = text.toDouble();
            mRegionAnalysisWidget->setBogusOrderThreshold(value);
            mInterRegionDataModel.setBogusOrderThreshold(value);
        });

        auto createPriceTypeCombo = [=](auto &combo) {
            combo = new QComboBox{this};
            toolBarLayout->addWidget(combo);
            combo->addItem(tr("Sell"), static_cast<int>(PriceType::Sell));
            combo->addItem(tr("Buy"), static_cast<int>(PriceType::Buy));

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] {
                mRegionAnalysisWidget->setPriceTypes(getPriceType(*mSrcPriceTypeCombo), getPriceType(*mDstPriceTypeCombo));
                recalculateAllData();
            });
        };

        toolBarLayout->addWidget(new QLabel{tr("Source price:"), this});
        createPriceTypeCombo(mSrcPriceTypeCombo);
        mSrcPriceTypeCombo->blockSignals(true);
        mSrcPriceTypeCombo->setCurrentIndex(1);
        mSrcPriceTypeCombo->blockSignals(false);

        toolBarLayout->addWidget(new QLabel{tr("Destination price:"), this});
        createPriceTypeCombo(mDstPriceTypeCombo);

        auto skillsDiffBtn = new QCheckBox{tr("Use skills and taxes for difference calculation (causes recalculation)"), this};
        toolBarLayout->addWidget(skillsDiffBtn);
        skillsDiffBtn->setChecked(
            settings.value(MarketAnalysisSettings::useSkillsForDifferenceKey, MarketAnalysisSettings::useSkillsForDifferenceDefault).toBool());
        connect(skillsDiffBtn, &QCheckBox::toggled, [=](auto checked) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::useSkillsForDifferenceKey, checked);

            recalculateAllData();
        });

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs);

        mRegionAnalysisWidget = new RegionAnalysisWidget{clientId, clientSecret, mDataProvider, *this, tabs};
        connect(mRegionAnalysisWidget, &RegionAnalysisWidget::showInEve, this, &MarketAnalysisWidget::showInEve);
        mRegionAnalysisWidget->setPriceTypes(getPriceType(*mSrcPriceTypeCombo), getPriceType(*mDstPriceTypeCombo));
        mRegionAnalysisWidget->setBogusOrderThreshold(bogusThresholdValue);
        mRegionAnalysisWidget->discardBogusOrders(discardBogusOrders);

        tabs->addTab(mRegionAnalysisWidget, tr("Region"));
        tabs->addTab(createInterRegionAnalysisWidget(), tr("Inter-Region"));
    }

    const MarketAnalysisWidget::HistoryMap *MarketAnalysisWidget::getHistory(uint regionId) const
    {
        if (!mHistory)
            return nullptr;

        const auto it = mHistory->find(regionId);
        return (it == std::end(*mHistory)) ? (nullptr) : (&it->second);
    }

    const MarketAnalysisWidget::OrderResultType *MarketAnalysisWidget::getOrders() const
    {
        return (mOrders) ? (mOrders.get()) : (nullptr);
    }

    void MarketAnalysisWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        const auto character = mCharacterRepo.find(mCharacterId);
        mRegionAnalysisWidget->setCharacter(character);
        mInterRegionDataModel.setCharacter(character);
    }

    void MarketAnalysisWidget::prepareOrderImport()
    {
        RegionTypeSelectDialog dlg{mDataProvider, mTypeRepo, mGroupRepo, this};
        connect(&dlg, &RegionTypeSelectDialog::selected, this, &MarketAnalysisWidget::importData);

        dlg.exec();
    }

    void MarketAnalysisWidget::importData(const ExternalOrderImporter::TypeLocationPairs &pairs)
    {
        mOrders = std::make_shared<MarketAnalysisDataFetcher::OrderResultType::element_type>();
        mHistory = std::make_shared<MarketAnalysisDataFetcher::HistoryResultType::element_type>();
        mInterRegionDataModel.reset();

        if (!mDataFetcher.hasPendingOrderRequests() && !mDataFetcher.hasPendingHistoryRequests())
        {
            QSettings settings;
            const auto webImporter = static_cast<ImportSettings::WebImporterType>(
                settings.value(ImportSettings::webImportTypeKey, static_cast<int>(ImportSettings::webImportTypeDefault)).toInt());

            const auto mainTask = mTaskManager.startTask(tr("Importing data for analysis..."));
            const auto infoText = (webImporter == ImportSettings::WebImporterType::EveCentral) ?
                                  (tr("Making %1 Eve-Central order requests...")) :
                                  (tr("Making %1 ESI order requests..."));

            mOrderSubtask = mTaskManager.startTask(mainTask, infoText.arg(pairs.size()));
            mHistorySubtask = mTaskManager.startTask(mainTask, tr("Making %1 ESI history requests...").arg(pairs.size()));
        }

        MarketOrderRepository::TypeLocationPairs ignored;
        if (mIgnoreExistingOrdersBtn->isChecked())
        {
            auto ignoreTypes = [&](const auto &activeTypes) {
                for (const auto &pair : activeTypes)
                    ignored.insert(std::make_pair(pair.first, mDataProvider.getStationRegionId(pair.second)));
            };

            ignoreTypes(mOrderRepo.fetchActiveTypes());
            ignoreTypes(mCorpOrderRepo.fetchActiveTypes());
        }

        QMetaObject::invokeMethod(&mDataFetcher,
                                  "importData",
                                  Qt::QueuedConnection,
                                  Q_ARG(ExternalOrderImporter::TypeLocationPairs, pairs),
                                  Q_ARG(MarketOrderRepository::TypeLocationPairs, ignored),
                                  Q_ARG(Character::IdType, mCharacterId));
    }

    void MarketAnalysisWidget::storeOrders()
    {
        emit updateExternalOrders(*mOrders);

        mTaskManager.endTask(mOrderSubtask);
        checkCompletion();
    }

    void MarketAnalysisWidget::showForCurrentRegion()
    {
        mRegionAnalysisWidget->showForCurrentRegion();
    }

    void MarketAnalysisWidget::applyInterRegionFilter()
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

    void MarketAnalysisWidget::selectInterRegionType(const QItemSelection &selected)
    {
        const auto enabled = !selected.isEmpty();
        mShowInEveInterRegionAct->setEnabled(enabled);
        mCopyInterRegionRowsAct->setEnabled(enabled);
    }

    void MarketAnalysisWidget::showInEveForCurrentInterRegion()
    {
        const auto index = mInterRegionViewProxy.mapToSource(mInterRegionTypeDataView->currentIndex());
        const auto id = mInterRegionDataModel.getTypeId(index);
        if (id != EveType::invalidId)
            emit showInEve(id, mInterRegionDataModel.getOwnerId(index));
    }

    void MarketAnalysisWidget::copyRows(const QAbstractItemView &view, const QAbstractItemModel &model) const
    {
        const auto indexes = view.selectionModel()->selectedIndexes();
        if (indexes.isEmpty())
            return;

        QSettings settings;
        const auto delim
            = settings.value(UISettings::columnDelimiterKey, UISettings::columnDelimiterDefault).value<char>();

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
            result.append(delim);
        }

        result.chop(1);
        QApplication::clipboard()->setText(result);
    }

    void MarketAnalysisWidget::updateOrderTask(const QString &text)
    {
        mTaskManager.updateTask(mOrderSubtask, text);
    }

    void MarketAnalysisWidget::updateHistoryTask(const QString &text)
    {
        mTaskManager.updateTask(mHistorySubtask, text);
    }

    void MarketAnalysisWidget::endOrderTask(const MarketAnalysisDataFetcher::OrderResultType &orders, const QString &error)
    {
        Q_ASSERT(orders);
        mOrders = orders;

        if (error.isEmpty())
        {
            if (!mDontSaveBtn->isChecked())
            {
                mTaskManager.updateTask(mOrderSubtask, tr("Saving %1 imported orders...").arg(mOrders->size()));
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
            mTaskManager.endTask(mOrderSubtask, error);
        }
    }

    void MarketAnalysisWidget::endHistoryTask(const MarketAnalysisDataFetcher::HistoryResultType &history, const QString &error)
    {
        Q_ASSERT(history);
        mHistory = history;

        if (error.isEmpty())
        {
            mTaskManager.endTask(mHistorySubtask);
            checkCompletion();
        }
        else
        {
            mTaskManager.endTask(mHistorySubtask, error);
        }
    }

    void MarketAnalysisWidget::checkCompletion()
    {
        if (!mDataFetcher.hasPendingOrderRequests() && !mDataFetcher.hasPendingHistoryRequests())
        {
            showForCurrentRegion();
            mRefreshedInterRegionData = false;
        }
    }

    void MarketAnalysisWidget::changeStation(quint64 &destination, QPushButton &btn, const QString &settingName)
    {
        StationSelectDialog dlg{mDataProvider, this};

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

    void MarketAnalysisWidget::recalculateInterRegionData()
    {
        qDebug() << "Recomputing inter-region data...";

        mInterRegionDataStack->setCurrentIndex(waitingLabelIndex);
        mInterRegionDataStack->repaint();

        mInterRegionDataModel.setOrderData(*mOrders,
                                           *mHistory,
                                           mSrcStation,
                                           mDstStation,
                                           getPriceType(*mSrcPriceTypeCombo),
                                           getPriceType(*mDstPriceTypeCombo));
    }

    void MarketAnalysisWidget::recalculateAllData()
    {
        mRefreshedInterRegionData = false;

        showForCurrentRegion();
        applyInterRegionFilter();
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

        auto getStationName = [this](auto id) {
            return (id != 0) ? (mDataProvider.getLocationName(id)) : (tr("- any station -"));
        };

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

        QSettings settings;
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
        connect(filterBtn, &QPushButton::clicked, this, &MarketAnalysisWidget::applyInterRegionFilter);

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
        mInterRegionTypeDataView->setModel(&mInterRegionViewProxy);
        mInterRegionTypeDataView->setContextMenuPolicy(Qt::ActionsContextMenu);
        mInterRegionTypeDataView->restoreHeaderState();
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

    PriceType MarketAnalysisWidget::getPriceType(const QComboBox &combo)
    {
        return static_cast<PriceType>(combo.currentData().toInt());
    }
}
