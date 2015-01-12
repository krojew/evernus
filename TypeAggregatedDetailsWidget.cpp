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

        const auto widgetLocale = locale();

        mHistoryPlot = new QCustomPlot{this};
        mainLayout->addWidget(mHistoryPlot);
        mHistoryPlot->setMinimumSize(500, 300);
        mHistoryPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        mHistoryPlot->xAxis->setAutoTicks(false);
        mHistoryPlot->xAxis->setAutoTickLabels(true);
        mHistoryPlot->xAxis->setTickLabelRotation(60);
        mHistoryPlot->xAxis->setSubTickCount(0);
        mHistoryPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
        mHistoryPlot->xAxis->setDateTimeFormat(widgetLocale.dateFormat(QLocale::NarrowFormat));
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

        auto rsiAxisRect = new QCPAxisRect{mHistoryPlot};
        mHistoryPlot->plotLayout()->addElement(1, 0, rsiAxisRect);
        rsiAxisRect->setMaximumSize(QWIDGETSIZE_MAX, 200);
        rsiAxisRect->setRangeDrag(Qt::Horizontal);
        rsiAxisRect->setRangeZoom(Qt::Horizontal);
        rsiAxisRect->axis(QCPAxis::atBottom)->setLayer("axes");
        rsiAxisRect->axis(QCPAxis::atBottom)->setTickLabelType(QCPAxis::ltDateTime);
        rsiAxisRect->axis(QCPAxis::atBottom)->setDateTimeFormat(widgetLocale.dateFormat(QLocale::NarrowFormat));
        rsiAxisRect->axis(QCPAxis::atLeft)->setRange(0., 100.);
        rsiAxisRect->axis(QCPAxis::atLeft)->setLabel(tr("RSI (14 days)"));
        rsiAxisRect->axis(QCPAxis::atBottom)->grid()->setLayer("grid");

        connect(mHistoryPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), rsiAxisRect->axis(QCPAxis::atBottom), SLOT(setRange(QCPRange)));
        connect(rsiAxisRect->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), mHistoryPlot->xAxis, SLOT(setRange(QCPRange)));

        auto marginGroup = new QCPMarginGroup{mHistoryPlot};
        mHistoryPlot->axisRect()->setMarginGroup(QCP::msLeft | QCP::msRight, marginGroup);
        rsiAxisRect->setMarginGroup(QCP::msLeft | QCP::msRight, marginGroup);

        auto overboughtLine = new QCPItemStraightLine{mHistoryPlot};
        mHistoryPlot->addItem(overboughtLine);
        overboughtLine->setClipAxisRect(rsiAxisRect);
        overboughtLine->point1->setAxes(rsiAxisRect->axis(QCPAxis::atBottom), rsiAxisRect->axis(QCPAxis::atLeft));
        overboughtLine->point2->setAxes(rsiAxisRect->axis(QCPAxis::atBottom), rsiAxisRect->axis(QCPAxis::atLeft));
        overboughtLine->point1->setCoords(0., 70.);
        overboughtLine->point2->setCoords(1000000000000., 70.);

        auto oversoldLine = new QCPItemStraightLine{mHistoryPlot};
        mHistoryPlot->addItem(oversoldLine);
        oversoldLine->setClipAxisRect(rsiAxisRect);
        oversoldLine->point1->setAxes(rsiAxisRect->axis(QCPAxis::atBottom), rsiAxisRect->axis(QCPAxis::atLeft));
        oversoldLine->point2->setAxes(rsiAxisRect->axis(QCPAxis::atBottom), rsiAxisRect->axis(QCPAxis::atLeft));
        oversoldLine->point1->setCoords(0., 30.);
        oversoldLine->point2->setCoords(1000000000000., 30.);

        mRSIGraph = mHistoryPlot->addGraph(rsiAxisRect->axis(QCPAxis::atBottom), rsiAxisRect->axis(QCPAxis::atLeft));
        mRSIGraph->setPen(QPen{Qt::blue});

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
        const auto rsiDays = 14;
        const auto rsiEmaAlpha = 1. / rsiDays;

        QSettings settings;
        settings.setValue(MarketAnalysisSettings::smaDaysKey, smaDays);

        boost::circular_buffer<double> smaBuffer(smaDays, prevAvg);
        auto smaSum = smaDays * prevAvg;
        auto prevUEma = 0., prevDEma = 0.;

        QVector<double> dates, volumes, open, high, low, close, sma, rsi;
        dates.reserve(size);
        volumes.reserve(size);
        open.reserve(size);
        high.reserve(size);
        low.reserve(size);
        close.reserve(size);
        sma.reserve(size);
        rsi.reserve(size);

        for (auto date = start, end = mToEdit->date(); date <= end; date = date.addDays(1))
        {
            dates << QDateTime{date}.toMSecsSinceEpoch() / 1000.;

            auto u = 0., d = 0.;

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
                u = std::max(0., it->second.mAvgPrice - prevAvg);
                d = std::max(0., prevAvg - it->second.mAvgPrice);

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

            prevUEma = rsiEmaAlpha * u + (1 - rsiEmaAlpha) * prevUEma;
            prevDEma = rsiEmaAlpha * d + (1 - rsiEmaAlpha) * prevDEma;

            if (qFuzzyIsNull(prevDEma))
            {
                rsi << 100.;
            }
            else
            {
                const auto rs = prevUEma / prevDEma;
                rsi << (100. - 100. / (1. + rs));
            }
        }

        mHistoryValuesGraph->setData(dates, open, high, low, close);
        mHistoryVolumeGraph->setData(dates, volumes);
        mSMAGraph->setData(dates, sma);
        mRSIGraph->setData(dates, rsi);

        mHistoryPlot->xAxis->setTickVector(dates);

        mHistoryPlot->xAxis->rescale();
        mHistoryPlot->yAxis->rescale();
        mHistoryPlot->yAxis2->rescale();
        mHistoryPlot->replot();
    }
}
