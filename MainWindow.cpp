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
#include <QCoreApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QSettings>
#include <QMenuBar>
#include <QLabel>
#include <QDebug>

#ifdef Q_OS_WIN
#   include <QWinTaskbarButton>
#endif

#include "WalletTransactionsWidget.h"
#include "CharacterManagerDialog.h"
#include "WalletJournalWidget.h"
#include "CharacterRepository.h"
#include "ActiveTasksDialog.h"
#include "PreferencesDialog.h"
#include "MarketOrderWidget.h"
#include "MarginToolDialog.h"
#include "StatisticsWidget.h"
#include "CharacterWidget.h"
#include "ItemCostWidget.h"
#include "ImportSettings.h"
#include "MenuBarWidget.h"
#include "AssetsWidget.h"
#include "AboutDialog.h"
#include "UISettings.h"

#include "MainWindow.h"

namespace Evernus
{
    const QString MainWindow::settingsMaximizedKey = "mainWindow/maximized";
    const QString MainWindow::settingsPosKey = "mainWindow/pos";
    const QString MainWindow::settingsSizeKey = "mainWindow/size";

    MainWindow::MainWindow(const CharacterRepository &characterRepository,
                           const Repository<Key> &keyRepository,
                           const AssetValueSnapshotRepository &assetSnapshotRepo,
                           const WalletSnapshotRepository &walletSnapshotRepo,
                           const MarketOrderValueSnapshotRepository &marketOrderSnapshotRepo,
                           const WalletJournalEntryRepository &walletJournalRepo,
                           const WalletTransactionRepository &walletTransactionRepo,
                           const MarketOrderRepository &orderRepo,
                           const ItemCostRepository &itemCostRepo,
                           const FilterTextRepository &filterRepo,
                           const MarketOrderProvider &orderProvider,
                           const AssetProvider &assetProvider,
                           const EveDataProvider &eveDataProvider,
                           const CacheTimerProvider &cacheTimerProvider,
                           const ItemCostProvider &itemCostProvider,
                           QWidget *parent,
                           Qt::WindowFlags flags)
        : QMainWindow{parent, flags}
        , mCharacterRepository{characterRepository}
        , mKeyRepository{keyRepository}
        , mAssetSnapshotRepository{assetSnapshotRepo}
        , mWalletSnapshotRepository{walletSnapshotRepo}
        , mMarketOrderSnapshotRepository{marketOrderSnapshotRepo}
        , mWalletJournalRepository{walletJournalRepo}
        , mWalletTransactionRepository{walletTransactionRepo}
        , mMarketOrderRepository{orderRepo}
        , mItemCostRepository{itemCostRepo}
        , mFilterRepository{filterRepo}
        , mOrderProvider{orderProvider}        
        , mAssetProvider{assetProvider}
        , mItemCostProvider{itemCostProvider}
        , mEveDataProvider{eveDataProvider}
        , mCacheTimerProvider{cacheTimerProvider}
        , mTrayIcon{new QSystemTrayIcon{QIcon{":/images/main-icon.png"}, this}}
    {
        readSettings();
        createMenu();
        createMainView();
        createStatusBar();

        connect(mTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::activateTrayIcon);

        setWindowIcon(QIcon{":/images/main-icon.png"});
    }

    void MainWindow::showAsSaved()
    {
        if (mShowMaximized)
            showMaximized();
        else
            show();
    }

    void MainWindow::showCharacterManagement()
    {
        if (mCharacterManagerDialog == nullptr)
        {
            mCharacterManagerDialog = new CharacterManagerDialog{mCharacterRepository, mKeyRepository, this};
            connect(mCharacterManagerDialog, &CharacterManagerDialog::refreshCharacters, this, &MainWindow::refreshCharacters);
            connect(this, &MainWindow::charactersChanged, mCharacterManagerDialog, &CharacterManagerDialog::updateCharacters);
            connect(mCharacterManagerDialog, &CharacterManagerDialog::charactersChanged,
                    mMenuWidget, &MenuBarWidget::refreshCharacters);
        }

        mCharacterManagerDialog->exec();
    }

