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
#include <algorithm>
#include <limits>

#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QSpinBox>
#include <QAction>
#include <QLabel>

#include "MarketAnalysisSettings.h"
#include "AdjustableTableView.h"
#include "StationSelectDialog.h"
#include "MarketDataProvider.h"
#include "EveDataProvider.h"
#include "ModelUtils.h"
#include "FlowLayout.h"

#include "ImportingAnalysisWidget.h"

namespace Evernus
{
    ImportingAnalysisWidget::ImportingAnalysisWidget(const EveDataProvider &dataProvider,
                                                     const MarketDataProvider &marketDataProvider,
                                                     QWidget *parent)
        : QWidget(parent)
        , mDataProvider(dataProvider)
        , mMarketDataProvider(marketDataProvider)
        , mDataModel(mDataProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        toolBarLayout->addWidget(new QLabel{tr("Source:"), this});

        const auto getStationName = [=](auto id) {
            return (id != 0) ? (mDataProvider.getLocationName(id)) : (tr("- no station -"));
        };

        QSettings settings;

        auto list = settings.value(MarketAnalysisSettings::srcImportStationKey).toList();
        if (list.size() == 4)
            mSrcStation = list[3].toULongLong();
        list = settings.value(MarketAnalysisSettings::dstImportStationKey).toList();
        if (list.size() == 4)
            mDstStation = list[3].toULongLong();

        auto stationBtn = new QPushButton{getStationName(mSrcStation), this};
        toolBarLayout->addWidget(stationBtn);
        connect(stationBtn, &QPushButton::clicked, this, [=] {
            changeStation(mSrcStation, *stationBtn, MarketAnalysisSettings::srcImportStationKey);
        });

        toolBarLayout->addWidget(new QLabel{tr("Destination:"), this});

        stationBtn = new QPushButton{getStationName(mDstStation), this};
        toolBarLayout->addWidget(stationBtn);
        connect(stationBtn, &QPushButton::clicked, this, [=] {
            changeStation(mDstStation, *stationBtn, MarketAnalysisSettings::dstImportStationKey);
        });

        toolBarLayout->addWidget(new QLabel{tr("Analysis period:"), this});

        mAnalysisDaysEdit = new QSpinBox{this};
        toolBarLayout->addWidget(mAnalysisDaysEdit);
        mAnalysisDaysEdit->setRange(1, 365);
        mAnalysisDaysEdit->setSuffix(tr("days"));
        mAnalysisDaysEdit->setValue(
            settings.value(MarketAnalysisSettings::importingAnalysisDaysKey, MarketAnalysisSettings::importingAnalysisDaysDefault).toInt());

        toolBarLayout->addWidget(new QLabel{tr("Aggregate over:"), this});

        mAggrDaysEdit = new QSpinBox{this};
        toolBarLayout->addWidget(mAggrDaysEdit);
        mAggrDaysEdit->setRange(1, 365);
        mAggrDaysEdit->setSuffix(tr("days"));
        mAggrDaysEdit->setValue(
            settings.value(MarketAnalysisSettings::importingAggrDaysKey, MarketAnalysisSettings::importingAggrDaysDefault).toInt());

        toolBarLayout->addWidget(new QLabel{tr("Price per m³:"), this});

        mPricePerM3 = new QDoubleSpinBox{this};
        toolBarLayout->addWidget(mPricePerM3);
        mPricePerM3->setMaximum(std::numeric_limits<double>::max());
        mPricePerM3->setValue(settings.value(MarketAnalysisSettings::importingPricePerM3Key).toDouble());

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &ImportingAnalysisWidget::recalculateData);

        toolBarLayout->addWidget(new QLabel{tr("Press \"Apply\" to show results. \"Show in EVE\" is available via the right-click menu."), this});

        mDataStack = new QStackedWidget{this};
        mainLayout->addWidget(mDataStack);

        auto waitingWidget = new QWidget{this};

        auto waitingLayout = new QVBoxLayout{waitingWidget};
        waitingLayout->setAlignment(Qt::AlignCenter);

        auto waitingLabel = new QLabel{tr("Calculating data..."), this};
        waitingLayout->addWidget(waitingLabel);
        waitingLabel->setAlignment(Qt::AlignCenter);

        auto waitingProgress = new QProgressBar{this};
        waitingLayout->addWidget(waitingProgress);
        waitingProgress->setRange(0, 0);

        mDataStack->addWidget(waitingWidget);

        mDataProxy.setSortRole(Qt::UserRole);
        mDataProxy.setSourceModel(&mDataModel);

