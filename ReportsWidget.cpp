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
#include <unordered_map>

#include <QVBoxLayout>
#include <QHeaderView>
#include <QCheckBox>
#include <QGroupBox>
#include <QSettings>
#include <QDateTime>
#include <QVector>
#include <QDate>
#include <QPen>

#include "WalletTransactionRepository.h"
#include "AdjustableTableView.h"
#include "CharacterRepository.h"
#include "RepositoryProvider.h"
#include "WalletTransaction.h"
#include "DateRangeWidget.h"
#include "EveDataProvider.h"
#include "FlowLayout.h"
#include "UISettings.h"

#include "qcustomplot.h"

#include "ReportsWidget.h"

namespace Evernus
{
    ReportsWidget::ReportsWidget(const RepositoryProvider &repositoryProvider,
                                 const EveDataProvider &dataProvider,
                                 QWidget *parent)
        : QWidget{parent}
        , mWalletTransactionRepository{repositoryProvider.getWalletTransactionRepository()}
        , mCorpWalletTransactionRepository{repositoryProvider.getCorpWalletTransactionRepository()}
        , mCharacterRepository{repositoryProvider.getCharacterRepository()}
        , mDataProvider{dataProvider}
        , mPerformanceModel{mWalletTransactionRepository,
                            mCorpWalletTransactionRepository,
                            mCharacterRepository,
                            mDataProvider}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        const auto tillDate = QDate::currentDate();
        const auto fromDate = tillDate.addDays(-7);

        mDateRangeEdit = new DateRangeWidget{this};
        toolBarLayout->addWidget(mDateRangeEdit);
        mDateRangeEdit->setRange(fromDate, tillDate);
        connect(mDateRangeEdit, &DateRangeWidget::rangeChanged, this, &ReportsWidget::recalculateData);

        QSettings settings;

