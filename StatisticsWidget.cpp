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
#include <QVBoxLayout>
#include <QGroupBox>

#include "AssetValueSnapshotRepository.h"
#include "WalletSnapshotRepository.h"
#include "DateFilteredPlotWidget.h"
#include "qcustomplot.h"

#include "StatisticsWidget.h"

namespace Evernus
{
    StatisticsWidget::StatisticsWidget(const AssetValueSnapshotRepository &assetSnapshotRepo,
                                       const WalletSnapshotRepository &walletSnapshotRepo,
                                       QWidget *parent)
        : QWidget{parent}
        , mAssetSnapshotRepository{assetSnapshotRepo}
        , mWalletSnapshotRepository{walletSnapshotRepo}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto balanceGroup = new QGroupBox{tr("Balance"), this};
        mainLayout->addWidget(balanceGroup);

        auto balanceLayout = new QVBoxLayout{};
        balanceGroup->setLayout(balanceLayout);

        mBalancePlot = new DateFilteredPlotWidget{this};
        balanceLayout->addWidget(mBalancePlot);
        connect(mBalancePlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateData);

        mBalancePlot->getPlot().yAxis->setNumberFormat("gbc");
        mBalancePlot->getPlot().yAxis->setNumberPrecision(2);
        mBalancePlot->getPlot().yAxis->setLabel("ISK");

        mainLayout->addStretch();

