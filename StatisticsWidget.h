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
#pragma once

#include <QWidget>
#include <QBarSet>

#include "ScriptOrderProcessingModel.h"
#include "AggregatedStatisticsModel.h"

QT_CHARTS_USE_NAMESPACE

QT_CHARTS_BEGIN_NAMESPACE
class QValueAxis;
class QChart;
QT_CHARTS_END_NAMESPACE

class QRadioButton;
class QPushButton;
class QTableView;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLabel;

namespace Evernus
{
    class CorpMarketOrderValueSnapshotRepository;
    class MarketOrderValueSnapshotRepository;
    class CorpAssetValueSnapshotRepository;
    class WalletJournalEntryRepository;
    class AssetValueSnapshotRepository;
    class CorpWalletSnapshotRepository;
    class WalletTransactionRepository;
    class WalletSnapshotRepository;
    class DateFilteredPlotWidget;
    class OrderScriptRepository;
    class CharacterRepository;
    class RepositoryProvider;
    class ItemCostProvider;

    class StatisticsWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        StatisticsWidget(const RepositoryProvider &repositoryProvider,
                         const EveDataProvider &dataProvider,
                         const ItemCostProvider &itemCostProvider,
                         QWidget *parent = nullptr);
        virtual ~StatisticsWidget() = default;

    signals:
        void makeSnapshots();

    public slots:
        void setCharacter(Character::IdType id);
        void updateBalanceData();
        void updateJournalData();
        void updateTransactionData();
        void updateData();
        void handleNewPreferences();

    private slots:
        void applyAggrFilter();
        void applyScript();

        void copyAggrData();

        void showScriptError(const QString &message);

        void saveScript();
        void loadScript();
        void deleteScript();

        void updateBalanceTooltip(const QPointF &point, bool state);

    private:
        enum
        {
            assetValueGraph,
            walletBalanceGraph,
            corpWalletBalanceGraph,
            corpAssetValueGraph,
            buyOrdersGraph,
            sellOrdersGraph,
            totalValueGraph,
        };

        using DatePosNegMap = QHash<QDate, std::pair<double, double>>;

        static const QString defaultDateFormat;

        const AssetValueSnapshotRepository &mAssetSnapshotRepository;
        const CorpAssetValueSnapshotRepository &mCorpAssetSnapshotRepository;
        const WalletSnapshotRepository &mWalletSnapshotRepository;
        const CorpWalletSnapshotRepository &mCorpWalletSnapshotRepository;
        const MarketOrderValueSnapshotRepository &mMarketOrderSnapshotRepository;
        const CorpMarketOrderValueSnapshotRepository &mCorpMarketOrderSnapshotRepository;
        const WalletJournalEntryRepository &mJournalRepository, &mCorpJournalRepository;
        const WalletTransactionRepository &mTransactionRepository, &mCorpTransactionRepository;
        const MarketOrderRepository &mMarketOrderRepository;
        const OrderScriptRepository &mOrderScriptRepository;
        const CharacterRepository &mCharacterRepository;

        DateFilteredPlotWidget *mBalancePlot = nullptr;
        DateFilteredPlotWidget *mJournalPlot = nullptr;
        DateFilteredPlotWidget *mTransactionPlot = nullptr;

        QCheckBox *mCombineStatsBtn = nullptr;
        QPushButton *mAggrApplyBtn = nullptr;
        QPushButton *mScriptApplyBtn = nullptr;
        QComboBox *mAggrGroupingColumnCombo = nullptr;
        QComboBox *mAggrOrderColumnCombo = nullptr;
        QSpinBox *mAggrLimitEdit = nullptr;
        QCheckBox *mAggrIncludeActiveBtn = nullptr;
        QCheckBox *mAggrIncludeNotFulfilledBtn = nullptr;
        QTextEdit *mAggrScriptEdit = nullptr;
        QRadioButton *mScriptForEachModeBtn = nullptr;
        QTableView *mAggrView = nullptr;
        QLabel *mJournalIncomeLabel = nullptr;
        QLabel *mJournalOutcomeLabel = nullptr;
        QLabel *mJournalBalanceLabel = nullptr;
        QLabel *mTransactionsIncomeLabel = nullptr;
        QLabel *mTransactionsOutcomeLabel = nullptr;
        QLabel *mTransactionsBalanceLabel = nullptr;

        Character::IdType mCharacterId = Character::invalidId;

        AggregatedStatisticsModel mAggrModel;
        ScriptOrderProcessingModel mScriptModel;

        QString mLastLoadedScript;

        void updateGraphColors();

        QWidget *createBasicStatisticsWidget();
        QWidget *createAdvancedStatisticsWidget();

        DateFilteredPlotWidget *createPlot();
        QValueAxis *createISKAxis();

        QBarSet *createBarSet(const QString &name, Qt::GlobalColor color);
        void createBarChart(QChart &chart,
                            const QString &posName,
                            const QString &negName,
                            const DatePosNegMap &values);

        static void createBarTicks(QBarSet &incoming,
                                   QBarSet &outgoing,
                                   QStringList &dates,
                                   const DatePosNegMap &values);
    };
}
