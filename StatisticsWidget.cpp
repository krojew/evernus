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
#include <memory>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QHash>

#include "MarketOrderValueSnapshotRepository.h"
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
                                       const MarketOrderValueSnapshotRepository &marketOrderSnapshotRepo,
                                       const WalletJournalEntryRepository &journalRepo,
                                       const WalletTransactionRepository &transactionRepo,
                                       QWidget *parent)
        : QWidget{parent}
        , mAssetSnapshotRepository{assetSnapshotRepo}
        , mWalletSnapshotRepository{walletSnapshotRepo}
        , mMarketOrderSnapshotRepository{marketOrderSnapshotRepo}
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

            mBalancePlot->setFrom(date.addDays(-7));
            mBalancePlot->setTo(date);
            mJournalPlot->setFrom(date.addDays(-7));
            mJournalPlot->setTo(date);
            mTransactionPlot->setFrom(date.addDays(-7));
            mTransactionPlot->setTo(date);

            updateBalanceData();
            updateJournalData();
            updateTransactionData();

            mTransactionPlot->blockSignals(false);
            mJournalPlot->blockSignals(false);
            mBalancePlot->blockSignals(false);
        }
    }

    void StatisticsWidget::updateBalanceData()
    {
        const QDateTime from{mBalancePlot->getFrom()};
        const QDateTime to{mBalancePlot->getTo().addDays(1)};

        const auto assetShots = mAssetSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC());
        const auto walletShots = mWalletSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC());
        const auto orderShots = mMarketOrderSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC());

        auto assetGraph = mBalancePlot->getPlot().graph(assetValueGraph);
        auto walletGraph = mBalancePlot->getPlot().graph(walletBalanceGraph);
        auto buyGraph = mBalancePlot->getPlot().graph(buyOrdersGraph);
        auto sellGraph = mBalancePlot->getPlot().graph(sellOrdersGraph);
        auto sumGraph = mBalancePlot->getPlot().graph(totalValueGraph);

        auto assetValues = std::make_unique<QCPDataMap>();
        auto walletValues = std::make_unique<QCPDataMap>();
        auto buyValues = std::make_unique<QCPDataMap>();
        auto sellValues = std::make_unique<QCPDataMap>();;

        QCPDataMap buyAndSellValues;

        auto yMax = -1.;

        const auto dataInserter = [](auto &values, const auto &range) {
            for (const auto &shot : range)
            {
                const auto secs = shot->getTimestamp().toMSecsSinceEpoch() / 1000.;
                values->insert(secs, QCPData{secs, shot->getBalance()});
            }
        };

        dataInserter(assetValues, assetShots);
        dataInserter(walletValues, walletShots);

        for (const auto &order : orderShots)
        {
            const auto secs = order->getTimestamp().toMSecsSinceEpoch() / 1000.;

            buyValues->insert(secs, QCPData{secs, order->getBuyValue()});
            sellValues->insert(secs, QCPData{secs, order->getSellValue()});

            buyAndSellValues.insert(secs, QCPData{secs, order->getBuyValue() + order->getSellValue()});
        }

        auto sumData = std::make_unique<QCPDataMap>();
        const auto merger = [&yMax](const auto &values1, const auto &values2) -> QCPDataMap {
            QCPDataMap result;

            auto value1It = std::begin(values1);
            auto value2It = std::begin(values2);
            auto prevValue1 = 0.;
            auto prevValue2 = 0.;

            const auto lerp = [](double a, double b, double t) {
                return a + (b - a) * t;
            };

            while (value1It != std::end(values1) || value2It != std::end(values2))
            {
                if (value1It == std::end(values1))
                {
                    const auto value = prevValue1 + value2It->value;
                    if (value > yMax)
                        yMax = value;

                    result.insert(value2It->key, QCPData{value2It->key, value});

                    ++value2It;
                }
                else if (value2It == std::end(values2))
                {
                    const auto value = prevValue2 + value1It->value;
                    if (value > yMax)
                        yMax = value;

                    result.insert(value1It->key, QCPData{value1It->key, value});

                    ++value1It;
                }
                else
                {
                    if (value1It->key < value2It->key)
                    {
                        const auto prevTick = (value2It == std::begin(values2)) ? (values1.firstKey()) : (std::prev(value2It)->key);
                        const auto div = value2It->key - prevTick;
                        while (value1It->key < value2It->key && value1It != std::end(values1))
                        {
                            prevValue1 = value1It->value;

                            const auto value = lerp(prevValue2, value2It->value, (value1It->key - prevTick) / div) + prevValue1;
                            if (value > yMax)
                                yMax = value;

                            result.insert(value1It->key, QCPData{value1It->key, value});

                            ++value1It;
                        }
                    }
                    else if (value1It->key > value2It->key)
                    {
                        const auto prevTick = (value1It == std::begin(values1)) ? (values2.firstKey()) : (std::prev(value1It)->key);
                        const auto div = value1It->key - prevTick;
                        while (value1It->key > value2It->key && value2It != std::end(values2))
                        {
                            prevValue2 = value2It->value;

                            const auto value = prevValue2 + lerp(prevValue1, value1It->value, (value2It->key - prevTick) / div);
                            if (value > yMax)
                                yMax = value;

                            result.insert(value2It->key, QCPData{value2It->key, value});

                            ++value2It;
                        }
                    }
                    else
                    {
                        prevValue2 = value2It->value;
                        prevValue1 = value1It->value;

                        const auto value = prevValue2 + prevValue1;
                        if (value > yMax)
                            yMax = value;

                        result.insert(value1It->key, QCPData{value1It->key, value});

                        ++value1It;
                        ++value2It;
                    }
                }
            }

            return result;
        };

        *sumData = merger(*assetValues, *walletValues);
        *sumData = merger(*sumData, buyAndSellValues);

        QVector<double> sumTicks;
        for (const auto &entry : *sumData)
            sumTicks << entry.key;

        mBalancePlot->getPlot().xAxis->setTickVector(sumTicks);

        assetGraph->setData(assetValues.get(), false);
        assetValues.release();

        walletGraph->setData(walletValues.get(), false);
        walletValues.release();

        buyGraph->setData(buyValues.get(), false);
        buyValues.release();

        sellGraph->setData(sellValues.get(), false);
        sellValues.release();

        sumGraph->setData(sumData.get(), false);
        sumData.release();

        mBalancePlot->getPlot().xAxis->rescale();
        mBalancePlot->getPlot().yAxis->setRange(0., yMax);
        mBalancePlot->getPlot().replot();
    }

    void StatisticsWidget::updateJournalData()
    {
        const auto entries = mJournalRepo.fetchForCharacterInRange(mCharacterId,
                                                                   QDateTime{mJournalPlot->getFrom()},
                                                                   QDateTime{mJournalPlot->getTo().addDays(1)},
                                                                   WalletJournalEntryRepository::EntryType::All);

        QHash<QDate, std::pair<double, double>> values;
        for (const auto &entry : entries)
        {
            if (entry->isIgnored())
                continue;

            auto &value = values[entry->getTimestamp().toLocalTime().date()];

            const auto amount = entry->getAmount();
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
                                                                       QDateTime{mTransactionPlot->getTo().addDays(1)},
                                                                       WalletTransactionRepository::EntryType::All);

        QHash<QDate, std::pair<double, double>> values;
        for (const auto &entry : entries)
        {
            if (entry->isIgnored())
                continue;

            auto &value = values[entry->getTimestamp().toLocalTime().date()];

            const auto amount = entry->getPrice();
            if (entry->getType() == WalletTransaction::Type::Buy)
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

        auto buyGraph = mBalancePlot->getPlot().addGraph();
        buyGraph->setName(tr("Buy order value"));
        buyGraph->setPen(QPen{Qt::yellow});

        auto sellGraph = mBalancePlot->getPlot().addGraph();
        sellGraph->setName(tr("Sell order value"));
        sellGraph->setPen(QPen{Qt::magenta});

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
