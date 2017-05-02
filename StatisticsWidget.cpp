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
#include <set>

#include <QBarCategoryAxis>
#include <QDateTimeAxis>
#include <QApplication>
#include <QRadioButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QLineSeries>
#include <QClipboard>
#include <QTabWidget>
#include <QTableView>
#include <QValueAxis>
#include <QBarSeries>
#include <QTextEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QAction>
#include <QDebug>
#include <QHash>
#include <QFont>

#include "CorpMarketOrderValueSnapshotRepository.h"
#include "MarketOrderValueSnapshotRepository.h"
#include "CorpAssetValueSnapshotRepository.h"
#include "WalletJournalEntryRepository.h"
#include "AssetValueSnapshotRepository.h"
#include "CorpWalletSnapshotRepository.h"
#include "WalletTransactionRepository.h"
#include "QtScriptSyntaxHighlighter.h"
#include "WalletSnapshotRepository.h"
#include "DateFilteredPlotWidget.h"
#include "OrderScriptRepository.h"
#include "NumberFormatDelegate.h"
#include "CharacterRepository.h"
#include "RepositoryProvider.h"
#include "StatisticsSettings.h"
#include "UISettings.h"
#include "TextUtils.h"

#include "qcustomplot.h"

#include "StatisticsWidget.h"

namespace Evernus
{
    const QString StatisticsWidget::defaultDateFormat = "hh:mm:ss\ndd.MM.yy";

    StatisticsWidget::StatisticsWidget(const RepositoryProvider &repositoryProvider,
                                       const EveDataProvider &dataProvider,
                                       const ItemCostProvider &itemCostProvider,
                                       QWidget *parent)
        : QWidget(parent)
        , mAssetSnapshotRepository(repositoryProvider.getAssetValueSnapshotRepository())
        , mCorpAssetSnapshotRepository(repositoryProvider.getCorpAssetValueSnapshotRepository())
        , mWalletSnapshotRepository(repositoryProvider.getWalletSnapshotRepository())
        , mCorpWalletSnapshotRepository(repositoryProvider.getCorpWalletSnapshotRepository())
        , mMarketOrderSnapshotRepository(repositoryProvider.getMarketOrderValueSnapshotRepository())
        , mCorpMarketOrderSnapshotRepository(repositoryProvider.getCorpMarketOrderValueSnapshotRepository())
        , mJournalRepository(repositoryProvider.getWalletJournalEntryRepository())
        , mCorpJournalRepository(repositoryProvider.getCorpWalletJournalEntryRepository())
        , mTransactionRepository(repositoryProvider.getWalletTransactionRepository())
        , mCorpTransactionRepository(repositoryProvider.getCorpWalletTransactionRepository())
        , mMarketOrderRepository(repositoryProvider.getMarketOrderRepository())
        , mOrderScriptRepository(repositoryProvider.getOrderScriptRepository())
        , mCharacterRepository(repositoryProvider.getCharacterRepository())
        , mAggrModel(mMarketOrderRepository, dataProvider)
        , mScriptModel(dataProvider, itemCostProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs);

        tabs->addTab(createBasicStatisticsWidget(), tr("Basic"));
        tabs->addTab(createAdvancedStatisticsWidget(), tr("Advanced"));

        connect(&mScriptModel, &ScriptOrderProcessingModel::error, this, &StatisticsWidget::showScriptError);