    void MainWindow::showPreferences()
    {
        PreferencesDialog dlg{this};
        dlg.exec();
    }

    void MainWindow::showMarginTool()
    {
        if (mMarginToolDialog.isNull())
        {
            mMarginToolDialog = new MarginToolDialog{mCharacterRepository, mItemCostProvider, mEveDataProvider};
            mMarginToolDialog->setCharacter(mCurrentCharacterId);
            connect(mMenuWidget, &MenuBarWidget::currentCharacterChanged, mMarginToolDialog, &MarginToolDialog::setCharacter);
            connect(mMarginToolDialog, &MarginToolDialog::parsedData, this, &MainWindow::marginToolParsedData);
            connect(mMarginToolDialog, &MarginToolDialog::hidden, this, [this]() {
                emit marginToolHidden(mCurrentCharacterId);
            });
        }

        showMinimized();

        mMarginToolDialog->activateWindow();
    }

    void MainWindow::showAbout()
    {
        AboutDialog dlg{this};
        dlg.exec();
    }

    void MainWindow::showError(const QString &info)
    {
        qCritical() << info;
        QMessageBox::warning(this, tr("Error"), info);
    }

    void MainWindow::addNewTaskInfo(uint taskId, const QString &description)
    {
        if (mActiveTasksDialog == nullptr)
        {
#ifdef Q_OS_WIN
            if (mTaskbarButton == nullptr)
            {
                mTaskbarButton = new QWinTaskbarButton{this};
                mTaskbarButton->setWindow(windowHandle());
            }

            mActiveTasksDialog = new ActiveTasksDialog{*mTaskbarButton, this};
#else
            mActiveTasksDialog = new ActiveTasksDialog{this};
#endif
            connect(this, &MainWindow::newTaskInfoAdded, mActiveTasksDialog, &ActiveTasksDialog::addNewTaskInfo);
            connect(this, &MainWindow::newSubTaskInfoAdded, mActiveTasksDialog, &ActiveTasksDialog::addNewSubTaskInfo);
            connect(this, &MainWindow::taskInfoChanged, mActiveTasksDialog, &ActiveTasksDialog::setTaskInfo);
            connect(this, &MainWindow::taskEnded, mActiveTasksDialog, &ActiveTasksDialog::endTask);
            mActiveTasksDialog->setModal(true);
            mActiveTasksDialog->show();
        }
        else
        {
            mActiveTasksDialog->show();
            mActiveTasksDialog->activateWindow();
        }

        emit newTaskInfoAdded(taskId, description);
    }

