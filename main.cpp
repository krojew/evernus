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

#include "MarketLogItemPriceImporterThread.h"
#include "EveMarketDataItemPriceImporter.h"
#include "MarketLogItemPriceImporter.h"
#include "ItemPriceImporterNames.h"
#include "EvernusApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    try
    {
        QCoreApplication::setApplicationName("Evernus");
        QCoreApplication::setApplicationVersion("0.1 BETA");
        QCoreApplication::setOrganizationDomain("evernus.com");
        QCoreApplication::setOrganizationName("evernus.com");

        qRegisterMetaType<Evernus::MarketLogItemPriceImporterThread::ItemPriceList>("ItemPriceList");
        qRegisterMetaType<Evernus::Character::IdType>("Character::IdType");

        Evernus::EvernusApplication app{argc, argv};

        app.registerImporter(Evernus::ItemPriceImporterNames::webImporter,
                             std::make_unique<Evernus::EveMarketDataItemPriceImporter>());
        app.registerImporter(Evernus::ItemPriceImporterNames::logImporter,
                             std::make_unique<Evernus::MarketLogItemPriceImporter>());

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
            app.connect(&mainWnd, SIGNAL(importMarketOrders(Character::IdType)), SLOT(refreshMarketOrders(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &)), SLOT(refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(importItemPricesFromFile(const ItemPriceImporter::TypeLocationPairs &)), SLOT(refreshItemPricesFromFile(const ItemPriceImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(marginToolHidden(Character::IdType)), SLOT(updateAssetsValue(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(itemCostsChanged()), SLOT(resetItemCostCache()));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, const QString &)), SLOT(addNewTaskInfo(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, uint, const QString &)), SIGNAL(newSubTaskInfoAdded(uint, uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskInfoChanged(uint, const QString &)), SIGNAL(taskInfoChanged(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskEnded(uint, const QString &)), SIGNAL(taskEnded(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(apiError(const QString &)), SLOT(showError(const QString &)));
            mainWnd.connect(&app, SIGNAL(conquerableStationsChanged()), SIGNAL(conquerableStationsChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SIGNAL(charactersChanged()));
            mainWnd.connect(&app, SIGNAL(assetsChanged()), SIGNAL(assetsChanged()));
            mainWnd.connect(&app, SIGNAL(itemPricesChanged()), SIGNAL(itemPricesChanged()));
            mainWnd.connect(&app, SIGNAL(walletJournalChanged()), SIGNAL(walletJournalChanged()));
            mainWnd.connect(&app, SIGNAL(walletTransactionsChanged()), SIGNAL(walletTransactionsChanged()));
            mainWnd.connect(&app, SIGNAL(marketOrdersChanged()), SIGNAL(marketOrdersChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SLOT(updateIskData()));
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