        mBalancePlot->getChart().legend()->show();
        mJournalPlot->getChart().legend()->show();
        mTransactionPlot->getChart().legend()->show();
    }

    void StatisticsWidget::setCharacter(Character::IdType id)
    {
        qDebug() << "Switching statistics to" << id;

        mCharacterId = id;

        if (mCharacterId == Character::invalidId)
        {
            mAggrApplyBtn->setDisabled(true);
            mScriptApplyBtn->setDisabled(true);
            mBalancePlot->getChart().removeAllSeries();
            mJournalPlot->getChart().removeAllSeries();
            mTransactionPlot->getChart().removeAllSeries();
        }
        else
        {
            mBalancePlot->blockSignals(true);
            mJournalPlot->blockSignals(true);
            mTransactionPlot->blockSignals(true);

            mAggrApplyBtn->setDisabled(false);
            mScriptApplyBtn->setDisabled(false);

            const auto date = QDate::currentDate();

            mBalancePlot->setFrom(date.addDays(-7));
            mBalancePlot->setTo(date);
            mJournalPlot->setFrom(date.addDays(-7));
            mJournalPlot->setTo(date);
            mTransactionPlot->setFrom(date.addDays(-7));
            mTransactionPlot->setTo(date);

            updateData();

            mTransactionPlot->blockSignals(false);
            mJournalPlot->blockSignals(false);
            mBalancePlot->blockSignals(false);
        }

        mAggrModel.clear();
        mScriptModel.clear();
    }

    void StatisticsWidget::updateBalanceData()
    {
        const QDateTime from{mBalancePlot->getFrom()};
        const QDateTime to{mBalancePlot->getTo().addDays(1)};

        const auto combineStats = mCombineStatsBtn->isChecked();
        const auto assetShots = (combineStats) ?
                                (mAssetSnapshotRepository.fetchRange(from.toUTC(), to.toUTC())) :
                                (mAssetSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC()));
        const auto walletShots = (combineStats) ?
                                 (mWalletSnapshotRepository.fetchRange(from.toUTC(), to.toUTC())) :
                                 (mWalletSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC()));
        const auto orderShots = (combineStats) ?
                                (mMarketOrderSnapshotRepository.fetchRange(from.toUTC(), to.toUTC())) :
                                (mMarketOrderSnapshotRepository.fetchRange(mCharacterId, from.toUTC(), to.toUTC()));

        struct PointXComp
        {
            inline bool operator ()(const QPointF &a, const QPointF &b) const noexcept
            {
                return a.x() < b.x();
            }
        };

        using PointContainer = std::set<QPointF, PointXComp>;

        PointContainer assetValues;
        PointContainer walletValues;
        PointContainer corpWalletValues;
        PointContainer corpAssetValues;
        PointContainer buyValues;
        PointContainer sellValues;

        PointContainer buyAndSellValues;

        const auto convertTimestamp = [](const auto &shot) -> qreal {
            return shot->getTimestamp().toMSecsSinceEpoch();
        };

        const auto dataInserter = [&](auto &values, const auto &range) {
            for (const auto &shot : range)
                values.emplace(QPointF{convertTimestamp(shot), shot->getBalance()});
        };

        PointContainer sumData;
        const auto merger = [](const auto &values1, const auto &values2) {
            PointContainer result;

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
                    const auto value = prevValue1 + value2It->y();
                    result.emplace(QPointF{value2It->x(), value});

                    ++value2It;
                }
                else if (value2It == std::end(values2))
                {
                    const auto value = prevValue2 + value1It->y();
                    result.emplace(QPointF{value1It->x(), value});

                    ++value1It;
                }
                else
                {
                    if (value1It->x() < value2It->x())
                    {
                        const auto prevTick = (value2It == std::begin(values2)) ? (values1.begin()->x()) : (std::prev(value2It)->x());
                        const auto div = value2It->x() - prevTick;
                        while (value1It != std::end(values1) && value1It->x() < value2It->x())
                        {
                            prevValue1 = value1It->y();

                            const auto value = lerp(prevValue2, value2It->y(), (value1It->x() - prevTick) / div) + prevValue1;
                            result.emplace(QPointF{value1It->x(), value});

                            ++value1It;
                        }
                    }
                    else if (value1It->x() > value2It->x())
                    {
                        const auto prevTick = (value1It == std::begin(values1)) ? (values2.begin()->x()) : (std::prev(value1It)->x());
                        const auto div = value1It->x() - prevTick;
                        while (value2It != std::end(values2) && value1It->x() > value2It->x())
                        {
                            prevValue2 = value2It->y();

                            const auto value = prevValue2 + lerp(prevValue1, value1It->y(), (value2It->x() - prevTick) / div);
                            result.emplace(QPointF{value2It->x(), value});

                            ++value2It;
                        }
                    }
                    else
                    {
                        prevValue2 = value2It->y();
                        prevValue1 = value1It->y();

                        const auto value = prevValue2 + prevValue1;
                        result.emplace(QPointF{value1It->x(), value});

                        ++value1It;
                        ++value2It;
                    }
                }
            }

            return result;
        };

        const auto combineMaps = [&](auto &target, const auto &characterValuesMap) {
            if (characterValuesMap.empty())
                return;

            target = std::begin(characterValuesMap)->second;

            for (auto it = std::next(std::begin(characterValuesMap)); it != std::end(characterValuesMap); ++it)
                target = merger(target, it->second);
        };

        const auto combineShots = [&](auto &target, const auto &snapshots) {
            std::unordered_map<Evernus::Character::IdType, PointContainer> map;
            for (const auto &snapshot : snapshots)
            {
                const auto secs = convertTimestamp(snapshot);
                map[snapshot->getCharacterId()].emplace(QPointF{secs, snapshot->getBalance()});
            }

            combineMaps(target, map);
        };

        if (combineStats)
        {
            combineShots(assetValues, assetShots);
            combineShots(walletValues, walletShots);

            std::unordered_map<Character::IdType, PointContainer> buyMap, sellMap;
            for (const auto &order : orderShots)
            {
                const auto secs = convertTimestamp(order);
                buyMap[order->getCharacterId()].emplace(QPointF{secs, order->getBuyValue()});
                sellMap[order->getCharacterId()].emplace(QPointF{secs, order->getSellValue()});
            }

            combineMaps(buyValues, buyMap);
            combineMaps(sellValues, sellMap);

            for (auto bIt = std::begin(buyValues), sIt = std::begin(sellValues); bIt != std::end(buyValues); ++bIt, ++sIt)
            {
                Q_ASSERT(bIt->x() == sIt->x());
                buyAndSellValues.emplace(QPointF{bIt->x(), bIt->y() + sIt->y()});
            }
        }
        else
        {
            dataInserter(assetValues, assetShots);
            dataInserter(walletValues, walletShots);

            for (const auto &order : orderShots)
            {
                const auto secs = convertTimestamp(order);

                buyValues.emplace(QPointF{secs, order->getBuyValue()});
                sellValues.emplace(QPointF{secs, order->getSellValue()});

                buyAndSellValues.emplace(QPointF{secs, order->getBuyValue() + order->getSellValue()});
            }
        }

        sumData = merger(assetValues, walletValues);

        try
        {
            const auto corpId = mCharacterRepository.getCorporationId(mCharacterId);
            const auto walletShots = (combineStats) ?
                                     (mCorpWalletSnapshotRepository.fetchRange(from.toUTC(), to.toUTC())) :
                                     (mCorpWalletSnapshotRepository.fetchRange(corpId, from.toUTC(), to.toUTC()));
            const auto assetShots = (combineStats) ?
                                    (mCorpAssetSnapshotRepository.fetchRange(from.toUTC(), to.toUTC())) :
                                    (mCorpAssetSnapshotRepository.fetchRange(corpId, from.toUTC(), to.toUTC()));

            if (!walletShots.empty())
            {
                dataInserter(corpWalletValues, walletShots);
                sumData = merger(sumData, corpWalletValues);
            }
            if (!assetShots.empty())
            {
                dataInserter(corpAssetValues, assetShots);
                sumData = merger(sumData, corpAssetValues);
            }

            const auto corpOrderShots = mCorpMarketOrderSnapshotRepository.fetchRange(corpId, from.toUTC(), to.toUTC());
            if (!corpOrderShots.empty())
            {
                PointContainer corpBuyValues, corpSellValues, corpBuyAndSellValues;
                for (const auto &order : corpOrderShots)
                {
                    const auto secs = convertTimestamp(order);

                    corpBuyValues.emplace(QPointF{secs, order->getBuyValue()});
                    corpSellValues.emplace(QPointF{secs, order->getSellValue()});

                    corpBuyAndSellValues.emplace(QPointF{secs, order->getBuyValue() + order->getSellValue()});
                }

                buyValues = merger(buyValues, corpBuyValues);
                sellValues = merger(sellValues, corpSellValues);
                buyAndSellValues = merger(buyAndSellValues, corpBuyAndSellValues);
            }
        }
        catch (const CharacterRepository::NotFoundException &)
        {
        }

        sumData = merger(sumData, buyAndSellValues);

        auto &chart = mBalancePlot->getChart();
        chart.removeAllSeries();

        const auto axes = chart.axes();
        for (const auto axis : axes)
        {
            chart.removeAxis(axis);
            delete axis;
        }

        auto dateAxis = new QDateTimeAxis{this};
        dateAxis->setLabelsAngle(60);

        auto valueAxis = createISKAxis();

        mBalancePlot->getChart().addAxis(dateAxis, Qt::AlignBottom);
        mBalancePlot->getChart().addAxis(valueAxis, Qt::AlignLeft);

        auto convertData = [](const auto &data) {
            QVector<QPointF> result;
            result.reserve(static_cast<int>(data.size()));

            for (const auto &point : data)
                result << point;

            return result;
        };

        auto addSeries = [&](const auto &data, const auto &name) {
            auto series = new QLineSeries{this};
            series->replace(convertData(data));
            series->setName(name);
//            series->setUseOpenGL(true); - very thin lines
            chart.addSeries(series);
            series->attachAxis(dateAxis);
            series->attachAxis(valueAxis);

            connect(series, &QLineSeries::hovered, this, &StatisticsWidget::updateBalanceTooltip);
        };

        addSeries(sumData, tr("Total value"));
        addSeries(assetValues, tr("Asset value"));
        addSeries(walletValues, tr("Wallet balance"));
        addSeries(corpWalletValues, tr("Corp. wallet balance"));
        addSeries(corpAssetValues, tr("Corp. asset value"));
        addSeries(buyValues, tr("Buy order value"));
        addSeries(sellValues, tr("Sell order value"));

        updateGraphColors();
    }

    void StatisticsWidget::updateJournalData()
    {
        const auto combineStats = mCombineStatsBtn->isChecked();
        const auto entries = (combineStats) ?
                             (mJournalRepository.fetchInRange(QDateTime{mJournalPlot->getFrom()},
                                                              QDateTime{mJournalPlot->getTo().addDays(1)},
                                                              WalletJournalEntryRepository::EntryType::All)) :
                             (mJournalRepository.fetchForCharacterInRange(mCharacterId,
                                                                          QDateTime{mJournalPlot->getFrom()},
                                                                          QDateTime{mJournalPlot->getTo().addDays(1)},
                                                                          WalletJournalEntryRepository::EntryType::All));

        auto totalIncome = 0., totalOutcome = 0.;

        QHash<QDate, std::pair<double, double>> values;
        const auto valueAdder = [&](const auto &entries) {
            for (const auto &entry : entries)
            {
                if (entry->isIgnored())
                    continue;

                auto &value = values[entry->getTimestamp().toLocalTime().date()];

                const auto amount = entry->getAmount();
                if (amount < 0.)
                {
                    totalOutcome -= amount;
                    value.first -= amount;
                }
                else
                {
                    totalIncome += amount;
                    value.second += amount;
                }
            }
        };

        valueAdder(entries);

        QSettings settings;
        if (settings.value(StatisticsSettings::combineCorpAndCharPlotsKey, StatisticsSettings::combineCorpAndCharPlotsDefault).toBool())
        {
            try
            {
                const auto corpId = mCharacterRepository.getCorporationId(mCharacterId);
                const auto entries = (combineStats) ?
                                     (mCorpJournalRepository.fetchInRange(QDateTime{mJournalPlot->getFrom()},
                                                                          QDateTime{mJournalPlot->getTo().addDays(1)},
                                                                          WalletJournalEntryRepository::EntryType::All)) :
                                     (mCorpJournalRepository.fetchForCorporationInRange(corpId,
                                                                                        QDateTime{mJournalPlot->getFrom()},
                                                                                        QDateTime{mJournalPlot->getTo().addDays(1)},
                                                                                        WalletJournalEntryRepository::EntryType::All));

                valueAdder(entries);
            }
            catch (const CharacterRepository::NotFoundException &)
            {
            }
        }

        createBarChart(mJournalPlot->getChart(), tr("Incoming"), tr("Outgoing"), values);

        const auto loc = locale();

        mJournalIncomeLabel->setText(TextUtils::currencyToString(totalIncome, loc));
        mJournalOutcomeLabel->setText(TextUtils::currencyToString(totalOutcome, loc));
        mJournalBalanceLabel->setText(TextUtils::currencyToString(totalIncome - totalOutcome, loc));
    }

    void StatisticsWidget::updateTransactionData()
    {
        const auto combineStats = mCombineStatsBtn->isChecked();
        const auto entries = (combineStats) ?
                             (mTransactionRepository.fetchInRange(QDateTime{mTransactionPlot->getFrom()},
                                                                  QDateTime{mTransactionPlot->getTo().addDays(1)},
                                                                  WalletTransactionRepository::EntryType::All)) :
                             (mTransactionRepository.fetchForCharacterInRange(mCharacterId,
                                                                              QDateTime{mTransactionPlot->getFrom()},
                                                                              QDateTime{mTransactionPlot->getTo().addDays(1)},
                                                                              WalletTransactionRepository::EntryType::All));

        auto totalIncome = 0., totalOutcome = 0.;

        QHash<QDate, std::pair<double, double>> values;
        const auto valueAdder = [&](const auto &entries) {
            for (const auto &entry : entries)
            {
                if (entry->isIgnored())
                    continue;

                auto &value = values[entry->getTimestamp().toLocalTime().date()];

                const auto amount = entry->getPrice() * entry->getQuantity();
                if (entry->getType() == Evernus::WalletTransaction::Type::Buy)
                {
                    totalOutcome += amount;
                    value.first += amount;
                }
                else
                {
                    totalIncome += amount;
                    value.second += amount;
                }
            }
        };

        valueAdder(entries);

        QSettings settings;
        if (settings.value(StatisticsSettings::combineCorpAndCharPlotsKey, StatisticsSettings::combineCorpAndCharPlotsDefault).toBool())
        {
            try
            {
                const auto corpId = mCharacterRepository.getCorporationId(mCharacterId);
                const auto entries = (combineStats) ?
                                     (mCorpTransactionRepository.fetchInRange(QDateTime{mJournalPlot->getFrom()},
                                                                              QDateTime{mJournalPlot->getTo().addDays(1)},
                                                                              WalletTransactionRepository::EntryType::All)) :
                                     (mCorpTransactionRepository.fetchForCorporationInRange(corpId,
                                                                                            QDateTime{mJournalPlot->getFrom()},
                                                                                            QDateTime{mJournalPlot->getTo().addDays(1)},
                                                                                            WalletTransactionRepository::EntryType::All));

                valueAdder(entries);
            }
            catch (const CharacterRepository::NotFoundException &)
            {
            }
        }

        createBarChart(mTransactionPlot->getChart(), tr("Sell"), tr("Buy"), values);

        const auto loc = locale();

        mTransactionsIncomeLabel->setText(TextUtils::currencyToString(totalIncome, loc));
        mTransactionsOutcomeLabel->setText(TextUtils::currencyToString(totalOutcome, loc));
        mTransactionsBalanceLabel->setText(TextUtils::currencyToString(totalIncome - totalOutcome, loc));
    }

    void StatisticsWidget::updateData()
    {
        updateBalanceData();
        updateJournalData();
        updateTransactionData();
    }

    void StatisticsWidget::handleNewPreferences()
    {
        QSettings settings;
        const auto numberFormat
            = settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString();

        auto setNumberFormat = [&](const auto &chart) {
            const auto axes = chart.axes(Qt::Vertical);
            for (const auto axis : axes)
                qobject_cast<QValueAxis *>(axis)->setLabelFormat(numberFormat);
        };

        setNumberFormat(mBalancePlot->getChart());
        setNumberFormat(mJournalPlot->getChart());
        setNumberFormat(mTransactionPlot->getChart());

        if (settings.value(UISettings::applyDateFormatToGraphsKey, UISettings::applyDateFormatToGraphsDefault).toBool())
        {
            const auto dateFormat = settings.value(UISettings::dateTimeFormatKey, defaultDateFormat).toString();
            const auto axes = mBalancePlot->getChart().axes(Qt::Horizontal);
            for (const auto axis : axes)
                qobject_cast<QDateTimeAxis *>(axis)->setFormat(dateFormat);

            updateJournalData();
            updateTransactionData();
        }

        updateGraphColors();
    }

    void StatisticsWidget::applyAggrFilter()
    {
        const auto limit = mAggrLimitEdit->value();
        mAggrModel.reset(mCharacterId,
                         static_cast<MarketOrderRepository::AggregateColumn>(mAggrGroupingColumnCombo->currentData().toInt()),
                         static_cast<MarketOrderRepository::AggregateOrderColumn>(mAggrOrderColumnCombo->currentData().toInt()),
                         (limit == 0) ? (-1) : (limit),
                         mAggrIncludeActiveBtn->isChecked(),
                         mAggrIncludeNotFulfilledBtn->isChecked());

        mAggrView->setModel(&mAggrModel);
    }

    void StatisticsWidget::applyScript()
    {
        mScriptModel.reset(mMarketOrderRepository.fetchForCharacter(mCharacterId),
                           mAggrScriptEdit->toPlainText(),
                           (mScriptForEachModeBtn->isChecked()) ?
                           (ScriptOrderProcessingModel::Mode::ForEach) :
                           (ScriptOrderProcessingModel::Mode::Aggregate));
        mAggrView->setModel(&mScriptModel);
    }

    void StatisticsWidget::copyAggrData()
    {
        const auto indexes = mAggrView->selectionModel()->selectedIndexes();
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
                result.append('\n');
            }

            result.append(mAggrModel.data(index).toString());
            result.append(delim);
        }

        QApplication::clipboard()->setText(result);
    }

    void StatisticsWidget::showScriptError(const QString &message)
    {
        QMessageBox::warning(this, tr("Script error"), message);
    }

    void StatisticsWidget::saveScript()
    {
        const auto name
            = QInputDialog::getText(this, tr("Save script"), tr("Enter script name:"), QLineEdit::Normal, mLastLoadedScript);
        if (!name.isEmpty())
        {
            mLastLoadedScript = name;

            OrderScript script{name};
            script.setCode(mAggrScriptEdit->toPlainText());

            mOrderScriptRepository.store(script);
        }
    }

    void StatisticsWidget::loadScript()
    {
        const auto name
            = QInputDialog::getItem(this, tr("Load script"), tr("Select script:"), mOrderScriptRepository.getAllNames(), 0, false);
        if (!name.isEmpty())
        {
            try
            {
                mAggrScriptEdit->setPlainText(mOrderScriptRepository.find(name)->getCode());
                mLastLoadedScript = name;
            }
            catch (const OrderScriptRepository::NotFoundException &)
            {
            }
        }
    }

    void StatisticsWidget::deleteScript()
    {
        const auto name
            = QInputDialog::getItem(this, tr("Delete script"), tr("Select script:"), mOrderScriptRepository.getAllNames(), 0, false);
        if (!name.isEmpty())
            mOrderScriptRepository.remove(name);
    }

    void StatisticsWidget::updateBalanceTooltip(const QPointF &point, bool state)
    {
        if (!state)
        {
            mBalancePlot->setToolTip(QString{});
            return;
        }

        mBalancePlot->setToolTip(QStringLiteral("%1 %2")
             .arg(QDateTime::fromMSecsSinceEpoch(point.x()).toString())
             .arg(TextUtils::currencyToString(point.y(), locale())));
    }

    void StatisticsWidget::updateGraphColors()
    {
        const auto series = mBalancePlot->getChart().series();
        if (series.isEmpty())
            return;

        const auto sellOrdersValue = static_cast<QLineSeries *>(series[sellOrdersGraph]);
        const auto buyOrdersValue = static_cast<QLineSeries *>(series[buyOrdersGraph]);
        const auto corpWalletValue = static_cast<QLineSeries *>(series[corpWalletBalanceGraph]);
        const auto walletValue = static_cast<QLineSeries *>(series[walletBalanceGraph]);
        const auto assetValue = static_cast<QLineSeries *>(series[assetValueGraph]);
        const auto corpAssetValue = static_cast<QLineSeries *>(series[corpAssetValueGraph]);
        const auto totalValue = static_cast<QLineSeries *>(series[totalValueGraph]);

        QSettings settings;

        sellOrdersValue->setPen(
            settings.value(StatisticsSettings::statisticsSellOrderPlotColorKey, StatisticsSettings::statisticsSellOrderPlotColorDefault).value<QColor>());
        buyOrdersValue->setPen(
            settings.value(StatisticsSettings::statisticsBuyOrderPlotColorKey, StatisticsSettings::statisticsBuyOrderPlotColorDefault).value<QColor>());
        corpWalletValue->setPen(
            settings.value(StatisticsSettings::statisticsCorpWalletPlotColorKey, StatisticsSettings::statisticsCorpWalletPlotColorDefault).value<QColor>());
        walletValue->setPen(
            settings.value(StatisticsSettings::statisticsWalletPlotColorKey, StatisticsSettings::statisticsWalletPlotColorDefault).value<QColor>());
        assetValue->setPen(
            settings.value(StatisticsSettings::statisticsAssetPlotColorKey, StatisticsSettings::statisticsAssetPlotColorDefault).value<QColor>());
        corpAssetValue->setPen(
            settings.value(StatisticsSettings::statisticsCorpAssetPlotColorKey, StatisticsSettings::statisticsCorpAssetPlotColorDefault).value<QColor>());
        totalValue->setPen(
            settings.value(StatisticsSettings::statisticsTotalPlotColorKey, StatisticsSettings::statisticsTotalPlotColorDefault).value<QColor>());
    }

    QWidget *StatisticsWidget::createBasicStatisticsWidget()
    {
        auto widget = new QWidget{this};
        auto mainLayout = new QVBoxLayout{widget};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        QSettings settings;

        mCombineStatsBtn = new QCheckBox{tr("Combine statistics for all characters"), this};
        toolBarLayout->addWidget(mCombineStatsBtn);
        mCombineStatsBtn->setChecked(settings.value(StatisticsSettings::combineStatisticsKey, StatisticsSettings::combineStatisticsDefault).toBool());
        connect(mCombineStatsBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(StatisticsSettings::combineStatisticsKey, checked);

            updateData();
        });

        auto makeSnapshotBtn = new QPushButton{tr("Make snapshot"), this};
        toolBarLayout->addWidget(makeSnapshotBtn);
        connect(makeSnapshotBtn, &QPushButton::clicked, this, &StatisticsWidget::makeSnapshots);

        toolBarLayout->addStretch();

        auto balanceGroup = new QGroupBox{tr("Balance"), this};
        mainLayout->addWidget(balanceGroup);

        auto balanceLayout = new QVBoxLayout{balanceGroup};

        mBalancePlot = createPlot();
        balanceLayout->addWidget(mBalancePlot);
        connect(mBalancePlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateBalanceData);

        auto journalGroup = new QGroupBox{tr("Wallet journal"), this};
        mainLayout->addWidget(journalGroup);

        auto journalLayout = new QVBoxLayout{journalGroup};

        mJournalPlot = createPlot();
        journalLayout->addWidget(mJournalPlot);
        connect(mJournalPlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateJournalData);

        QFont font;
        font.setBold(true);

        auto journalSummaryLayout = new QHBoxLayout{};
        journalLayout->addLayout(journalSummaryLayout);

        journalSummaryLayout->addWidget(new QLabel{tr("Total income:"), this}, 0, Qt::AlignRight);

        mJournalIncomeLabel = new QLabel{"-", this};
        journalSummaryLayout->addWidget(mJournalIncomeLabel, 0, Qt::AlignLeft);
        mJournalIncomeLabel->setFont(font);

        journalSummaryLayout->addWidget(new QLabel{tr("Total cost:"), this}, 0, Qt::AlignRight);

        mJournalOutcomeLabel = new QLabel{"-", this};
        journalSummaryLayout->addWidget(mJournalOutcomeLabel, 0, Qt::AlignLeft);
        mJournalOutcomeLabel->setFont(font);

        journalSummaryLayout->addWidget(new QLabel{tr("Balance:"), this}, 0, Qt::AlignRight);

        mJournalBalanceLabel = new QLabel{"-", this};
        journalSummaryLayout->addWidget(mJournalBalanceLabel, 0, Qt::AlignLeft);
        mJournalBalanceLabel->setFont(font);

        journalSummaryLayout->addStretch();

        auto transactionGroup = new QGroupBox{tr("Wallet transactions"), this};
        mainLayout->addWidget(transactionGroup);

        auto transactionLayout = new QVBoxLayout{transactionGroup};

        mTransactionPlot = createPlot();
        transactionLayout->addWidget(mTransactionPlot);
        connect(mTransactionPlot, &DateFilteredPlotWidget::filterChanged, this, &StatisticsWidget::updateTransactionData);

        auto transactionSummaryLayout = new QHBoxLayout{};
        transactionLayout->addLayout(transactionSummaryLayout);

        transactionSummaryLayout->addWidget(new QLabel{tr("Total income:"), this}, 0, Qt::AlignRight);

        mTransactionsIncomeLabel = new QLabel{"-", this};
        transactionSummaryLayout->addWidget(mTransactionsIncomeLabel, 0, Qt::AlignLeft);
        mTransactionsIncomeLabel->setFont(font);

        transactionSummaryLayout->addWidget(new QLabel{tr("Total cost:"), this}, 0, Qt::AlignRight);

        mTransactionsOutcomeLabel = new QLabel{"-", this};
        transactionSummaryLayout->addWidget(mTransactionsOutcomeLabel, 0, Qt::AlignLeft);
        mTransactionsOutcomeLabel->setFont(font);

        transactionSummaryLayout->addWidget(new QLabel{tr("Balance:"), this}, 0, Qt::AlignRight);

        mTransactionsBalanceLabel = new QLabel{"-", this};
        transactionSummaryLayout->addWidget(mTransactionsBalanceLabel, 0, Qt::AlignLeft);
        mTransactionsBalanceLabel->setFont(font);

        transactionSummaryLayout->addStretch();

        mainLayout->addStretch();

        return widget;
    }

    QWidget *StatisticsWidget::createAdvancedStatisticsWidget()
    {
        auto widget = new QWidget{this};
        auto mainLayout = new QVBoxLayout{widget};

        mainLayout->addWidget(new QLabel{tr(
            "This tab allows you to create custom reports aggregating historic market order data."), this});

        auto configGroup = new QGroupBox{this};
        mainLayout->addWidget(configGroup);

        auto configLayout = new QVBoxLayout{configGroup};

        auto simpleConfigBtn = new QRadioButton{tr("Simple aggregation"), this};
        configLayout->addWidget(simpleConfigBtn);
        simpleConfigBtn->setChecked(true);

        auto simpleConfigWidget = new QWidget{this};
        configLayout->addWidget(simpleConfigWidget);
        connect(simpleConfigBtn, &QRadioButton::toggled, simpleConfigWidget, &QWidget::setVisible);

        auto simgpleConfigLayout = new QHBoxLayout{simpleConfigWidget};

        simgpleConfigLayout->addWidget(new QLabel{tr("Group by:"), this});

        mAggrGroupingColumnCombo = new QComboBox{this};
        simgpleConfigLayout->addWidget(mAggrGroupingColumnCombo);
        mAggrGroupingColumnCombo->addItem(tr("Type"), static_cast<int>(MarketOrderRepository::AggregateColumn::TypeId));
        mAggrGroupingColumnCombo->addItem(tr("Location"), static_cast<int>(MarketOrderRepository::AggregateColumn::LocationId));

        simgpleConfigLayout->addWidget(new QLabel{tr("Order by:"), this});

        mAggrOrderColumnCombo = new QComboBox{this};
        simgpleConfigLayout->addWidget(mAggrOrderColumnCombo);
        mAggrOrderColumnCombo->addItem(tr("Id"), static_cast<int>(MarketOrderRepository::AggregateOrderColumn::Id));
        mAggrOrderColumnCombo->addItem(tr("Count"), static_cast<int>(MarketOrderRepository::AggregateOrderColumn::Count));
        mAggrOrderColumnCombo->addItem(tr("Price"), static_cast<int>(MarketOrderRepository::AggregateOrderColumn::Price));
        mAggrOrderColumnCombo->addItem(tr("Volume"), static_cast<int>(MarketOrderRepository::AggregateOrderColumn::Volume));

        simgpleConfigLayout->addWidget(new QLabel{tr("Limit:"), this});

        mAggrLimitEdit = new QSpinBox{this};
        simgpleConfigLayout->addWidget(mAggrLimitEdit);
        mAggrLimitEdit->setValue(10);
        mAggrLimitEdit->setMaximum(10000);
        mAggrLimitEdit->setSpecialValueText(tr("none"));

        mAggrIncludeActiveBtn = new QCheckBox{tr("Include active"), this};
        simgpleConfigLayout->addWidget(mAggrIncludeActiveBtn);

        mAggrIncludeNotFulfilledBtn = new QCheckBox{tr("Include expired/cancelled"), this};
        simgpleConfigLayout->addWidget(mAggrIncludeNotFulfilledBtn);

        mAggrApplyBtn = new QPushButton{tr("Apply"), this};
        simgpleConfigLayout->addWidget(mAggrApplyBtn);
        mAggrApplyBtn->setDisabled(true);
        connect(mAggrApplyBtn, &QPushButton::clicked, this, &StatisticsWidget::applyAggrFilter);

        simgpleConfigLayout->addStretch();

        auto scriptConfigBtn = new QRadioButton{tr("Script processing"), this};
        configLayout->addWidget(scriptConfigBtn);

        auto scriptConfigWidget = new QWidget{this};
        configLayout->addWidget(scriptConfigWidget);
        scriptConfigWidget->setVisible(false);
        connect(scriptConfigBtn, &QRadioButton::toggled, scriptConfigWidget, &QWidget::setVisible);

        auto scriptConfigLayout = new QHBoxLayout{scriptConfigWidget};

        QFont scriptFont{"Monospace"};
        scriptFont.setFixedPitch(true);
        scriptFont.setStyleHint(QFont::TypeWriter);

        mAggrScriptEdit = new QTextEdit{this};
        scriptConfigLayout->addWidget(mAggrScriptEdit, 1);
        mAggrScriptEdit->setPlaceholderText(tr("see the online help to learn how to use script processing"));
        mAggrScriptEdit->document()->setDefaultFont(scriptFont);

        new QtScriptSyntaxHighlighter{mAggrScriptEdit};

        auto scriptControlsLayout = new QVBoxLayout{};
        scriptConfigLayout->addLayout(scriptControlsLayout);

        mScriptApplyBtn = new QPushButton{tr("Apply"), this};
        scriptControlsLayout->addWidget(mScriptApplyBtn);
        mScriptApplyBtn->setDisabled(true);
        connect(mScriptApplyBtn, &QPushButton::clicked, this, &StatisticsWidget::applyScript);

        auto saveScriptBtn = new QPushButton{tr("Save script..."), this};
        scriptControlsLayout->addWidget(saveScriptBtn);
        connect(saveScriptBtn, &QPushButton::clicked, this, &StatisticsWidget::saveScript);

        auto loadScriptBtn = new QPushButton{tr("Load script..."), this};
        scriptControlsLayout->addWidget(loadScriptBtn);
        connect(loadScriptBtn, &QPushButton::clicked, this, &StatisticsWidget::loadScript);

        auto deleteScriptBtn = new QPushButton{tr("Delete script..."), this};
        scriptControlsLayout->addWidget(deleteScriptBtn);
        connect(deleteScriptBtn, &QPushButton::clicked, this, &StatisticsWidget::deleteScript);

        auto scriptModeGroup = new QGroupBox{tr("Mode"), this};
        scriptControlsLayout->addWidget(scriptModeGroup);

        auto scriptModeGroupLayout = new QVBoxLayout{scriptModeGroup};

        mScriptForEachModeBtn = new QRadioButton{tr("For each"), this};
        scriptModeGroupLayout->addWidget(mScriptForEachModeBtn);
        mScriptForEachModeBtn->setChecked(true);

        auto scriptAggrgateModeBtn = new QRadioButton{tr("Aggregate"), this};
        scriptModeGroupLayout->addWidget(scriptAggrgateModeBtn);

        scriptControlsLayout->addStretch();

        auto copyAct = new QAction{this};
        copyAct->setShortcuts(QKeySequence::Copy);
        connect(copyAct, &QAction::triggered, this, &StatisticsWidget::copyAggrData);

        mAggrView = new QTableView{this};
        mainLayout->addWidget(mAggrView, 1);
        mAggrView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        mAggrView->addAction(copyAct);
        mAggrView->setItemDelegate(new NumberFormatDelegate{this});

        return widget;
    }

    DateFilteredPlotWidget *StatisticsWidget::createPlot()
    {
        return new DateFilteredPlotWidget{this};
    }

    QValueAxis *StatisticsWidget::createISKAxis()
    {
        QSettings settings;

        auto axis = new QValueAxis{this};
        axis->setLabelFormat(settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString());
        axis->setTitleText("ISK");

        return axis;
    }

    QBarSet *StatisticsWidget::createBarSet(const QString &name, Qt::GlobalColor color)
    {
        auto set = new QBarSet{name, this};
        set->setColor(color);

        return set;
    }

    void StatisticsWidget::createBarChart(QChart &chart, const QString &posName, const QString &negName, const DatePosNegMap &values)
    {
        chart.removeAllSeries();

        const auto axes = chart.axes();
        for (const auto axis : axes)
        {
            chart.removeAxis(axis);
            delete axis;
        }

        const auto posPlot = createBarSet(posName, Qt::green);
        const auto negPlot = createBarSet(negName, Qt::red);

        QStringList dates;
        createBarTicks(*posPlot, *negPlot, dates, values);

        const auto categoryAxis = new QBarCategoryAxis{this};
        categoryAxis->setCategories(dates);

        const auto valueAxis = createISKAxis();

        chart.addAxis(categoryAxis, Qt::AlignBottom);
        chart.addAxis(valueAxis, Qt::AlignLeft);

        auto journalSeries = new QBarSeries{this};
        journalSeries->append(posPlot);
        journalSeries->append(negPlot);
        chart.addSeries(journalSeries);
        journalSeries->attachAxis(categoryAxis);
        journalSeries->attachAxis(valueAxis);
    }

    void StatisticsWidget::createBarTicks(QBarSet &incoming,
                                          QBarSet &outgoing,
                                          QStringList &dates,
                                          const DatePosNegMap &values)
    {
        incoming.remove(0);
        outgoing.remove(0);

        QHashIterator<QDate, std::pair<double, double>> it{values};
        while (it.hasNext())
        {
            it.next();

            dates << it.key().toString();

            outgoing << it.value().first;
            incoming << it.value().second;
        }
    }
}
