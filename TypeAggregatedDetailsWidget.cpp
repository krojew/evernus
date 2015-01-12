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
#include <memory>

#include <boost/circular_buffer.hpp>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDateEdit>
#include <QSettings>
#include <QSpinBox>
#include <QLabel>

#include "MarketAnalysisSettings.h"
#include "UISettings.h"

#include "qcustomplot.h"

#include "TypeAggregatedDetailsWidget.h"

namespace Evernus
{
    TypeAggregatedDetailsWidget::TypeAggregatedDetailsWidget(History history, QWidget *parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
        , mHistory(std::move(history))
    {
        const auto dayWidth = 23 * 3600;

        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        toolBarLayout->addWidget(new QLabel{tr("From:"), this});

        const auto current = QDate::currentDate();

        mFromEdit = new QDateEdit{this};
        toolBarLayout->addWidget(mFromEdit);
        mFromEdit->setCalendarPopup(true);
        mFromEdit->setDate(current.addDays(-30));
        connect(mFromEdit, &QDateEdit::dateChanged, this, [=](const QDate &date) {
            if (date > mToEdit->date())
                mToEdit->setDate(date);
        });

        toolBarLayout->addWidget(new QLabel{tr("To:"), this});

        mToEdit = new QDateEdit{this};
        toolBarLayout->addWidget(mToEdit);
        mToEdit->setCalendarPopup(true);
        mToEdit->setDate(current);
        connect(mToEdit, &QDateEdit::dateChanged, this, [=](const QDate &date) {
            if (date < mFromEdit->date())
                mFromEdit->setDate(date);
        });

        toolBarLayout->addWidget(new QLabel{tr("Moving average days:"), this});

        QSettings settings;

        mSMADaysEdit = new QSpinBox{this};
        toolBarLayout->addWidget(mSMADaysEdit);
        mSMADaysEdit->setMinimum(2);
        mSMADaysEdit->setValue(settings.value(MarketAnalysisSettings::smaDaysKey, MarketAnalysisSettings::smaDaysDefault).toInt());

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &TypeAggregatedDetailsWidget::applyFilter);

        toolBarLayout->addStretch();

        mHistoryPlot = new QCustomPlot{this};
        mainLayout->addWidget(mHistoryPlot);
        mHistoryPlot->setMinimumSize(500, 300);
        mHistoryPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        mHistoryPlot->xAxis->setAutoTicks(false);
        mHistoryPlot->xAxis->setAutoTickLabels(true);
        mHistoryPlot->xAxis->setTickLabelRotation(60);
        mHistoryPlot->xAxis->setSubTickCount(0);
        mHistoryPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
        mHistoryPlot->xAxis->setDateTimeFormat(locale().dateFormat(QLocale::NarrowFormat));
        mHistoryPlot->xAxis->grid()->setVisible(false);
        mHistoryPlot->yAxis->setNumberPrecision(2);
        mHistoryPlot->yAxis->setLabel("ISK");
        mHistoryPlot->yAxis2->setVisible(true);
        mHistoryPlot->yAxis2->setLabel(tr("Volume"));

        auto locale = mHistoryPlot->locale();
        locale.setNumberOptions(0);
        mHistoryPlot->setLocale(locale);

        mHistoryPlot->yAxis->setNumberFormat(
            settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString());

        auto volumeGraph = std::make_unique<QCPBars>(mHistoryPlot->xAxis, mHistoryPlot->yAxis2);
        mHistoryVolumeGraph = volumeGraph.get();
        mHistoryPlot->addPlottable(mHistoryVolumeGraph);
        volumeGraph.release();

        mHistoryVolumeGraph->setName(tr("Volume"));
        mHistoryVolumeGraph->setPen(QPen{Qt::cyan});
        mHistoryVolumeGraph->setBrush(Qt::cyan);
        mHistoryVolumeGraph->setWidth(dayWidth);

        auto valuesGraph = std::make_unique<QCPFinancial>(mHistoryPlot->xAxis, mHistoryPlot->yAxis);
        mHistoryValuesGraph = valuesGraph.get();
        mHistoryPlot->addPlottable(mHistoryValuesGraph);
        valuesGraph.release();

        mHistoryValuesGraph->setName(tr("Value"));
        mHistoryValuesGraph->setWidth(dayWidth);
        mHistoryValuesGraph->setChartStyle(QCPFinancial::csCandlestick);
        mHistoryValuesGraph->setTwoColored(true);

        mSMAGraph = mHistoryPlot->addGraph();
        mSMAGraph->setPen(Qt::DashLine);

        applyFilter();
    }

    void TypeAggregatedDetailsWidget::handleNewPreferences()
    {
        QSettings settings;
        mHistoryPlot->yAxis->setNumberFormat(
            settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString());

        mHistoryPlot->replot();
    }

    void TypeAggregatedDetailsWidget::applyFilter()
    {
        const auto start = mFromEdit->date();
        const auto size = start.daysTo(mToEdit->date()) + 1;

        auto prevAvg = 0.;

        const auto it = mHistory.lower_bound(start);
        if (it != std::end(mHistory))
        {
            if (it == std::begin(mHistory))
                prevAvg = it->second.mAvgPrice;
            else
                prevAvg = std::prev(it)->second.mAvgPrice;
        }

        const auto smaDays = mSMADaysEdit->value();

        QSettings settings;
        settings.setValue(MarketAnalysisSettings::smaDaysKey, smaDays);

        boost::circular_buffer<double> smaBuffer(smaDays, prevAvg);
        auto smaSum = smaDays * prevAvg;

        QVector<double> dates, volumes, open, high, low, close, sma;
        dates.reserve(size);
        volumes.reserve(size);
        open.reserve(size);
        high.reserve(size);
        low.reserve(size);
        close.reserve(size);
        sma.reserve(size);

        for (auto date = start, end = mToEdit->date(); date <= end; date = date.addDays(1))
        {
            dates << QDateTime{date}.toMSecsSinceEpoch() / 1000.;

            const auto it = mHistory.find(date);
            if (it == std::end(mHistory))
            {
                volumes << 0.;
                open << 0.;
                high << 0.;
                low << 0.;
                close << 0.;

                prevAvg = 0.;
            }
            else
            {
                volumes << it->second.mVolume;
                open << std::max(std::min(prevAvg, it->second.mHighPrice), it->second.mLowPrice);
                high << it->second.mHighPrice;
                low << it->second.mLowPrice;
                close << it->second.mAvgPrice;

                prevAvg = it->second.mAvgPrice;
            }

            smaSum -= smaBuffer.front();
            smaBuffer.pop_front();
            smaBuffer.push_back(prevAvg);
            smaSum += prevAvg;

            sma << (smaSum / smaDays);
        }

        mHistoryValuesGraph->setData(dates, open, high, low, close);
        mHistoryVolumeGraph->setData(dates, volumes);
        mSMAGraph->setData(dates, sma);

        mHistoryPlot->xAxis->setTickVector(dates);

        mHistoryPlot->rescaleAxes();
        mHistoryPlot->replot();
    }
}
