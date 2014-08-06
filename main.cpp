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
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "MarketLogExternalOrderImporterThread.h"
#include "EveMarketDataExternalOrderImporter.h"
#include "MarketLogExternalOrderImporter.h"
#include "MarketOrderFilterProxyModel.h"
#include "ExternalOrderImporterNames.h"
#include "EvernusApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    try
    {
        QCoreApplication::setApplicationName("Evernus");
        QCoreApplication::setApplicationVersion("0.3");
        QCoreApplication::setOrganizationDomain("evernus.com");
        QCoreApplication::setOrganizationName("evernus.com");

        qRegisterMetaType<Evernus::MarketLogExternalOrderImporterThread::ExternalOrderList>("ExternalOrderList");
        qRegisterMetaType<Evernus::Character::IdType>("Character::IdType");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::StatusFilters>("MarketOrderFilterProxyModel::StatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::StatusFilters>("StatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::PriceStatusFilters>("MarketOrderFilterProxyModel::PriceStatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::PriceStatusFilters>("PriceStatusFilters");
        qRegisterMetaType<std::vector<Evernus::ExternalOrder>>("std::vector<ExternalOrder>");
        qRegisterMetaType<Evernus::ExternalOrderImporter::TypeLocationPairs>("ExternalOrderImporter::TypeLocationPairs");

        Evernus::EvernusApplication app{argc, argv};

        app.registerImporter(Evernus::ExternalOrderImporterNames::webImporter,
                             std::make_unique<Evernus::EveMarketDataExternalOrderImporter>());
        app.registerImporter(Evernus::ExternalOrderImporterNames::logImporter,
                             std::make_unique<Evernus::MarketLogExternalOrderImporter>());

        try
        {
            Evernus::MainWindow mainWnd{app.getCharacterRepository(),
                                        app.getKeyRepository(),
                                        app.getAssetValueSnapshotRepository(),
                                        app.getWalletSnapshotRepository(),
                                        app.getMarketOrderValueSnapshotRepository(),
                                        app.getWalletJournalEntryRepository(),
                                        app.getWalletTransactionRepository(),
                                        app.getMarketOrderRepository(),
                                        app.getItemCostRepository(),
                                        app.getFilterTextRepository(),
                                        app,
                                        app,
                                        app,
                                        app,
                                        app};

            app.connect(&mainWnd, SIGNAL(refreshCharacters()), SLOT(refreshCharacters()));
            app.connect(&mainWnd, SIGNAL(refreshConquerableStations()), SLOT(refreshConquerableStations()));
            app.connect(&mainWnd, SIGNAL(importCharacter(Character::IdType)), SLOT(refreshCharacter(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importAssets(Character::IdType)), SLOT(refreshAssets(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importWalletJournal(Character::IdType)), SLOT(refreshWalletJournal(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importWalletTransactions(Character::IdType)), SLOT(refreshWalletTransactions(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importMarketOrdersFromAPI(Character::IdType)), SLOT(refreshMarketOrdersFromAPI(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importMarketOrdersFromLogs(Character::IdType)), SLOT(refreshMarketOrdersFromLogs(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &)), SLOT(refreshExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(importExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &)), SLOT(refreshExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(preferencesChanged()), SLOT(handleNewPreferences()));
            app.connect(&mainWnd, SIGNAL(importFromMentat()), SLOT(importFromMentat()));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, const QString &)), SLOT(addNewTaskInfo(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, uint, const QString &)), SIGNAL(newSubTaskInfoAdded(uint, uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskInfoChanged(uint, const QString &)), SIGNAL(taskInfoChanged(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskEnded(uint, const QString &)), SIGNAL(taskEnded(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(apiError(const QString &)), SLOT(showError(const QString &)));
            mainWnd.connect(&app, SIGNAL(conquerableStationsChanged()), SIGNAL(conquerableStationsChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SIGNAL(charactersChanged()));
            mainWnd.connect(&app, SIGNAL(assetsChanged()), SIGNAL(assetsChanged()));
            mainWnd.connect(&app, SIGNAL(externalOrdersChanged()), SIGNAL(externalOrdersChanged()));
            mainWnd.connect(&app, SIGNAL(walletJournalChanged()), SIGNAL(walletJournalChanged()));
            mainWnd.connect(&app, SIGNAL(walletTransactionsChanged()), SIGNAL(walletTransactionsChanged()));
            mainWnd.connect(&app, SIGNAL(marketOrdersChanged()), SIGNAL(marketOrdersChanged()));
            mainWnd.connect(&app, SIGNAL(itemCostsChanged()), SIGNAL(itemCostsChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SLOT(updateIskData()));
            mainWnd.connect(&app, SIGNAL(openMarginTool()), SLOT(showMarginTool()));
            mainWnd.showAsSaved();

            return app.exec();
        }
        catch (const std::exception &e)
        {
            qCritical() << e.what();
            QMessageBox::critical(nullptr, QCoreApplication::translate("main", "Error"), e.what());
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        qCritical() << e.what();

        QApplication tempApp{argc, argv};
        QMessageBox::critical(nullptr, QCoreApplication::translate("main", "Initialization error"), e.what());
        return 1;
    }

    return 0;
}
