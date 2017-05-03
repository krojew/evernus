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
#include <limits>
#include <memory>
#include <cmath>

#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QSplineSeries>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QBarSeries>
#include <QDateEdit>
#include <QSettings>
#include <QCheckBox>
#include <QSpinBox>
#include <QBarSet>
#include <QLabel>

#ifdef Q_CC_MSVC
#   pragma warning(push)
#   pragma warning(disable : 4244)
#endif

#include <boost/accumulators/statistics/rolling_variance.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/accumulators.hpp>

#ifdef Q_CC_MSVC
#   pragma warning(pop)
#endif

#include "MarketAnalysisSettings.h"
#include "UISettings.h"

#include "TypeAggregatedDetailsWidget.h"

namespace ba = boost::accumulators;

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

        const auto current = QDate::currentDate().addDays(-1);

        mFromEdit = new QDateEdit{this};
        toolBarLayout->addWidget(mFromEdit);
        mFromEdit->setCalendarPopup(true);
        mFromEdit->setDate(current.addDays(-90));
        mFromEdit->setMaximumDate(current);
        connect(mFromEdit, &QDateEdit::dateChanged, this, [=](const QDate &date) {
            if (date > mToEdit->date())
                mToEdit->setDate(date);
        });

        toolBarLayout->addWidget(new QLabel{tr("To:"), this});

        mToEdit = new QDateEdit{this};
        toolBarLayout->addWidget(mToEdit);
        mToEdit->setCalendarPopup(true);
        mToEdit->setDate(current);
        mToEdit->setMaximumDate(current);
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

        toolBarLayout->addWidget(new QLabel{tr("MACD days:"), this});

        mMACDFastDaysEdit = new QSpinBox{this};
        toolBarLayout->addWidget(mMACDFastDaysEdit);
        mMACDFastDaysEdit->setMinimum(2);
        mMACDFastDaysEdit->setValue(settings.value(MarketAnalysisSettings::macdFastDaysKey, MarketAnalysisSettings::macdFastDaysDefault).toInt());

        mMACDSlowDaysEdit = new QSpinBox{this};
        toolBarLayout->addWidget(mMACDSlowDaysEdit);
        mMACDSlowDaysEdit->setMinimum(2);
        mMACDSlowDaysEdit->setValue(settings.value(MarketAnalysisSettings::macdSlowDaysKey, MarketAnalysisSettings::macdSlowDaysDefault).toInt());

        mMACDEMADaysEdit = new QSpinBox{this};
        toolBarLayout->addWidget(mMACDEMADaysEdit);
        mMACDEMADaysEdit->setMinimum(2);
        mMACDEMADaysEdit->setValue(settings.value(MarketAnalysisSettings::macdEmaDaysKey, MarketAnalysisSettings::macdEmaDaysDefault).toInt());

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &TypeAggregatedDetailsWidget::applyFilter);

        auto addTrendLineBtn = new QPushButton{tr("Add trend line"), this};
        toolBarLayout->addWidget(addTrendLineBtn);
        connect(addTrendLineBtn, &QPushButton::clicked, this, &TypeAggregatedDetailsWidget::addTrendLine);

        const auto showLegend
            = settings.value(MarketAnalysisSettings::showLegendKey, MarketAnalysisSettings::showLegendDefault).toBool();

        auto legendBtn = new QCheckBox{tr("Show legend"), this};
        toolBarLayout->addWidget(legendBtn);
        legendBtn->setChecked(showLegend);

        toolBarLayout->addStretch();

        auto scrollArea = new QScrollArea{this};
        mainLayout->addWidget(scrollArea);
        scrollArea->setMinimumSize(500, 300);
        scrollArea->setWidgetResizable(true);

        auto chartContainer = new QWidget{this};
        scrollArea->setWidget(chartContainer);

        auto chartLayout = new QVBoxLayout{chartContainer};
        chartLayout->setSpacing(0);
        chartLayout->setContentsMargins(QMargins{});

        mHistoryChart = new ZoomableChartView{this};
        chartLayout->addWidget(mHistoryChart);
        mHistoryChart->chart()->legend()->setVisible(showLegend);
        mHistoryChart->setBackgroundBrush(mHistoryChart->chart()->backgroundBrush());

        connect(legendBtn, &QCheckBox::stateChanged, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::showLegendKey, checked);

            mHistoryChart->chart()->legend()->setVisible(checked);
            mMACDChart->chart()->legend()->setVisible(checked);
        });

        mRSIChart = new QChartView{this};
        chartLayout->addWidget(mRSIChart);
        mRSIChart->chart()->legend()->hide();
        mRSIChart->setBackgroundBrush(mRSIChart->chart()->backgroundBrush());

        mMACDChart = new QChartView{this};
        chartLayout->addWidget(mMACDChart);
        mMACDChart->chart()->legend()->setVisible(showLegend);
        mMACDChart->setBackgroundBrush(mRSIChart->chart()->backgroundBrush());

        auto locale = mHistoryChart->locale();
        locale.setNumberOptions(0);

        mHistoryChart->setLocale(locale);
        mRSIChart->setLocale(locale);
        mMACDChart->setLocale(locale);

        mValueAxis = new QValueAxis{this};
        mValueAxis->setTitleText("ISK");
        mHistoryChart->chart()->addAxis(mValueAxis, Qt::AlignLeft);

        mVolumeAxis = new QValueAxis{this};
        mVolumeAxis->setTitleText(tr("Volume"));
        mHistoryChart->chart()->addAxis(mVolumeAxis, Qt::AlignRight);

        mHistoryDateAxis = new QBarCategoryAxis{this};
        mHistoryDateAxis->setGridLineVisible(false);
        mHistoryDateAxis->setLabelsAngle(90);
        mHistoryChart->chart()->addAxis(mHistoryDateAxis, Qt::AlignBottom);

        mRSIAxis = new QValueAxis{this};
        mRSIAxis->setTitleText(tr("RSI (14 days)"));
        mRSIAxis->setRange(0., 100.);
        mRSIAxis->setLabelFormat("%.0f");
        mRSIChart->chart()->addAxis(mRSIAxis, Qt::AlignLeft);

        mRSIDateAxis = new QBarCategoryAxis{this};
        mRSIDateAxis->setGridLineVisible(false);
        mRSIDateAxis->setLabelsAngle(90);
        mRSIChart->chart()->addAxis(mRSIDateAxis, Qt::AlignBottom);
        connect(mHistoryDateAxis, &QBarCategoryAxis::rangeChanged, mRSIDateAxis, &QBarCategoryAxis::setRange);

        mMACDAxis = new QValueAxis{this};
        mMACDAxis->setTitleText(tr("MACD"));
        mMACDChart->chart()->addAxis(mMACDAxis, Qt::AlignLeft);

        mMACDDateAxis = new QBarCategoryAxis{this};
        mMACDDateAxis->setGridLineVisible(false);
        mMACDDateAxis->setLabelsAngle(90);
        mMACDChart->chart()->addAxis(mMACDDateAxis, Qt::AlignBottom);
        connect(mHistoryDateAxis, &QBarCategoryAxis::rangeChanged, mMACDDateAxis, &QBarCategoryAxis::setRange);

        applyFilter();
    }

    void TypeAggregatedDetailsWidget::handleNewPreferences()
    {
        applyGraphFormats();
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
        const auto macdFastDays = mMACDFastDaysEdit->value();
        const auto macdSlowDays = mMACDSlowDaysEdit->value();
        const auto macdEmaDays = mMACDEMADaysEdit->value();
        const auto rsiDays = 14;
        const auto rsiEmaAlpha = 1. / rsiDays;
        const auto macdFastEmaAlpha = 1. / macdFastDays;
        const auto macdSlowEmaAlpha = 1. / macdSlowDays;
        const auto macdEmaAlpha = 1. / macdEmaDays;

        QSettings settings;
        settings.setValue(MarketAnalysisSettings::smaDaysKey, smaDays);
        settings.setValue(MarketAnalysisSettings::macdFastDaysKey, macdFastDays);
        settings.setValue(MarketAnalysisSettings::macdSlowDaysKey, macdSlowDays);
        settings.setValue(MarketAnalysisSettings::macdEmaDaysKey, macdEmaDays);

        auto prevUEma = 0., prevDEma = 0.;
        auto prevMacdFastEma = prevAvg, prevMacdSlowEma = prevAvg, prevMacdEma = 0.;

        QStringList dates;
        dates.reserve(size);

        ba::accumulator_set<quint64, ba::stats<ba::tag::variance>> volAcc;
        ba::accumulator_set<double, ba::stats<ba::tag::rolling_mean, ba::tag::rolling_variance>>
        prcAcc(ba::tag::rolling_window::window_size = smaDays);

        auto minValue = std::numeric_limits<double>::max(), maxValue = 0.;
        auto maxVolume = 0u;

        const auto historyChart = mHistoryChart->chart();
        Q_ASSERT(historyChart != nullptr);

        const auto rsiChart = mRSIChart->chart();
        Q_ASSERT(rsiChart != nullptr);

        const auto macdChart = mMACDChart->chart();
        Q_ASSERT(macdChart != nullptr);

        historyChart->removeAllSeries();
        rsiChart->removeAllSeries();
        macdChart->removeAllSeries();

        mTrendLine = nullptr;

        const auto volumeSet = new QBarSet{tr("Volume"), this};
        volumeSet->setBrush(Qt::cyan);
        volumeSet->setPen(QPen{Qt::cyan});

        const auto bollingerUpSeries = new QSplineSeries{this};
        bollingerUpSeries->setName(tr("Bollinger upper band"));
        bollingerUpSeries->setPen(QPen{Qt::darkRed, 0., Qt::DashLine});

        const auto bollingerLowSeries = new QSplineSeries{this};
        bollingerLowSeries->setName(tr("Bollinger lower band"));
        bollingerLowSeries->setPen(QPen{Qt::darkGreen, 0., Qt::DashLine});

        const auto smaSeries = new QSplineSeries{this};
        smaSeries->setName(tr("SMA"));
        smaSeries->setPen(Qt::DashLine);

        const auto priceSeries = new QCandlestickSeries{this};
        priceSeries->setName(tr("Value"));
        priceSeries->setIncreasingColor(Qt::green);
        priceSeries->setDecreasingColor(Qt::red);

        const auto rsiSeries = new QLineSeries{this};
        rsiSeries->setPen(QPen{Qt::blue});

        const auto macdDivergence = new QBarSet{tr("MACD Divergence"), this};
        macdDivergence->setBrush(Qt::gray);
        macdDivergence->setPen(QPen{Qt::gray});

        const auto macdSeries = new QLineSeries{this};
        macdSeries->setName(tr("MACD"));
        macdSeries->setPen(QPen{Qt::darkYellow});

        const auto macdAvgSeries = new QLineSeries{this};
        macdAvgSeries->setName(tr("MACD Signal"));
        macdAvgSeries->setPen(QPen{Qt::red});

        const auto defaultDateFormat = locale().dateFormat(QLocale::NarrowFormat);

        QString dateFormat;
        if (settings.value(UISettings::applyDateFormatToGraphsKey, UISettings::applyDateFormatToGraphsDefault).toBool())
            dateFormat = settings.value(UISettings::dateTimeFormatKey, defaultDateFormat).toString();
        else
            dateFormat = defaultDateFormat;

        auto dateIndex = 0u;

        for (auto date = start, end = mToEdit->date(); date <= end; date = date.addDays(1))
        {
            const qreal datePoint = dateIndex;
            auto u = 0., d = 0.;

            dates << date.toString(dateFormat);

            QCandlestickSet *candle = nullptr;

            const auto it = mHistory.find(date);
            if (it == std::end(mHistory))
            {
                volAcc(0);
                prcAcc(0.);

                candle = new QCandlestickSet{datePoint, this};
                volumeSet->append(0.);

                prevAvg = 0.;
            }
            else
            {
                u = std::max(0., it->second.mAvgPrice - prevAvg);
                d = std::max(0., prevAvg - it->second.mAvgPrice);

                volAcc(it->second.mVolume);
                prcAcc(it->second.mAvgPrice);

                candle = new QCandlestickSet{std::max(std::min(prevAvg, it->second.mHighPrice), it->second.mLowPrice),
                                             it->second.mHighPrice,
                                             it->second.mLowPrice,
                                             it->second.mAvgPrice,
                                             datePoint,
                                             this};
                volumeSet->append(it->second.mVolume);

                prevAvg = it->second.mAvgPrice;

                if (it->second.mLowPrice < minValue)
                    minValue = it->second.mLowPrice;
                if (it->second.mHighPrice > maxValue)
                    maxValue = it->second.mHighPrice;

                if (it->second.mVolume > maxVolume)
                    maxVolume = it->second.mVolume;
            }

            priceSeries->append(candle);

            const auto avg = ba::rolling_mean(prcAcc);

            smaSeries->append(datePoint, avg);

            prevUEma = rsiEmaAlpha * u + (1. - rsiEmaAlpha) * prevUEma;
            prevDEma = rsiEmaAlpha * d + (1. - rsiEmaAlpha) * prevDEma;

            if (qFuzzyIsNull(prevDEma))
            {
                rsiSeries->append(datePoint, 100.);
            }
            else
            {
                const auto rs = prevUEma / prevDEma;
                rsiSeries->append(datePoint, (100. - 100. / (1. + rs)));
            }

            prevMacdFastEma = macdFastEmaAlpha * prevAvg + (1. - macdFastEmaAlpha) * prevMacdFastEma;
            prevMacdSlowEma = macdSlowEmaAlpha * prevAvg + (1. - macdSlowEmaAlpha) * prevMacdSlowEma;

            const auto curMacd = prevMacdFastEma - prevMacdSlowEma;
            macdSeries->append(datePoint, curMacd);

            prevMacdEma = macdEmaAlpha * curMacd + (1. - macdEmaAlpha) * prevMacdEma;
            macdAvgSeries->append(datePoint, prevMacdEma);

            macdDivergence->append(curMacd - prevMacdEma);

            const auto stdDev2 = 2. * std::sqrt(ba::rolling_variance(prcAcc));
            const auto bollingerUp = avg + stdDev2;
            const auto bollingerLow = avg - stdDev2;

            bollingerUpSeries->append(datePoint, bollingerUp);
            bollingerLowSeries->append(datePoint, bollingerLow);

            if (bollingerUp > maxValue)
                maxValue = bollingerUp;
            if (bollingerLow < minValue)
                minValue = bollingerLow;

            ++dateIndex;
        }

        const quint64 volStdDev2 = 2 * std::sqrt(ba::variance(volAcc));
        const auto volMean = ba::mean(volAcc);

        const auto unusualVolumeSet = new QBarSet{tr("Unusual volume"), this};
        unusualVolumeSet->setPen(QPen{Qt::cyan});
        unusualVolumeSet->setBrush(Qt::darkCyan);

        for (auto date = start, end = mToEdit->date(); date <= end; date = date.addDays(1))
        {
            const auto it = mHistory.find(date);
            if (it == std::end(mHistory))
                continue;

            if (it->second.mVolume < volMean - volStdDev2 || it->second.mVolume > volMean + volStdDev2)
                unusualVolumeSet->append(volumeSet->at(start.daysTo(date)));
            else
                unusualVolumeSet->append(0.);
        }

        mHistoryDateAxis->setCategories(dates);
        mValueAxis->setRange(minValue, maxValue);
        mVolumeAxis->setRange(0., maxVolume);

        const auto volumeSeries = new QBarSeries{this};
        volumeSeries->append(volumeSet);
        volumeSeries->setBarWidth(1.);

        const auto unusualVolumeSeries = new QBarSeries{this};
        unusualVolumeSeries->append(unusualVolumeSet);
        unusualVolumeSeries->setBarWidth(1.);

        historyChart->addSeries(volumeSeries);
        historyChart->addSeries(unusualVolumeSeries);
        historyChart->addSeries(bollingerUpSeries);
        historyChart->addSeries(bollingerLowSeries);
        historyChart->addSeries(smaSeries);
        historyChart->addSeries(priceSeries);

        const auto attachVolumeAxes = [=](auto series) {
            series->attachAxis(mHistoryDateAxis);
            series->attachAxis(mVolumeAxis);
        };

        const auto attachValueAxes = [=](auto series) {
            series->attachAxis(mHistoryDateAxis);
            series->attachAxis(mValueAxis);
        };

        attachVolumeAxes(volumeSeries);
        attachVolumeAxes(unusualVolumeSeries);

        attachValueAxes(bollingerUpSeries);
        attachValueAxes(bollingerLowSeries);
        attachValueAxes(smaSeries);
        attachValueAxes(priceSeries);

        mRSIDateAxis->setCategories(dates);

        const auto rsiHighSeries = new QLineSeries{this};
        rsiHighSeries->append(-1000000., 70.);
        rsiHighSeries->append(1000000., 70.);
        rsiHighSeries->setPen(QPen{Qt::black});
        rsiChart->addSeries(rsiHighSeries);

        const auto rsiLowSeries = new QLineSeries{this};
        rsiLowSeries->append(-1000000., 30.);
        rsiLowSeries->append(1000000., 30.);
        rsiLowSeries->setPen(QPen{Qt::black});
        rsiChart->addSeries(rsiLowSeries);

        rsiChart->addSeries(rsiSeries);

        const auto attachRSIAxes = [=](auto series) {
            series->attachAxis(mRSIDateAxis);
            series->attachAxis(mRSIAxis);
        };

        attachRSIAxes(rsiSeries);
        attachRSIAxes(rsiHighSeries);
        attachRSIAxes(rsiLowSeries);

        mMACDDateAxis->setCategories(dates);

        const auto macdDivergenceSeries = new QBarSeries{this};
        macdDivergenceSeries->append(macdDivergence);
        macdDivergenceSeries->setBarWidth(1.);

        macdChart->addSeries(macdDivergenceSeries);
        macdChart->addSeries(macdSeries);
        macdChart->addSeries(macdAvgSeries);

        const auto attachMACDAxes = [=](auto series) {
            series->attachAxis(mMACDDateAxis);
            series->attachAxis(mMACDAxis);
        };

        attachMACDAxes(macdSeries);
        attachMACDAxes(macdAvgSeries);
        attachMACDAxes(macdDivergenceSeries);

        applyGraphFormats();
    }

    void TypeAggregatedDetailsWidget::addTrendLine()
    {
        const auto start = mFromEdit->date();
        const auto end = mToEdit->date();

        auto sumXY = 0., sumX = 0., sumY = 0., sumX2 = 0.;

        for (auto date = start; date <= end; date = date.addDays(1))
        {
            const auto x = date.toJulianDay() - start.toJulianDay();
            sumX += x;
            sumX2 += x * x;

            const auto it = mHistory.find(date);
            if (it != std::end(mHistory))
            {
                sumXY += x * it->second.mAvgPrice;
                sumY += it->second.mAvgPrice;
            }
        }

        const auto n = start.daysTo(end) + 1;
        if (n == 0)
            return;

        const auto div = sumX2 - sumX * sumX / n;
        if (qFuzzyIsNull(div))
            return;

        const auto a = (sumXY - sumX * sumY / n) / div;
        const auto b = (sumY - a * sumX) / n;

        auto linearFunc = [=](double x) {
            return a * x + b;
        };

        if (mTrendLine == nullptr)
        {
            mTrendLine = new QLineSeries{this};
            mHistoryChart->chart()->addSeries(mTrendLine);
            mTrendLine->attachAxis(mHistoryDateAxis);
            mTrendLine->attachAxis(mValueAxis);
        }
        else
        {
            mTrendLine->clear();
        }

        mTrendLine->append(0, linearFunc(0.));
        const auto length = end.toJulianDay() - start.toJulianDay();
        mTrendLine->append(length, linearFunc(length));
    }

    void TypeAggregatedDetailsWidget::deleteTrendLine() noexcept
    {
        if (mTrendLine != nullptr)
            mTrendLine->hide();
    }

    void TypeAggregatedDetailsWidget::applyGraphFormats()
    {
        QSettings settings;
        mValueAxis->setLabelFormat(
           settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString());
    }
}
