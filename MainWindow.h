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

#include <unordered_map>

#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QPointer>
#include <QTimer>

#include "ExternalOrderImporter.h"
#include "Character.h"

class QSystemTrayIcon;
class QTabWidget;
class QLabel;

#ifdef Q_OS_WIN
class QWinTaskbarButton;
#endif

namespace Evernus
{
    class CharacterManagerDialog;
    class MarketOrderProvider;
    class WalletJournalEntry;
    class CacheTimerProvider;
    class RepositoryProvider;
    class ActiveTasksDialog;
    class MarginToolDialog;
    class ItemCostProvider;
    class ContractProvider;
    class EveDataProvider;
    class MenuBarWidget;
    class AssetProvider;
    class Key;

    class MainWindow
        : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(const RepositoryProvider &repositoryProvider,
                   MarketOrderProvider &orderProvider,
                   MarketOrderProvider &corpOrderProvider,
                   const AssetProvider &assetProvider,
                   const ContractProvider &contractProvider,
                   const ContractProvider &corpContractProvider,
                   EveDataProvider &eveDataProvider,
                   const CacheTimerProvider &cacheTimerProvider,
                   ItemCostProvider &itemCostProvider,
                   QWidget *parent = nullptr,
                   Qt::WindowFlags flags = 0);
        virtual ~MainWindow() = default;

        void showAsSaved();

    signals:
        void refreshCharacters();
        void refreshConquerableStations();

        void conquerableStationsChanged();
        void charactersChanged();
        void assetsChanged();
        void externalOrdersChanged();
        void externalOrdersChangedWithMarketOrders();
        void walletJournalChanged();
        void walletTransactionsChanged();
        void marketOrdersChanged();
        void contractsChanged();
        void corpWalletJournalChanged();
        void corpWalletTransactionsChanged();
        void corpMarketOrdersChanged();
        void corpContractsChanged();
        void itemCostsChanged();
        void itemVolumeChanged();

        void preferencesChanged();

        void newTaskInfoAdded(uint taskId, const QString &description);
        void newSubTaskInfoAdded(uint taskId, uint parentTask, const QString &description);
        void taskInfoChanged(uint taskId, const QString &text);
        void taskEnded(uint taskId, const QString &error);

        void importCharacter(Character::IdType id);
        void importAssets(Character::IdType id);
        void importContracts(Character::IdType id);
        void importWalletJournal(Character::IdType id);
        void importWalletTransactions(Character::IdType id);
        void importMarketOrdersFromAPI(Character::IdType id);
        void importMarketOrdersFromLogs(Character::IdType id);
        void importCorpContracts(Character::IdType id);
        void importCorpWalletJournal(Character::IdType id);
        void importCorpWalletTransactions(Character::IdType id);
        void importCorpMarketOrdersFromAPI(Character::IdType id);
        void importCorpMarketOrdersFromLogs(Character::IdType id);

        void importExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &target);
        void importExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &target);
        void importExternalOrdersFromCache(const ExternalOrderImporter::TypeLocationPairs &target);

        void importFromMentat();

        void showInEve(EveType::IdType id);

    public slots:
        void showCharacterManagement();
        void showPreferences();
        void showMarginTool();
        void showAbout();
        void showError(const QString &info);
        void openHelp();
        void checkForUpdates();
        void showColumnHelp();

        void addNewTaskInfo(uint taskId, const QString &description);

        void updateIskData();

        void setCharacter(Character::IdType id);

        void refreshAssets();
        void refreshContracts();
        void refreshWalletJournal();
        void refreshWalletTransactions();
        void refreshMarketOrdersFromAPI();
        void refreshMarketOrdersFromLogs();
        void refreshAll();

    private slots:
        void updateCurrentTab(int index);

        void activateTrayIcon(QSystemTrayIcon::ActivationReason reason);
        void copyIGBLink();
        void copyHTTPLink();

        void showMarketBrowser();

        void performSync();

    protected:
        virtual void changeEvent(QEvent *event) override;
        virtual void closeEvent(QCloseEvent *event) override;

    private:
        static const QString settingsMaximizedKey;
        static const QString settingsPosKey;
        static const QString settingsSizeKey;

        const RepositoryProvider &mRepositoryProvider;
        MarketOrderProvider &mOrderProvider, &mCorpOrderProvider;
        const AssetProvider &mAssetProvider;
        const ContractProvider &mContractProvider, &mCorpContractProvider;
        ItemCostProvider &mItemCostProvider;

        EveDataProvider &mEveDataProvider;

        const CacheTimerProvider &mCacheTimerProvider;

#ifdef Q_OS_WIN
        QWinTaskbarButton *mTaskbarButton = nullptr;
#endif

        MenuBarWidget *mMenuWidget = nullptr;
        QTabWidget *mMainTabs = nullptr;
        QSystemTrayIcon *mTrayIcon = nullptr;

        bool mShowMaximized = false;

        CharacterManagerDialog *mCharacterManagerDialog = nullptr;
        ActiveTasksDialog *mActiveTasksDialog = nullptr;
        QPointer<MarginToolDialog> mMarginToolDialog;

        QLabel *mStatusWalletLabel = nullptr;

        Character::IdType mCurrentCharacterId = Character::invalidId;

        std::unordered_map<int, Character::IdType> mTabCharacterIds;
        std::unordered_map<int, QWidget *> mTabWidgets;

        QMenu *mViewTabsMenu = nullptr;

        QTimer mAutoImportTimer;

        int mMarketBrowserTab = -1;

        void readSettings();
        void writeSettings();

        void createMenu();
        void createMainView();
        void createStatusBar();

        QWidget *createMainViewTab(QWidget *content);
        int addTab(QWidget *widget, const QString &label);

        void setUpAutoImportTimer();
    };
}