        mDataView = new AdjustableTableView{QStringLiteral("marketAnalysisImportingView"), this};
        mDataStack->addWidget(mDataView);
        mDataView->setSortingEnabled(true);
        mDataView->setAlternatingRowColors(true);
        mDataView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mDataView->setModel(&mDataProxy);
        mDataView->setContextMenuPolicy(Qt::ActionsContextMenu);
        mDataView->restoreHeaderState();
        connect(mDataView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &ImportingAnalysisWidget::selectType);

        mDataStack->setCurrentWidget(mDataView);

        mShowInEveAct = new QAction{tr("Show in EVE"), this};
        mShowInEveAct->setEnabled(false);
        mDataView->addAction(mShowInEveAct);
        connect(mShowInEveAct, &QAction::triggered, this, &ImportingAnalysisWidget::showInEveForCurrent);

        mCopyRowsAct = new QAction{tr("&Copy"), this};
        mCopyRowsAct->setEnabled(false);
        mCopyRowsAct->setShortcut(QKeySequence::Copy);
        connect(mCopyRowsAct, &QAction::triggered, this, &ImportingAnalysisWidget::copyRows);
        mDataView->addAction(mCopyRowsAct);
    }

    void ImportingAnalysisWidget::setPriceTypes(PriceType src, PriceType dst) noexcept
    {
        mSrcPriceType = src;
        mDstPriceType = dst;
    }

    void ImportingAnalysisWidget::setBogusOrderThreshold(double value) noexcept
    {
        mDataModel.setBogusOrderThreshold(value);
    }

    void ImportingAnalysisWidget::discardBogusOrders(bool flag) noexcept
    {
        mDataModel.discardBogusOrders(flag);
    }

    void ImportingAnalysisWidget::setCharacter(const std::shared_ptr<Character> &character)
    {
        mCharacter = character;
        mDataModel.setCharacter(mCharacter);
    }

    void ImportingAnalysisWidget::recalculateData()
    {
        if (mSrcStation == 0 || mDstStation == 0)
            return;

        qDebug() << "Recomputing importing data...";

        mDataStack->setCurrentIndex(waitingLabelIndex);
        mDataStack->repaint();

        const auto history = mMarketDataProvider.getHistory();
        if (history == nullptr)
            return;

        const auto orders = mMarketDataProvider.getOrders();
        if (orders == nullptr)
            return;

        const auto analysisDays = mAnalysisDaysEdit->value();
        const auto aggrDays = mAggrDaysEdit->value();
        const auto pricePerM3 = mPricePerM3->value();

        QSettings settings;
        settings.setValue(MarketAnalysisSettings::importingAnalysisDaysKey, analysisDays);
        settings.setValue(MarketAnalysisSettings::importingAggrDaysKey, aggrDays);
        settings.setValue(MarketAnalysisSettings::importingPricePerM3Key, pricePerM3);

        mDataModel.setOrderData(*orders,
                                *history,
                                mSrcStation,
                                mDstStation,
                                mSrcPriceType,
                                mDstPriceType,
                                analysisDays,
                                std::min(analysisDays, aggrDays),
                                pricePerM3);

        mDataStack->setCurrentWidget(mDataView);
    }

    void ImportingAnalysisWidget::copyRows() const
    {
        ModelUtils::copyRowsToClipboard(mDataView->selectionModel()->selectedIndexes(), mDataProxy);
    }

    void ImportingAnalysisWidget::selectType(const QItemSelection &selected)
    {
        const auto enabled = !selected.isEmpty();
        mShowInEveAct->setEnabled(enabled);
        mCopyRowsAct->setEnabled(enabled);
    }

    void ImportingAnalysisWidget::showInEveForCurrent()
    {
        if (!mCharacter)
            return;

        const auto id = mDataModel.getTypeId(mDataProxy.mapToSource(mDataView->currentIndex()));
        if (id != EveType::invalidId)
            emit showInEve(id, mCharacter->getId());
    }

    void ImportingAnalysisWidget::changeStation(quint64 &destination, QPushButton &btn, const QString &settingName)
    {
        StationSelectDialog dlg{mDataProvider, false, this};

        QSettings settings;
        dlg.selectPath(settings.value(settingName).toList());

        if (dlg.exec() != QDialog::Accepted)
            return;

        settings.setValue(settingName, dlg.getSelectedPath());

        destination = dlg.getStationId();
        if (destination == 0)
        {
            btn.setText(tr("- no station -"));
        }
        else
        {
            btn.setText(mDataProvider.getLocationName(destination));

            if (QMessageBox::question(this, tr("Station change"), tr("Changing station requires data recalculation. Do you wish to do it now?")) == QMessageBox::No)
                return;

            recalculateData();
        }
    }
}
