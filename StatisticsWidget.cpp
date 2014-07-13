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
#include <thread>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QHash>

#include "WalletJournalEntryRepository.h"
#include "AssetValueSnapshotRepository.h"
#include "WalletTransactionRepository.h"
#include "WalletSnapshotRepository.h"
#include "DateFilteredPlotWidget.h"
#include "qcustomplot.h"

#include "StatisticsWidget.h"

namespace Evernus
{
    StatisticsWidget::StatisticsWidget(const AssetValueSnapshotRepository &assetSnapshotRepo,
                                       const WalletSnapshotRepository &walletSnapshotRepo,
                                       const WalletJournalEntryRepository &journalRepo,
                                       const WalletTransactionRepository &transactionRepo,
                                       QWidget *parent)
        : QWidget{parent}
        , mAssetSnapshotRepository{assetSnapshotRepo}
        , mWalletSnapshotRepository{walletSnapshotRepo}
        , mJournalRepo{journalRepo}
        , mTransactionRepo{transactionRepo}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto balanceGroup = new QGroupBox{tr("Balance"), this};
        mainLayout->addWidget(balanceGroup);

        auto balanceLayout = new QVBoxLayout{};
        balanceGroup->setLayout(balanceLayout);

        mBalancePlot = createPlot();
        balanceLayout->addWidget(mBalancePlot);
        connect(mBalancePlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateBalanceData);

        auto journalGroup = new QGroupBox{tr("Wallet journal"), this};
        mainLayout->addWidget(journalGroup);

        auto journalLayout = new QVBoxLayout{};
        journalGroup->setLayout(journalLayout);

        mJournalPlot = createPlot();
        journalLayout->addWidget(mJournalPlot);
        connect(mJournalPlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateJournalData);

        auto transactionGroup = new QGroupBox{tr("Wallet transactions"), this};
        mainLayout->addWidget(transactionGroup);

        auto transactionLayout = new QVBoxLayout{};
        transactionGroup->setLayout(transactionLayout);

        mTransactionPlot = createPlot();
        transactionLayout->addWidget(mTransactionPlot);
        connect(mTransactionPlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateTransactionData);

        mainLayout->addStretch();

