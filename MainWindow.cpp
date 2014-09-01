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
#include <QNetworkInterface>
#include <QDesktopServices>
#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QClipboard>
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
#include "MarketBrowserWidget.h"
#include "RepositoryProvider.h"
#include "ActiveTasksDialog.h"
#include "PreferencesDialog.h"
#include "MarketOrderWidget.h"
#include "ItemHistoryWidget.h"
#include "MarginToolDialog.h"
#include "StatisticsWidget.h"
#include "CharacterWidget.h"
#include "ContractWidget.h"
#include "ItemCostWidget.h"
#include "ImportSettings.h"
#include "KeyRepository.h"
#include "MenuBarWidget.h"
#include "PriceSettings.h"
#include "HttpSettings.h"
#include "AssetsWidget.h"
#include "AboutDialog.h"
#include "IGBSettings.h"
#include "UISettings.h"
#include "Updater.h"

#include "MainWindow.h"

namespace Evernus
{
    const QString MainWindow::settingsMaximizedKey = "mainWindow/maximized";
    const QString MainWindow::settingsPosKey = "mainWindow/pos";
    const QString MainWindow::settingsSizeKey = "mainWindow/size";

    MainWindow::MainWindow(const RepositoryProvider &repositoryProvider,
                           MarketOrderProvider &orderProvider,
                           MarketOrderProvider &corpOrderProvider,
                           const AssetProvider &assetProvider,
                           const ContractProvider &contractProvider,
                           const ContractProvider &corpContractProvider,
                           EveDataProvider &eveDataProvider,
                           const CacheTimerProvider &cacheTimerProvider,
                           ItemCostProvider &itemCostProvider,
                           QWidget *parent,
                           Qt::WindowFlags flags)
        : QMainWindow{parent, flags}
        , mRepositoryProvider{repositoryProvider}
        , mOrderProvider{orderProvider}
        , mCorpOrderProvider{corpOrderProvider}
        , mAssetProvider{assetProvider}
        , mContractProvider{contractProvider}
        , mCorpContractProvider{corpContractProvider}
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
        connect(&mAutoImportTimer, &QTimer::timeout, this, &MainWindow::refreshAll);

        setWindowIcon(QIcon{":/images/main-icon.png"});

        if (!mRepositoryProvider.getCharacterRepository().hasCharacters())
            QMetaObject::invokeMethod(this, "showCharacterManagement", Qt::QueuedConnection);

        setUpAutoImportTimer();
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
            mCharacterManagerDialog = new CharacterManagerDialog{mRepositoryProvider.getCharacterRepository(),
                                                                 mRepositoryProvider.getKeyRepository(),
                                                                 mRepositoryProvider.getCorpKeyRepository(),
                                                                 this};
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

        setUpAutoImportTimer();