        updateGraphAndLegend();
    }

    void StatisticsWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        if (mCharacterId == Character::invalidId)
        {
            mBalancePlot->getPlot().clearPlottables();
            updateGraphAndLegend();
        }
        else
        {
            mBalancePlot->blockSignals(true);

            const auto date = QDate::currentDate();

            mBalancePlot->setFrom(date.addMonths(-1));
            mBalancePlot->setTo(date);

            updateData();

            mBalancePlot->blockSignals(false);
        }
    }

    void StatisticsWidget::updateData()
    {
        const QDateTime from{mBalancePlot->getFrom()};
        const QDateTime to{mBalancePlot->getTo()};

        const auto assetShots = mAssetSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC().addDays(1));
        const auto walletShots = mWalletSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC().addDays(1));

        auto assetGraph = mBalancePlot->getPlot().graph(assetValueGraph);
        auto walletGraph = mBalancePlot->getPlot().graph(walletBalanceGraph);
        auto sumGraph = mBalancePlot->getPlot().graph(totalValueGraph);

        QVector<double> assetTicks, assetValues;
        QVector<double> walletTicks, walletValues;

        auto xMin = std::numeric_limits<double>::max(), xMax = -1.;
        auto yMax = -1.;

        for (const auto &shot : assetShots)
        {
            const auto secs = shot.getId().toMSecsSinceEpoch() / 1000.;
            if (secs > xMax)
                xMax = secs;
            if (secs < xMin)
                xMin = secs;

            assetTicks << secs;
            assetValues << shot.getBalance();
        }

        for (const auto &shot : walletShots)
        {
            const auto secs = shot.getId().toMSecsSinceEpoch() / 1000.;
            if (secs > xMax)
                xMax = secs;
            if (secs < xMin)
                xMin = secs;

            walletTicks << secs;
            walletValues << shot.getBalance();
        }

        if (!walletTicks.isEmpty() && !assetTicks.isEmpty())
        {
            const auto ab = assetTicks.back();
            const auto wb = walletTicks.back();

            if (wb > ab)
            {
                assetTicks << wb;
                assetValues << assetValues.back();
            }
            else if (ab > wb)
            {
                walletTicks << ab;
                walletValues << walletValues.back();
            }
        }

        QVector<double> sumTicks, sumValues;

        auto assetTickIt = std::begin(assetTicks);
        auto walletTickIt = std::begin(walletTicks);
        auto assetValueIt = std::begin(assetValues);
        auto walletValueIt = std::begin(walletValues);
        auto prevAsset = 0.;
        auto prevWallet = 0.;

        const auto lerp = [](double a, double b, double t) {
            return a + (b - a) * t;
        };

        while (assetTickIt != std::end(assetTicks) || walletTickIt != std::end(walletTicks))
        {
            if (assetTickIt == std::end(assetTicks))
            {
                const auto value = prevAsset + *walletValueIt;
                if (value > yMax)
                    yMax = value;

                sumTicks << *walletTickIt;
                sumValues << value;

                ++walletTickIt;
                ++walletValueIt;
            }
            else if (walletTickIt == std::end(walletTicks))
            {
                const auto value = prevWallet + *assetValueIt;
                if (value > yMax)
                    yMax = value;

                sumTicks << *assetTickIt;
                sumValues << value;

                ++assetTickIt;
                ++assetValueIt;
            }
            else
            {
                if (*assetTickIt < *walletTickIt)
                {
                    const auto prevTick = (walletTickIt == std::begin(walletTicks)) ? (assetTicks.front()) : (*std::prev(walletTickIt));
                    const auto div = *walletTickIt - prevTick;
                    while (*assetTickIt < *walletTickIt && assetTickIt != std::end(assetTicks))
                    {
                        prevAsset = *assetValueIt;

                        const auto value = lerp(prevWallet, *walletValueIt, (*assetTickIt - prevTick) / div) + prevAsset;
                        if (value > yMax)
                            yMax = value;

                        sumTicks << *assetTickIt;
                        sumValues << value;

                        ++assetTickIt;
                        ++assetValueIt;
                    }
                }
                else if (*assetTickIt > *walletTickIt)
                {
                    const auto prevTick = (assetTickIt == std::begin(assetTicks)) ? (walletTicks.front()) : (*std::prev(assetTickIt));
                    const auto div = *assetTickIt - prevTick;
                    while (*assetTickIt > *walletTickIt && walletTickIt != std::end(walletTicks))
                    {
                        prevWallet = *walletValueIt;

                        const auto value = prevWallet + lerp(prevAsset, *assetValueIt, (*walletTickIt - prevTick) / div);
                        if (value > yMax)
                            yMax = value;

                        sumTicks << *walletTickIt;
                        sumValues << value;

                        ++walletTickIt;
                        ++walletValueIt;
                    }
                }
                else
                {
                    prevWallet = *walletValueIt;
                    prevAsset = *assetValueIt;

                    const auto value = prevWallet + prevAsset;
                    if (value > yMax)
                        yMax = value;

                    sumTicks << *assetTickIt;
                    sumValues << value;

                    ++assetTickIt;
                    ++assetValueIt;
                    ++walletTickIt;
                    ++walletValueIt;
                }
            }
        }

        mBalancePlot->getPlot().xAxis->setTickVector(sumTicks);

        if (!walletTicks.isEmpty() || !assetTicks.isEmpty())
        {
            mBalancePlot->getPlot().xAxis->setRange(xMin, xMax);
            mBalancePlot->getPlot().yAxis->setRange(0., yMax);
        }

        assetGraph->setData(assetTicks, assetValues);
        walletGraph->setData(walletTicks, walletValues);
        sumGraph->setData(sumTicks, sumValues);

        mBalancePlot->getPlot().replot();
    }

    void StatisticsWidget::updateGraphAndLegend()
    {
        auto assetGraph = mBalancePlot->getPlot().addGraph();
        assetGraph->setName(tr("Asset value"));
        assetGraph->setPen(QPen{Qt::blue});

        auto walletGraph = mBalancePlot->getPlot().addGraph();
        walletGraph->setName(tr("Wallet balance"));
        walletGraph->setPen(QPen{Qt::red});

        auto totalGraph = mBalancePlot->getPlot().addGraph();
        totalGraph->setName(tr("Total value"));
        totalGraph->setPen(QPen{Qt::green});

        mBalancePlot->getPlot().legend->setVisible(true);
    }
}