        updateGraphAndLegend();
    }

    void StatisticsWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        if (mCharacterId == Character::invalidId)
        {
            mBalancePlot->getPlot().clearPlottables();
            mJournalPlot->getPlot().clearPlottables();
            updateGraphAndLegend();
        }
        else
        {
            mBalancePlot->blockSignals(true);
            mJournalPlot->blockSignals(true);
            mTransactionPlot->blockSignals(true);

            const auto date = QDate::currentDate();

            mBalancePlot->setFrom(date.addMonths(-1));
            mBalancePlot->setTo(date);
            mJournalPlot->setFrom(date.addMonths(-1));
            mJournalPlot->setTo(date);
            mTransactionPlot->setFrom(date.addMonths(-1));
            mTransactionPlot->setTo(date);

            std::thread balanceThread{&StatisticsWidget::updateBalanceData, this};
            std::thread journalThread{&StatisticsWidget::updateJournalData, this};

            updateTransactionData();

            if (balanceThread.joinable())
                balanceThread.join();
            if (journalThread.joinable())
                journalThread.join();

            mTransactionPlot->blockSignals(false);
            mJournalPlot->blockSignals(false);
            mBalancePlot->blockSignals(false);
        }
    }

    void StatisticsWidget::updateBalanceData()
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

        auto yMax = -1.;

        for (const auto &shot : assetShots)
        {
            const auto secs = shot.getTimestamp().toMSecsSinceEpoch() / 1000.;

            assetTicks << secs;
            assetValues << shot.getBalance();
        }

        for (const auto &shot : walletShots)
        {
            const auto secs = shot.getTimestamp().toMSecsSinceEpoch() / 1000.;

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

        assetGraph->setData(assetTicks, assetValues);
        walletGraph->setData(walletTicks, walletValues);
        sumGraph->setData(sumTicks, sumValues);

        if (!walletTicks.isEmpty() || !assetTicks.isEmpty())
        {
            mBalancePlot->getPlot().xAxis->rescale();
            mBalancePlot->getPlot().yAxis->setRange(0., yMax);
        }

        mBalancePlot->getPlot().replot();
    }

    void StatisticsWidget::updateJournalData()
    {
        const auto entries = mJournalRepo.fetchForCharacterInRange(mCharacterId,
                                                                   QDateTime{mJournalPlot->getFrom()},
                                                                   QDateTime{mJournalPlot->getTo()},
                                                                   WalletJournalEntryRepository::EntryType::All);

        QHash<QDate, std::pair<double, double>> values;
        for (const auto &entry : entries)
        {
            if (entry.isIgnored())
                continue;

            auto &value = values[entry.getTimestamp().toLocalTime().date()];

            const auto amount = entry.getAmount();
            if (amount < 0.)
                value.first -= amount;
            else
                value.second += amount;
        }

        QVector<double> ticks, incomingTicks, outgoingTicks, incomingValues, outgoingValues;

        QHashIterator<QDate, std::pair<double, double>> it{values};
        while (it.hasNext())
        {
            it.next();

            const auto secs = QDateTime{it.key()}.toMSecsSinceEpoch() / 1000.;

            ticks << secs;

            incomingTicks << secs - 3600 * 3;
            outgoingTicks << secs + 3600 * 3;

            outgoingValues << it.value().first;
            incomingValues << it.value().second;
        }

        mIncomingPlot->setData(incomingTicks, incomingValues);
        mOutgoingPlot->setData(outgoingTicks, outgoingValues);

        mJournalPlot->getPlot().xAxis->setTickVector(ticks);

        mJournalPlot->getPlot().rescaleAxes();
        mJournalPlot->getPlot().replot();
    }

    void StatisticsWidget::updateTransactionData()
    {
        const auto entries = mTransactionRepo.fetchForCharacterInRange(mCharacterId,
                                                                       QDateTime{mTransactionPlot->getFrom()},
                                                                       QDateTime{mTransactionPlot->getTo()},
                                                                       WalletTransactionRepository::EntryType::All);

        QHash<QDate, std::pair<double, double>> values;
        for (const auto &entry : entries)
        {
            if (entry.isIgnored())
                continue;

            auto &value = values[entry.getTimestamp().toLocalTime().date()];

            const auto amount = entry.getPrice();
            if (entry.getType() == WalletTransaction::Type::Buy)
                value.first += amount;
            else
                value.second += amount;
        }

        QVector<double> ticks, incomingTicks, outgoingTicks, incomingValues, outgoingValues;

        QHashIterator<QDate, std::pair<double, double>> it{values};
        while (it.hasNext())
        {
            it.next();

            const auto secs = QDateTime{it.key()}.toMSecsSinceEpoch() / 1000.;

            ticks << secs;

            incomingTicks << secs - 3600 * 3;
            outgoingTicks << secs + 3600 * 3;

            outgoingValues << it.value().first;
            incomingValues << it.value().second;
        }

        mSellPlot->setData(incomingTicks, incomingValues);
        mBuyPlot->setData(outgoingTicks, outgoingValues);

        mTransactionPlot->getPlot().xAxis->setTickVector(ticks);

        mTransactionPlot->getPlot().rescaleAxes();
        mTransactionPlot->getPlot().replot();
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

        mIncomingPlot = createBarPlot(mJournalPlot, tr("Incoming"), Qt::green);
        mOutgoingPlot = createBarPlot(mJournalPlot, tr("Outgoing"), Qt::red);

        mSellPlot = createBarPlot(mTransactionPlot, tr("Sell"), Qt::green);
        mBuyPlot = createBarPlot(mTransactionPlot, tr("Buy"), Qt::red);

        mJournalPlot->getPlot().legend->setVisible(true);
        mTransactionPlot->getPlot().legend->setVisible(true);
    }

    DateFilteredPlotWidget *StatisticsWidget::createPlot()
    {
        auto plot = new DateFilteredPlotWidget{this};
        plot->getPlot().xAxis->grid()->setVisible(false);
        plot->getPlot().yAxis->setNumberFormat("gbc");
        plot->getPlot().yAxis->setNumberPrecision(2);
        plot->getPlot().yAxis->setLabel("ISK");

        return plot;
    }

    QCPBars *StatisticsWidget::createBarPlot(DateFilteredPlotWidget *plot, const QString &name, Qt::GlobalColor color)
    {
        auto graph = std::make_unique<QCPBars>(plot->getPlot().xAxis, plot->getPlot().yAxis);
        auto graphPtr = graph.get();
        graphPtr->setWidth(3600 * 6);
        graphPtr->setName(name);
        graphPtr->setPen(QPen{color});
        graphPtr->setBrush(color);
        plot->getPlot().addPlottable(graphPtr);
        graph.release();

        return graphPtr;
    }
}