        emit preferencesChanged();
    }

    void MainWindow::showMarginTool()
    {
        showMinimized();

        if (mMarginToolDialog.isNull())
        {
            mMarginToolDialog = new MarginToolDialog{mRepositoryProvider.getCharacterRepository(),
                                                     mItemCostProvider,
                                                     mEveDataProvider};
            mMarginToolDialog->setCharacter(mCurrentCharacterId);
            connect(mMenuWidget, &MenuBarWidget::currentCharacterChanged, mMarginToolDialog, &MarginToolDialog::setCharacter);
        }

        mMarginToolDialog->showNormal();
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

    void MainWindow::openHelp()
    {
        QDesktopServices::openUrl(QUrl{"http://evernus.com/help"});
    }

    void MainWindow::checkForUpdates()
    {
        Updater::getInstance().checkForUpdates(false);
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
            const auto character = mRepositoryProvider.getCharacterRepository().find(mCurrentCharacterId);
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

    void MainWindow::refreshContracts()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            QSettings settings;
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpContracts(mCurrentCharacterId);

            emit importContracts(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshWalletJournal()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            QSettings settings;
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpWalletJournal(mCurrentCharacterId);

            emit importWalletJournal(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshWalletTransactions()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            QSettings settings;
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpWalletTransactions(mCurrentCharacterId);

            emit importWalletTransactions(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshMarketOrdersFromAPI()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            QSettings settings;
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpMarketOrdersFromAPI(mCurrentCharacterId);

            emit importMarketOrdersFromAPI(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshMarketOrdersFromLogs()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            QSettings settings;
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpMarketOrdersFromLogs(mCurrentCharacterId);

            emit importMarketOrdersFromLogs(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshAll()
    {
        emit refreshCharacters();
        emit refreshConquerableStations();

        refreshWalletJournal();
        refreshMarketOrdersFromAPI();

        QSettings settings;

        if (!settings.value(PriceSettings::autoAddCustomItemCostKey, PriceSettings::autoAddCustomItemCostDefault).toBool())
            refreshWalletTransactions();

        if (settings.value(ImportSettings::importAssetsKey, ImportSettings::importAssetsDefault).toBool())
            refreshAssets();
        if (settings.value(ImportSettings::importContractsKey, ImportSettings::importContractsDefault).toBool())
            refreshContracts();
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

    void MainWindow::copyIGBLink()
    {
        QSettings settings;
        QApplication::clipboard()->setText(QString{"http://localhost:%1"}
            .arg(settings.value(IGBSettings::portKey, IGBSettings::portDefault).value<quint16>()));

        QMessageBox::information(this, tr("Evernus"), tr("IGB link was copied to the clipboard."));
    }

    void MainWindow::copyHTTPLink()
    {
        const auto addresses = QNetworkInterface::allAddresses();
        QString curAddress = "localhost";

        for (const auto &address : addresses)
        {
            if (!address.isLoopback() && address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                curAddress = address.toString();
                break;
            }
        }

        QSettings settings;
        QApplication::clipboard()->setText(QString{"http://%1:%2"}
            .arg(curAddress)
            .arg(settings.value(HttpSettings::portKey, HttpSettings::portDefault).value<quint16>()));

        QMessageBox::information(this, tr("Evernus"), tr("HTTP link was copied to the clipboard."));
    }

    void MainWindow::showMarketBrowser()
    {
        mMainTabs->setCurrentIndex(mMarketBrowserTab);
    }

    void MainWindow::changeEvent(QEvent *event)
    {
        QSettings settings;
        if ((event->type() == QEvent::WindowStateChange) &&
            settings.value(UISettings::minimizeToTrayKey, UISettings::minimizeToTrayDefault).toBool())
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
        fileMenu->addAction(QIcon{":/images/user.png"}, tr("&Manage characters..."), this, SLOT(showCharacterManagement()));
#ifdef Q_OS_OSX
        fileMenu->addAction(QIcon{":/images/wrench.png"}, tr("&Preferences..."), this, SLOT(showPreferences()), QKeySequence::Preferences)->setMenuRole(QAction::PreferencesRole);
#else
        fileMenu->addAction(QIcon{":/images/wrench.png"}, tr("&Preferences..."), this, SLOT(showPreferences()), Qt::CTRL + Qt::Key_O)->setMenuRole(QAction::PreferencesRole);
#endif
        fileMenu->addSeparator();
        fileMenu->addAction(tr("Import EVE Mentat order history..."), this, SIGNAL(importFromMentat()));
        fileMenu->addSeparator();
        fileMenu->addAction(tr("E&xit"), this, SLOT(close()), QKeySequence::Quit)->setMenuRole(QAction::QuitRole);

        auto toolsMenu = bar->addMenu(tr("&Tools"));
        toolsMenu->addAction(tr("Import conquerable stations"), this, SIGNAL(refreshConquerableStations()));
        toolsMenu->addAction(QIcon{":/images/report.png"}, tr("Ma&rgin tool..."), this, SLOT(showMarginTool()), Qt::CTRL + Qt::Key_M);
        toolsMenu->addSeparator();
        toolsMenu->addAction(tr("Copy HTTP link"), this, SLOT(copyHTTPLink()));
        toolsMenu->addAction(tr("Copy IGB link"), this, SLOT(copyIGBLink()));

        auto helpMenu = bar->addMenu(tr("&Help"));
        helpMenu->addAction(QIcon{":/images/help.png"}, tr("&Online help..."), this, SLOT(openHelp()));
        helpMenu->addAction(tr("Check for &updates"), this, SLOT(checkForUpdates()));
        helpMenu->addSeparator();
        helpMenu->addAction(tr("&About..."), this, SLOT(showAbout()))->setMenuRole(QAction::AboutRole);

        mMenuWidget = new MenuBarWidget{mRepositoryProvider.getCharacterRepository(), this};
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

        auto charTab = new CharacterWidget{mRepositoryProvider.getCharacterRepository(),
                                           mRepositoryProvider.getMarketOrderRepository(),
                                           mRepositoryProvider.getCorpKeyRepository(),
                                           mCacheTimerProvider,
                                           this};
        addTab(charTab, tr("Character"));
        connect(charTab, &CharacterWidget::importFromAPI, this, &MainWindow::importCharacter);
        connect(charTab, &CharacterWidget::importAll, this, &MainWindow::refreshAll);
        connect(this, &MainWindow::charactersChanged, charTab, &CharacterWidget::updateData);
        connect(this, &MainWindow::assetsChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::walletJournalChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::walletTransactionsChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::marketOrdersChanged, charTab, &CharacterWidget::updateMarketData);

        auto statsTab = new StatisticsWidget{mRepositoryProvider.getAssetValueSnapshotRepository(),
                                             mRepositoryProvider.getWalletSnapshotRepository(),
                                             mRepositoryProvider.getCorpWalletSnapshotRepository(),
                                             mRepositoryProvider.getMarketOrderValueSnapshotRepository(),
                                             mRepositoryProvider.getWalletJournalEntryRepository(),
                                             mRepositoryProvider.getWalletTransactionRepository(),
                                             mRepositoryProvider.getCorpWalletJournalEntryRepository(),
                                             mRepositoryProvider.getCorpWalletTransactionRepository(),
                                             mRepositoryProvider.getMarketOrderRepository(),
                                             mRepositoryProvider.getOrderScriptRepository(),
                                             mRepositoryProvider.getCharacterRepository(),
                                             mEveDataProvider,
                                             this};
        addTab(statsTab, tr("Statistics"));
        connect(this, &MainWindow::charactersChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::externalOrdersChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::assetsChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletJournalChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletJournalChanged, statsTab, &StatisticsWidget::updateJournalData);
        connect(this, &MainWindow::corpWalletJournalChanged, statsTab, &StatisticsWidget::updateJournalData);
        connect(this, &MainWindow::corpWalletJournalChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletTransactionsChanged, statsTab, &StatisticsWidget::updateTransactionData);
        connect(this, &MainWindow::corpWalletTransactionsChanged, statsTab, &StatisticsWidget::updateTransactionData);
        connect(this, &MainWindow::preferencesChanged, statsTab, &StatisticsWidget::handleNewPreferences);

        auto assetsTab = new AssetsWidget{mAssetProvider,
                                          mEveDataProvider,
                                          mCacheTimerProvider,
                                          mRepositoryProvider.getFilterTextRepository(),
                                          this};
        addTab(assetsTab, tr("Assets"));
        connect(assetsTab, &AssetsWidget::importFromAPI, this, &MainWindow::importAssets);
        connect(assetsTab, &AssetsWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(assetsTab, &AssetsWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(assetsTab, &AssetsWidget::importPricesFromCache, this, &MainWindow::importExternalOrdersFromCache);
        connect(this, &MainWindow::conquerableStationsChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::assetsChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::externalOrdersChangedWithMarketOrders, assetsTab, &AssetsWidget::updateData);

        auto orderTab = new MarketOrderWidget{mOrderProvider,
                                              mCorpOrderProvider,
                                              mCacheTimerProvider,
                                              mEveDataProvider,
                                              mItemCostProvider,
                                              mRepositoryProvider.getWalletTransactionRepository(),
                                              mRepositoryProvider.getCharacterRepository(),
                                              mRepositoryProvider.getFilterTextRepository(),
                                              mRepositoryProvider.getExternalOrderRepository(),
                                              false,
                                              this};
        addTab(orderTab, tr("Character orders"));
        connect(orderTab, &MarketOrderWidget::importFromAPI, this, &MainWindow::importMarketOrdersFromAPI);
        connect(orderTab, &MarketOrderWidget::importFromLogs, this, &MainWindow::importMarketOrdersFromLogs);
        connect(orderTab, &MarketOrderWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(orderTab, &MarketOrderWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(orderTab, &MarketOrderWidget::importPricesFromCache, this, &MainWindow::importExternalOrdersFromCache);
        connect(orderTab, &MarketOrderWidget::openMarginTool, this, &MainWindow::showMarginTool);
        connect(orderTab, &MarketOrderWidget::showExternalOrders, this, &MainWindow::showMarketBrowser);
        connect(orderTab, &MarketOrderWidget::showInEve, this, &MainWindow::showInEve);
        connect(this, &MainWindow::marketOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::corpMarketOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::conquerableStationsChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::itemCostsChanged, orderTab, &MarketOrderWidget::updateData);

        auto journalTab = new WalletJournalWidget{mRepositoryProvider.getWalletJournalEntryRepository(),
                                                  mRepositoryProvider.getCharacterRepository(),
                                                  mRepositoryProvider.getFilterTextRepository(),
                                                  mCacheTimerProvider,
                                                  mEveDataProvider,
                                                  false,
                                                  this};
        addTab(journalTab, tr("Character journal"));
        connect(journalTab, &WalletJournalWidget::importFromAPI, this, &MainWindow::importWalletJournal);
        connect(this, &MainWindow::walletJournalChanged, journalTab, &WalletJournalWidget::updateData);

        auto transactionsTab = new WalletTransactionsWidget{mRepositoryProvider.getWalletTransactionRepository(),
                                                            mRepositoryProvider.getCharacterRepository(),
                                                            mRepositoryProvider.getFilterTextRepository(),
                                                            mCacheTimerProvider,
                                                            mEveDataProvider,
                                                            mItemCostProvider,
                                                            false,
                                                            this};
        addTab(transactionsTab, tr("Character transactions"));
        connect(transactionsTab, &WalletTransactionsWidget::importFromAPI, this, &MainWindow::importWalletTransactions);
        connect(this, &MainWindow::walletTransactionsChanged, transactionsTab, &WalletTransactionsWidget::updateData);

        auto contractsTab = new ContractWidget{mCacheTimerProvider,
                                               mEveDataProvider,
                                               mContractProvider,
                                               mRepositoryProvider.getFilterTextRepository(),
                                               mRepositoryProvider.getCharacterRepository(),
                                               false,
                                               this};
        addTab(contractsTab, tr("Character contracts"));
        connect(contractsTab, &ContractWidget::importFromAPI, this, &MainWindow::importContracts);
        connect(this, &MainWindow::contractsChanged, contractsTab, &ContractWidget::updateData);

        auto corpOrderTab = new MarketOrderWidget{mCorpOrderProvider,
                                                  mCorpOrderProvider,
                                                  mCacheTimerProvider,
                                                  mEveDataProvider,
                                                  mItemCostProvider,
                                                  mRepositoryProvider.getCorpWalletTransactionRepository(),
                                                  mRepositoryProvider.getCharacterRepository(),
                                                  mRepositoryProvider.getFilterTextRepository(),
                                                  mRepositoryProvider.getExternalOrderRepository(),
                                                  true,
                                                  this};
        addTab(corpOrderTab, tr("Corporation orders"));
        connect(corpOrderTab, &MarketOrderWidget::importFromAPI, this, &MainWindow::importCorpMarketOrdersFromAPI);
        connect(corpOrderTab, &MarketOrderWidget::importFromLogs, this, &MainWindow::importCorpMarketOrdersFromLogs);
        connect(corpOrderTab, &MarketOrderWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(corpOrderTab, &MarketOrderWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(corpOrderTab, &MarketOrderWidget::importPricesFromCache, this, &MainWindow::importExternalOrdersFromCache);
        connect(corpOrderTab, &MarketOrderWidget::openMarginTool, this, &MainWindow::showMarginTool);
        connect(corpOrderTab, &MarketOrderWidget::showExternalOrders, this, &MainWindow::showMarketBrowser);
        connect(corpOrderTab, &MarketOrderWidget::showInEve, this, &MainWindow::showInEve);
        connect(this, &MainWindow::corpMarketOrdersChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::conquerableStationsChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::itemCostsChanged, corpOrderTab, &MarketOrderWidget::updateData);

        auto corpJournalTab = new WalletJournalWidget{mRepositoryProvider.getCorpWalletJournalEntryRepository(),
                                                      mRepositoryProvider.getCharacterRepository(),
                                                      mRepositoryProvider.getFilterTextRepository(),
                                                      mCacheTimerProvider,
                                                      mEveDataProvider,
                                                      true,
                                                      this};
        addTab(corpJournalTab, tr("Corporation journal"));
        connect(corpJournalTab, &WalletJournalWidget::importFromAPI, this, &MainWindow::importCorpWalletJournal);
        connect(this, &MainWindow::corpWalletJournalChanged, corpJournalTab, &WalletJournalWidget::updateData);

        auto corpTransactionsTab = new WalletTransactionsWidget{mRepositoryProvider.getCorpWalletTransactionRepository(),
                                                                mRepositoryProvider.getCharacterRepository(),
                                                                mRepositoryProvider.getFilterTextRepository(),
                                                                mCacheTimerProvider,
                                                                mEveDataProvider,
                                                                mItemCostProvider,
                                                                true,
                                                                this};
        addTab(corpTransactionsTab, tr("Corporation transactions"));
        connect(corpTransactionsTab, &WalletTransactionsWidget::importFromAPI, this, &MainWindow::importCorpWalletTransactions);
        connect(this, &MainWindow::corpWalletTransactionsChanged, corpTransactionsTab, &WalletTransactionsWidget::updateData);

        auto corpContractsTab = new ContractWidget{mCacheTimerProvider,
                                                   mEveDataProvider,
                                                   mCorpContractProvider,
                                                   mRepositoryProvider.getFilterTextRepository(),
                                                   mRepositoryProvider.getCharacterRepository(),
                                                   true,
                                                   this};
        addTab(corpContractsTab, tr("Corporation contracts"));
        connect(corpContractsTab, &ContractWidget::importFromAPI, this, &MainWindow::importCorpContracts);
        connect(this, &MainWindow::corpContractsChanged, corpContractsTab, &ContractWidget::updateData);

        auto itemHistoryTab = new ItemHistoryWidget{mRepositoryProvider.getWalletTransactionRepository(),
                                                    mRepositoryProvider.getCorpWalletTransactionRepository(),
                                                    mEveDataProvider,
                                                    this};
        connect(this, &MainWindow::walletTransactionsChanged, itemHistoryTab, &ItemHistoryWidget::updateData);
        connect(this, &MainWindow::corpWalletTransactionsChanged, itemHistoryTab, &ItemHistoryWidget::updateData);
        connect(this, &MainWindow::preferencesChanged, itemHistoryTab, &ItemHistoryWidget::handleNewPreferences);
        addTab(itemHistoryTab, tr("Item history"));

        auto marketBrowserTab = new MarketBrowserWidget{mRepositoryProvider.getExternalOrderRepository(),
                                                        mRepositoryProvider.getMarketOrderRepository(),
                                                        mRepositoryProvider.getCorpMarketOrderRepository(),
                                                        mRepositoryProvider.getCharacterRepository(),
                                                        mRepositoryProvider.getFavoriteItemRepository(),
                                                        mRepositoryProvider.getLocationBookmarkRepository(),
                                                        mOrderProvider,
                                                        mCorpOrderProvider,
                                                        mEveDataProvider,
                                                        mItemCostProvider,
                                                        this};
        mMarketBrowserTab = addTab(marketBrowserTab, tr("Market browser"));
        connect(marketBrowserTab, &MarketBrowserWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(marketBrowserTab, &MarketBrowserWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(marketBrowserTab, &MarketBrowserWidget::importPricesFromCache, this, &MainWindow::importExternalOrdersFromCache);
        connect(corpOrderTab, &MarketOrderWidget::showExternalOrders, marketBrowserTab, &MarketBrowserWidget::showOrdersForType);
        connect(orderTab, &MarketOrderWidget::showExternalOrders, marketBrowserTab, &MarketBrowserWidget::showOrdersForType);
        connect(this, &MainWindow::marketOrdersChanged, marketBrowserTab, &MarketBrowserWidget::fillOrderItemNames);
        connect(this, &MainWindow::corpMarketOrdersChanged, marketBrowserTab, &MarketBrowserWidget::fillOrderItemNames);
        connect(this, &MainWindow::externalOrdersChanged, marketBrowserTab, &MarketBrowserWidget::updateData);

        auto itemCostTab = new ItemCostWidget{mItemCostProvider, mEveDataProvider, this};
        addTab(itemCostTab, tr("Item costs"));
        connect(this, &MainWindow::itemCostsChanged, itemCostTab, &ItemCostWidget::updateData);
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

    int MainWindow::addTab(QWidget *widget, const QString &label)
    {
        const auto index = mMainTabs->addTab(createMainViewTab(widget), label);

        mTabWidgets[index] = widget;
        return index;
    }

    void MainWindow::setUpAutoImportTimer()
    {
        QSettings settings;

        mAutoImportTimer.setInterval(
            settings.value(ImportSettings::autoImportTimeKey, ImportSettings::autoImportTimerDefault).toInt() * 1000 * 60);

        if (settings.value(ImportSettings::autoImportEnabledKey, ImportSettings::autoImportEnabledDefault).toBool())
            mAutoImportTimer.start();
        else
            mAutoImportTimer.stop();
    }
}