    void MainWindow::updateIskData()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            const auto character = mCharacterRepository.find(mCurrentCharacterId);
            mStatusWalletLabel->setText(tr("Wallet: <strong>%1</strong>")
                .arg(character->getISKPresentation()));
        }
        else
        {
            mStatusWalletLabel->setText(QString{});
        }
    }

    void MainWindow::setCharacter(Character::IdType id)
    {
        mCurrentCharacterId = id;
        updateIskData();
        updateCurrentTab(mMainTabs->currentIndex());
    }

    void MainWindow::refreshAssets()
    {
        if (mCurrentCharacterId != Character::invalidId)
            emit importAssets(mCurrentCharacterId);
    }

    void MainWindow::refreshWalletJournal()
    {
        if (mCurrentCharacterId != Character::invalidId)
            emit importWalletJournal(mCurrentCharacterId);
    }

    void MainWindow::refreshWalletTransactions()
    {
        if (mCurrentCharacterId != Character::invalidId)
            emit importWalletTransactions(mCurrentCharacterId);
    }

    void MainWindow::refreshMarketOrdersFromAPI()
    {
        if (mCurrentCharacterId != Character::invalidId)
            emit importMarketOrdersFromAPI(mCurrentCharacterId);
    }

    void MainWindow::refreshMarketOrdersFromLogs()
    {
        if (mCurrentCharacterId != Character::invalidId)
            emit importMarketOrdersFromLogs(mCurrentCharacterId);
    }

    void MainWindow::refreshAll()
    {
        emit refreshCharacters();
        emit refreshConquerableStations();

        refreshWalletJournal();
        refreshWalletTransactions();
        refreshMarketOrdersFromAPI();

        QSettings settings;
        if (settings.value(ImportSettings::importAssetsKey, true).toBool())
            refreshAssets();
    }

    void MainWindow::updateCurrentTab(int index)
    {
        auto widget = mMainTabs->widget(index);
        if (widget != nullptr)
        {
            if (mCurrentCharacterId != mTabCharacterIds[index])
            {
                mTabCharacterIds[index] = mCurrentCharacterId;
                QMetaObject::invokeMethod(mTabWidgets[index], "setCharacter", Qt::AutoConnection, Q_ARG(Character::IdType, mCurrentCharacterId));
            }
        }
    }

    void MainWindow::activateTrayIcon(QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        {
            mTrayIcon->hide();
            showMaximized();
            activateWindow();
        }
    }

    void MainWindow::changeEvent(QEvent *event)
    {
        QSettings settings;
        if ((event->type() == QEvent::WindowStateChange) &&
            settings.value(UISettings::minimizeToTrayKey, false).toBool())
        {
            if (isMinimized())
            {
                mTrayIcon->show();
                QMetaObject::invokeMethod(this, "hide", Qt::QueuedConnection);
            }
            else
            {
                mTrayIcon->hide();
            }
        }
    }

    void MainWindow::closeEvent(QCloseEvent *event)
    {
        if (!mMarginToolDialog.isNull())
            mMarginToolDialog->close();

        writeSettings();
        event->accept();
    }

    void MainWindow::readSettings()
    {
        QSettings settings;

        const auto pos = settings.value(settingsPosKey).toPoint();
        const auto size = settings.value(settingsSizeKey, QSize{600, 400}).toSize();

        resize(size);
        move(pos);

        mShowMaximized = settings.value(settingsMaximizedKey, false).toBool();
    }

    void MainWindow::writeSettings()
    {
        QSettings settings;
        settings.setValue(settingsPosKey, pos());
        settings.setValue(settingsSizeKey, size());
        settings.setValue(settingsMaximizedKey, isMaximized());
    }

    void MainWindow::createMenu()
    {
        auto bar = menuBar();

        auto fileMenu = bar->addMenu(tr("&File"));
        fileMenu->addAction(tr("&Manage characters..."), this, SLOT(showCharacterManagement()));
        fileMenu->addAction(tr("&Preferences..."), this, SLOT(showPreferences()), Qt::CTRL + Qt::Key_O);
        fileMenu->addSeparator();
        fileMenu->addAction(tr("E&xit"), this, SLOT(close()));

        auto toolsMenu = bar->addMenu(tr("&Tools"));
        toolsMenu->addAction(tr("Import conquerable stations"), this, SIGNAL(refreshConquerableStations()));
        toolsMenu->addAction(tr("Ma&rgin tool..."), this, SLOT(showMarginTool()), Qt::CTRL + Qt::Key_M);

        auto helpMenu = bar->addMenu(tr("&Help"));
        helpMenu->addAction(tr("&About..."), this, SLOT(showAbout()));

        mMenuWidget = new MenuBarWidget{mCharacterRepository, this};
        bar->setCornerWidget(mMenuWidget);
        connect(this, &MainWindow::charactersChanged, mMenuWidget, &MenuBarWidget::refreshCharacters);
        connect(mMenuWidget, &MenuBarWidget::currentCharacterChanged, this, &MainWindow::setCharacter);
        connect(mMenuWidget, &MenuBarWidget::importAll, this, &MainWindow::refreshAll);
    }

    void MainWindow::createMainView()
    {
        mMainTabs = new QTabWidget{this};
        setCentralWidget(mMainTabs);
        connect(mMainTabs, &QTabWidget::currentChanged, this, &MainWindow::updateCurrentTab);

        auto charTab = new CharacterWidget{mCharacterRepository, mMarketOrderRepository, mCacheTimerProvider, this};
        addTab(charTab, tr("Character"));
        connect(charTab, &CharacterWidget::importFromAPI, this, &MainWindow::importCharacter);
        connect(charTab, &CharacterWidget::importAll, this, &MainWindow::refreshAll);
        connect(this, &MainWindow::charactersChanged, charTab, &CharacterWidget::updateData);
        connect(this, &MainWindow::assetsChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::walletJournalChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::walletTransactionsChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::marketOrdersChanged, charTab, &CharacterWidget::updateMarketData);

        auto statsTab = new StatisticsWidget{mAssetSnapshotRepository,
                                             mWalletSnapshotRepository,
                                             mMarketOrderSnapshotRepository,
                                             mWalletJournalRepository,
                                             mWalletTransactionRepository,
                                             this};
        addTab(statsTab, tr("Statistics"));
        connect(this, &MainWindow::charactersChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::externalOrdersChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::assetsChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletJournalChanged, statsTab, &StatisticsWidget::updateJournalData);
        connect(this, &MainWindow::walletTransactionsChanged, statsTab, &StatisticsWidget::updateTransactionData);

        auto assetsTab = new AssetsWidget{mAssetProvider, mEveDataProvider, mCacheTimerProvider, mFilterRepository, this};
        addTab(assetsTab, tr("Assets"));
        connect(assetsTab, &AssetsWidget::importFromAPI, this, &MainWindow::importAssets);
        connect(assetsTab, &AssetsWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(assetsTab, &AssetsWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(this, &MainWindow::conquerableStationsChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::marginToolHidden, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::assetsChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, assetsTab, &AssetsWidget::updateData);

        auto orderTab = new MarketOrderWidget{mOrderProvider,
                                              mCacheTimerProvider,
                                              mEveDataProvider,
                                              mItemCostProvider,
                                              mWalletTransactionRepository,
                                              mFilterRepository,
                                              this};
        addTab(orderTab, tr("Orders"));
        connect(orderTab, &MarketOrderWidget::importFromAPI, this, &MainWindow::importMarketOrdersFromAPI);
        connect(orderTab, &MarketOrderWidget::importFromLogs, this, &MainWindow::importMarketOrdersFromLogs);
        connect(orderTab, &MarketOrderWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(orderTab, &MarketOrderWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(this, &MainWindow::marketOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::marginToolHidden, orderTab, &MarketOrderWidget::updateData);

        auto journalTab = new WalletJournalWidget{mWalletJournalRepository, mFilterRepository, mCacheTimerProvider, mEveDataProvider, this};
        addTab(journalTab, tr("Journal"));
        connect(journalTab, &WalletJournalWidget::importFromAPI, this, &MainWindow::importWalletJournal);
        connect(this, &MainWindow::walletJournalChanged, journalTab, &WalletJournalWidget::updateData);

        auto transactionsTab
            = new WalletTransactionsWidget{mWalletTransactionRepository, mFilterRepository, mCacheTimerProvider, mEveDataProvider, this};
        addTab(transactionsTab, tr("Transactions"));
        connect(transactionsTab, &WalletTransactionsWidget::importFromAPI, this, &MainWindow::importWalletTransactions);
        connect(this, &MainWindow::walletTransactionsChanged, transactionsTab, &WalletTransactionsWidget::updateData);

        auto itemCostTab = new ItemCostWidget{mItemCostRepository, mEveDataProvider, this};
        addTab(itemCostTab, tr("Item costs"));
        connect(itemCostTab, &ItemCostWidget::costsChanged, this, &MainWindow::itemCostsChanged);
    }

    void MainWindow::createStatusBar()
    {
        mStatusWalletLabel = new QLabel{this};
        statusBar()->addPermanentWidget(mStatusWalletLabel);
    }

    QWidget *MainWindow::createMainViewTab(QWidget *content)
    {
        auto scroll = new QScrollArea{this};
        scroll->setWidgetResizable(true);
        scroll->setWidget(content);
        scroll->setFrameStyle(QFrame::NoFrame);

        return scroll;
    }

    void MainWindow::addTab(QWidget *widget, const QString &label)
    {
        mTabWidgets[mMainTabs->addTab(createMainViewTab(widget), label)] = widget;
    }
}