        mCombineBtn = new QCheckBox{tr("Combine for all characters"), this};
        toolBarLayout->addWidget(mCombineBtn);
        mCombineBtn->setChecked(settings.value(UISettings::combineReportsKey, UISettings::combineReportsDefault).toBool());
        connect(mCombineBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(UISettings::combineReportsKey, checked);

            recalculateData();
        });

        mCombineWithCorpBtn = new QCheckBox{tr("Combine with corp. data"), this};
        toolBarLayout->addWidget(mCombineWithCorpBtn);
        mCombineWithCorpBtn->setChecked(settings.value(UISettings::combineReportsWithCorpKey, UISettings::combineReportsWithCorpDefault).toBool());
        connect(mCombineWithCorpBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(UISettings::combineReportsWithCorpKey, checked);

            recalculateData();
        });

        mPerformanceProxy.setSortRole(Qt::UserRole);
        mPerformanceProxy.setSourceModel(&mPerformanceModel);

        const auto bestItemsGroup = new QGroupBox{tr("Best items"), this};
        mainLayout->addWidget(bestItemsGroup, 1);

        const auto bestItemsGroupLayout = new QVBoxLayout{bestItemsGroup};

        mBestItemsView = new AdjustableTableView{QStringLiteral("reportsBestItemsView"), this};
        bestItemsGroupLayout->addWidget(mBestItemsView);
        mBestItemsView->setSortingEnabled(true);
        mBestItemsView->setAlternatingRowColors(true);
        mBestItemsView->setModel(&mPerformanceProxy);
        mBestItemsView->restoreHeaderState();

        const auto stationProfitGroup = new QGroupBox{tr("Station profit"), this};
        mainLayout->addWidget(stationProfitGroup, 1);

        const auto stationProfitGroupLayout = new QVBoxLayout{stationProfitGroup};

        mStationProfitPlot = new QCustomPlot{this};
        stationProfitGroupLayout->addWidget(mStationProfitPlot);
        mStationProfitPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        mStationProfitPlot->xAxis->setAutoTicks(false);
        mStationProfitPlot->xAxis->setAutoTickLabels(false);
        mStationProfitPlot->xAxis->setAutoSubTicks(false);
        mStationProfitPlot->yAxis->setNumberPrecision(2);
        mStationProfitPlot->yAxis->setLabel("ISK");
        mStationProfitPlot->yAxis2->setLabel(tr("Size [m³]"));
        mStationProfitPlot->yAxis2->setVisible(true);
        mStationProfitPlot->legend->setVisible(true);

        mStationProfitGraph = new QCPBars{mStationProfitPlot->xAxis, mStationProfitPlot->yAxis};
        mStationProfitPlot->addPlottable(mStationProfitGraph);
        mStationProfitGraph->setPen(QPen{Qt::green});
        mStationProfitGraph->setBrush(Qt::green);
        mStationProfitGraph->setName(tr("Total profit"));
        mStationProfitGraph->setWidth(0.2);

        mStationCostGraph = new QCPBars{mStationProfitPlot->xAxis, mStationProfitPlot->yAxis};
        mStationProfitPlot->addPlottable(mStationCostGraph);
        mStationCostGraph->setPen(QPen{Qt::red});
        mStationCostGraph->setBrush(Qt::red);
        mStationCostGraph->setName(tr("Total costs"));
        mStationCostGraph->setWidth(0.2);

        mStationVolumeGraph = new QCPBars{mStationProfitPlot->xAxis, mStationProfitPlot->yAxis2};
        mStationProfitPlot->addPlottable(mStationVolumeGraph);
        mStationVolumeGraph->setPen(QPen{Qt::cyan});
        mStationVolumeGraph->setBrush(Qt::cyan);
        mStationVolumeGraph->setName(tr("Total size"));
        mStationVolumeGraph->setWidth(0.2);

        applyGraphFormats();
    }

    void ReportsWidget::setCharacter(Character::IdType id)
    {
        const auto prevCharactedId = mCharacterId;

        mCharacterId = id;

        if (!mCombineBtn->isChecked() || prevCharactedId == Character::invalidId)
            recalculateData();
    }

    void ReportsWidget::handleNewPreferences()
    {
        applyGraphFormats();
    }

    void ReportsWidget::recalculateData()
    {
        const auto combineCharacters = mCombineBtn->isChecked();
        const auto combineCorp = mCombineWithCorpBtn->isChecked();

        recalculateBestItems(combineCharacters, combineCorp);
        recalculateTotalProfit(combineCharacters, combineCorp);
    }

    void ReportsWidget::recalculateBestItems(bool combineCharacters, bool combineCorp)
    {
        mPerformanceModel.reset(mDateRangeEdit->getFrom(),
                                mDateRangeEdit->getTo(),
                                combineCharacters,
                                combineCorp,
                                mCharacterId);

        mBestItemsView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
        mBestItemsView->sortByColumn(TypePerformanceModel::profitColumn, Qt::DescendingOrder);
    }

    void ReportsWidget::recalculateTotalProfit(bool combineCharacters, bool combineCorp)
    {
        const QDateTime from{mDateRangeEdit->getFrom(), QTime{0, 0}};
        const QDateTime to{mDateRangeEdit->getTo(), QTime{23, 59}};

        const auto charEntries = (combineCharacters) ?
                                 (mWalletTransactionRepository.fetchInRange(from, to, WalletTransactionRepository::EntryType::All)) :
                                 (mWalletTransactionRepository.fetchForCharacterInRange(mCharacterId, from, to, WalletTransactionRepository::EntryType::All));

        struct DayData
        {
            double mIncoming = 0.;
            double mOutgoing = 0.;
            double mVolume = 0.;
        };

        std::unordered_map<quint64, DayData> aggregatedEntries;

        const auto insertEntries = [&](const auto &entries) {
            for (const auto &entry : entries)
            {
                Q_ASSERT(entry != nullptr);

                const auto quantity = entry->getQuantity();
                const auto price = entry->getPrice();

                auto &data = aggregatedEntries[entry->getLocationId()];
                if (entry->getType() == WalletTransaction::Type::Buy)
                    data.mOutgoing += quantity * price;
                else
                    data.mIncoming += quantity * price;

                data.mVolume += quantity * mDataProvider.getTypeVolume(entry->getTypeId());
            }
        };

        insertEntries(charEntries);

        if (combineCorp)
        {
            const auto corpEntries = (combineCharacters) ?
                                     (mCorpWalletTransactionRepository.fetchInRange(from, to, WalletTransactionRepository::EntryType::All)) :
                                     (mCorpWalletTransactionRepository.fetchForCorporationInRange(mCharacterRepository.getCorporationId(mCharacterId),
                                                                                                  from,
                                                                                                  to,
                                                                                                  WalletTransactionRepository::EntryType::All));
            insertEntries(corpEntries);
        }

        QVector<double> incoming, outgoing, volume, incomingStations, outgoingStations, volumeStations, stations;
        QVector<QString> stationNames;

        const auto size = static_cast<int>(aggregatedEntries.size());

        incoming.reserve(size);
        outgoing.reserve(size);
        incomingStations.reserve(size);
        outgoingStations.reserve(size);
        stations.reserve(size);
        stationNames.reserve(size);

        for (const auto &entry : aggregatedEntries)
        {
            incoming << entry.second.mIncoming;
            outgoing << entry.second.mOutgoing;
            volume << entry.second.mVolume;
            incomingStations << entry.first - 0.25;
            outgoingStations << entry.first;
            volumeStations << entry.first + 0.25;
            stations << entry.first;
            stationNames << mDataProvider.getLocationName(entry.first);
        }

        mStationProfitGraph->setData(incomingStations, incoming);
        mStationCostGraph->setData(outgoingStations, outgoing);
        mStationVolumeGraph->setData(volumeStations, volume);

        mStationProfitPlot->xAxis->setTickVector(stations);
        mStationProfitPlot->xAxis->setTickVectorLabels(stationNames);

        mStationProfitPlot->xAxis->rescale();
        mStationProfitPlot->yAxis->rescale();
        mStationProfitPlot->yAxis2->rescale();
        mStationProfitPlot->replot();
    }

    void ReportsWidget::applyGraphFormats()
    {
        QSettings settings;
        mStationProfitPlot->yAxis->setNumberFormat(
           settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString());
    }
}
