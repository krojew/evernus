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
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QSettings>
#include <QLabel>
#include <QDebug>
#include <QMenu>

#include "MarketOrderViewWithTransactions.h"
#include "MarketOrderFilterWidget.h"
#include "MarketOrderProvider.h"
#include "CacheTimerProvider.h"
#include "WarningBarWidget.h"
#include "MarketOrderView.h"
#include "DateRangeWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"

#include "MarketOrderWidget.h"

namespace Evernus
{
    const QString MarketOrderWidget::settingsLastTabkey = "ui/orders/lastTab";

    MarketOrderWidget::MarketOrderWidget(const MarketOrderProvider &orderProvider,
                                         const CacheTimerProvider &cacheTimerProvider,
                                         const EveDataProvider &dataProvider,
                                         ItemCostProvider &itemCostProvider,
                                         const WalletTransactionRepository &transactionsRepo,
                                         const CharacterRepository &characterRepository,
                                         const FilterTextRepository &filterRepo,
                                         bool corp,
                                         QWidget *parent)
        : CharacterBoundWidget(std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpMarketOrders) : (TimerType::MarketOrders)),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpMarketOrders) : (TimerType::MarketOrders)),
                               ImportSettings::maxMarketOrdersAgeKey,
                               parent)
        , mOrderProvider(orderProvider)
        , mSellModel(mOrderProvider, dataProvider, itemCostProvider, cacheTimerProvider, characterRepository, corp)
        , mBuyModel(mOrderProvider, dataProvider, cacheTimerProvider, characterRepository, corp)
        , mArchiveModel(mOrderProvider, dataProvider, itemCostProvider, characterRepository, corp)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        mLogImportBtn = new QPushButton{QIcon{":/images/page_refresh.png"}, tr("File import"), this};
        toolBarLayout->addWidget(mLogImportBtn);
        mLogImportBtn->setEnabled(false);
        mLogImportBtn->setFlat(true);
        connect(mLogImportBtn, &QPushButton::clicked, this, [this]() {
            emit importFromLogs(getCharacterId());
        });

        auto importMenu = new QMenu{this};

        importMenu->addAction(QIcon{":/images/world.png"}, tr("Import prices from Web"), this, SLOT(prepareItemImportFromWeb()));
        importMenu->addAction(QIcon{":/images/page_refresh.png"}, tr("Import prices from logs"), this, SLOT(prepareItemImportFromFile()));
        importMenu->addAction(QIcon{":/images/disk_multiple.png"}, tr("Import prices from cache"), this, SLOT(prepareItemImportFromCache()));

        auto allImportBtn = new QPushButton{QIcon{":/images/arrow_refresh_small.png"}, tr("Import prices  "), this};
        toolBarLayout->addWidget(allImportBtn);
        allImportBtn->setFlat(true);
        allImportBtn->setMenu(importMenu);

        auto openMarginTool = new QPushButton{QIcon{":/images/report.png"}, tr("Open margin tool"), this};
        toolBarLayout->addWidget(openMarginTool);
        openMarginTool->setFlat(true);
        connect(openMarginTool, &QPushButton::clicked, this, &MarketOrderWidget::openMarginTool);

        auto stateFilter = new MarketOrderFilterWidget{filterRepo, this};
        toolBarLayout->addWidget(stateFilter);

        toolBarLayout->addWidget(new QLabel{tr("Group by:"), this});

        mGroupingCombo = new QComboBox{this};
        toolBarLayout->addWidget(mGroupingCombo);
        mGroupingCombo->addItem(tr("- none -"), static_cast<int>(MarketOrderModel::Grouping::None));
        mGroupingCombo->addItem(tr("Type"), static_cast<int>(MarketOrderModel::Grouping::Type));
        mGroupingCombo->addItem(tr("Group"), static_cast<int>(MarketOrderModel::Grouping::Group));
        mGroupingCombo->addItem(tr("Station"), static_cast<int>(MarketOrderModel::Grouping::Station));
        connect(mGroupingCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeGrouping()));

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);

        auto mainTabs = new QTabWidget{this};
        mainLayout->addWidget(mainTabs);

        mSellView = new MarketOrderViewWithTransactions{transactionsRepo, dataProvider, itemCostProvider, this};
        mainTabs->addTab(mSellView, QIcon{":/images/arrow_out.png"}, tr("Sell"));
        mSellView->setModel(&mSellModel);
        mSellView->sortByColumn(0, Qt::AscendingOrder);
        connect(this, &MarketOrderWidget::characterChanged, mSellView, &MarketOrderViewWithTransactions::setCharacter);
        connect(mSellView, &MarketOrderViewWithTransactions::scriptError, this, &MarketOrderWidget::showScriptError);
        connect(stateFilter, &MarketOrderFilterWidget::statusFilterChanged, mSellView, &MarketOrderViewWithTransactions::statusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::priceStatusFilterChanged, mSellView, &MarketOrderViewWithTransactions::priceStatusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::textFilterChanged, mSellView, &MarketOrderViewWithTransactions::textFilterChanged);

        mBuyView = new MarketOrderViewWithTransactions{transactionsRepo, dataProvider, itemCostProvider, this};
        mainTabs->addTab(mBuyView, QIcon{":/images/arrow_in.png"}, tr("Buy"));
        mBuyView->setModel(&mBuyModel);
        mBuyView->sortByColumn(0, Qt::AscendingOrder);
        connect(this, &MarketOrderWidget::characterChanged, mBuyView, &MarketOrderViewWithTransactions::setCharacter);
        connect(stateFilter, &MarketOrderFilterWidget::statusFilterChanged, mBuyView, &MarketOrderViewWithTransactions::statusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::priceStatusFilterChanged, mBuyView, &MarketOrderViewWithTransactions::priceStatusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::textFilterChanged, mBuyView, &MarketOrderViewWithTransactions::textFilterChanged);

        auto combinedWidget = new QWidget{this};
        mainTabs->addTab(combinedWidget, QIcon{":/images/arrow_inout.png"}, tr("Sell && Buy"));

        auto combinedLayout = new QVBoxLayout{};
        combinedWidget->setLayout(combinedLayout);

        auto sellGroup = new QGroupBox{tr("Sell orders"), this};
        combinedLayout->addWidget(sellGroup);

        auto sellGroupLayout = new QVBoxLayout{};
        sellGroup->setLayout(sellGroupLayout);

        mCombinedSellView = new MarketOrderView{dataProvider, this};
        sellGroupLayout->addWidget(mCombinedSellView);
        mCombinedSellView->setModel(&mSellModel);
        mCombinedSellView->sortByColumn(0, Qt::AscendingOrder);
        connect(stateFilter, &MarketOrderFilterWidget::statusFilterChanged, mCombinedSellView, &MarketOrderView::statusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::priceStatusFilterChanged, mCombinedSellView, &MarketOrderView::priceStatusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::textFilterChanged, mCombinedSellView, &MarketOrderView::textFilterChanged);

        auto buyGroup = new QGroupBox{tr("Buy orders"), this};
        combinedLayout->addWidget(buyGroup);

        auto buyGroupLayout = new QVBoxLayout{};
        buyGroup->setLayout(buyGroupLayout);

        mCombinedBuyView = new MarketOrderView{dataProvider, this};
        buyGroupLayout->addWidget(mCombinedBuyView);
        mCombinedBuyView->setModel(&mBuyModel);
        mCombinedBuyView->sortByColumn(0, Qt::AscendingOrder);
        connect(stateFilter, &MarketOrderFilterWidget::statusFilterChanged, mCombinedBuyView, &MarketOrderView::statusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::priceStatusFilterChanged, mCombinedBuyView, &MarketOrderView::priceStatusFilterChanged);
        connect(stateFilter, &MarketOrderFilterWidget::textFilterChanged, mCombinedBuyView, &MarketOrderView::textFilterChanged);

        auto archiveTab = new QWidget{this};
        mainTabs->addTab(archiveTab, QIcon{":/images/hourglass.png"}, tr("History"));

        auto archiveLayout = new QVBoxLayout{};
        archiveTab->setLayout(archiveLayout);

        auto rangeLayout = new QHBoxLayout{};
        archiveLayout->addLayout(rangeLayout);

        mArchiveRangeEdit = new DateRangeWidget{this};
        rangeLayout->addWidget(mArchiveRangeEdit);
        connect(mArchiveRangeEdit, &DateRangeWidget::rangeChanged, this, &MarketOrderWidget::setArchiveRange);

        rangeLayout->addStretch();

        mArchiveView = new MarketOrderViewWithTransactions{transactionsRepo, dataProvider, itemCostProvider, this};
        archiveLayout->addWidget(mArchiveView);
        mArchiveView->setShowInfo(false);
        mArchiveView->statusFilterChanged(MarketOrderFilterProxyModel::EveryStatus);
        mArchiveView->priceStatusFilterChanged(MarketOrderFilterProxyModel::EveryPriceStatus);
        mArchiveView->setModel(&mArchiveModel);
        mArchiveView->sortByColumn(0, Qt::DescendingOrder);
        connect(this, &MarketOrderWidget::characterChanged, mArchiveView, &MarketOrderViewWithTransactions::setCharacter);
        connect(stateFilter, &MarketOrderFilterWidget::textFilterChanged, mArchiveView, &MarketOrderViewWithTransactions::textFilterChanged);

        QSettings settings;
        mainTabs->setCurrentIndex(settings.value(settingsLastTabkey).toInt());
        connect(mainTabs, &QTabWidget::currentChanged, this, &MarketOrderWidget::saveChosenTab);

        connect(this, &MarketOrderWidget::characterChanged, &mSellModel, &MarketOrderSellModel::setCharacter);
        connect(this, &MarketOrderWidget::characterChanged, &mBuyModel, &MarketOrderBuyModel::setCharacter);
    }

    void MarketOrderWidget::updateData()
    {
        refreshImportTimer();
        mSellModel.reset();
        mBuyModel.reset();
        mArchiveModel.reset();
    }

    void MarketOrderWidget::changeGrouping()
    {
        mSellModel.setGrouping(static_cast<MarketOrderModel::Grouping>(mGroupingCombo->currentData().toInt()));
        mSellView->expandAll();
        mCombinedSellView->expandAll();
        mBuyModel.setGrouping(static_cast<MarketOrderModel::Grouping>(mGroupingCombo->currentData().toInt()));
        mBuyView->expandAll();
        mCombinedBuyView->expandAll();
        mArchiveModel.setGrouping(static_cast<MarketOrderModel::Grouping>(mGroupingCombo->currentData().toInt()));
        mArchiveView->expandAll();
    }

    void MarketOrderWidget::saveChosenTab(int index)
    {
        QSettings settings;
        settings.setValue(settingsLastTabkey, index);
    }

    void MarketOrderWidget::prepareItemImportFromWeb()
    {
        emit importPricesFromWeb(getImportTarget());
    }

    void MarketOrderWidget::prepareItemImportFromFile()
    {
        emit importPricesFromFile(getImportTarget());
    }

    void MarketOrderWidget::prepareItemImportFromCache()
    {
        emit importPricesFromCache(getImportTarget());
    }

    void MarketOrderWidget::setArchiveRange(const QDate &from, const QDate &to)
    {
        mArchiveModel.setCharacterAndRange(getCharacterId(), QDateTime{from}.toUTC(), QDateTime{to}.addDays(1).toUTC());
    }

    void MarketOrderWidget::showScriptError(const QString &message)
    {
        QMessageBox::warning(this, tr("Script error"), message);
    }

    void MarketOrderWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching market orders to" << id;

        const auto tillDate = QDate::currentDate();
        const auto fromDate = tillDate.addDays(-7);

        mArchiveRangeEdit->blockSignals(true);
        mArchiveRangeEdit->setRange(fromDate, tillDate);
        mArchiveRangeEdit->blockSignals(false);

        mLogImportBtn->setEnabled(id != Character::invalidId);
        mArchiveModel.setCharacterAndRange(id, QDateTime{fromDate}.toUTC(), QDateTime{tillDate}.addDays(1).toUTC());

        emit characterChanged(id);
    }

    ExternalOrderImporter::TypeLocationPairs MarketOrderWidget::getImportTarget() const
    {
        ExternalOrderImporter::TypeLocationPairs target;

        const auto importer = [&target](const auto &orders) {
            for (const auto &order : orders)
                target.emplace(std::make_pair(order->getTypeId(), order->getLocationId()));
        };

        importer(mOrderProvider.getBuyOrders(getCharacterId()));
        importer(mOrderProvider.getSellOrders(getCharacterId()));

        return target;
    }
}
