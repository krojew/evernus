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
#include <QLocalSocket>
#include <QLocalServer>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QDir>

#include "MarketLogExternalOrderImporterThread.h"
#include "MarketLogExternalOrderImporter.h"
#include "MarketOrderFilterProxyModel.h"
#include "ExternalOrderImporterNames.h"
#include "CRESTExternalOrderImporter.h"
#include "CacheExternalOrderImporter.h"
#include "ContractFilterProxyModel.h"
#include "ExternalOrderModel.h"
#include "EvernusApplication.h"
#include "MainWindow.h"
#include "Version.h"

int main(int argc, char *argv[])
{
    try
    {
        QCoreApplication::setApplicationName("Evernus");
        QCoreApplication::setApplicationVersion(version::fullStr());
        QCoreApplication::setOrganizationDomain("evernus.com");
        QCoreApplication::setOrganizationName("evernus.com");

#ifdef Q_OS_WIN
        const auto serverName = QCoreApplication::applicationName() + ".socket";
#else
        const auto serverName = QDir::tempPath() + "/" + QCoreApplication::applicationName() + ".socket";
#endif

        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(500))
        {
            qDebug() << "Connected to" << socket.fullServerName();

            QApplication tempApp{argc, argv};
            QMessageBox::information(nullptr, QCoreApplication::translate("main", "Already running"), QCoreApplication::translate("main",
                "Evernus seems to be already running. If this is not the case, please remove '%1'.").arg(socket.fullServerName()));

            return 0;
        }

        QLocalServer server;
        server.connect(&server, &QLocalServer::newConnection, &QLocalServer::nextPendingConnection);
        if (!server.listen(serverName))
        {
            qDebug() << "Local server listen failed:" << server.errorString();
#ifndef Q_OS_WIN
            if (server.serverError() == QAbstractSocket::AddressInUseError)
            {
                QApplication tempApp{argc, argv};
                const auto ret = QMessageBox::question(nullptr, QCoreApplication::translate("main", "Already running"), QCoreApplication::translate("main",
                    "Evernus probably didn't close cleanly the last time. Do you want to try to perform a cleanup?"));
                if (ret == QMessageBox::Yes)
                {
                    qDebug() << "Cleanup attempt.";
                    if (!QFile::remove(serverName))
                    {
                        qDebug() << "Cleanup failed.";
                        QMessageBox::critical(nullptr,
                                              QCoreApplication::translate("main", "Error"),
                                              QCoreApplication::translate("main", "Couldn't remove '%1'!").arg(serverName));
                        return 1;
                    }
                    else
                    {
                        server.listen(serverName);
                    }
                }
            }
            else
            {
                qDebug() << "Running anyway...";
            }
#endif
        }

        qRegisterMetaType<Evernus::MarketLogExternalOrderImporterThread::ExternalOrderList>("ExternalOrderList");
        qRegisterMetaType<Evernus::EveType::IdType>("EveType::IdType");
        qRegisterMetaType<Evernus::Character::IdType>("Character::IdType");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::StatusFilters>("MarketOrderFilterProxyModel::StatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::StatusFilters>("StatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::PriceStatusFilters>("MarketOrderFilterProxyModel::PriceStatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::PriceStatusFilters>("PriceStatusFilters");
        qRegisterMetaType<std::vector<Evernus::ExternalOrder>>("std::vector<ExternalOrder>");
        qRegisterMetaType<Evernus::ExternalOrderImporter::TypeLocationPairs>("ExternalOrderImporter::TypeLocationPairs");
        qRegisterMetaType<Evernus::ExternalOrderModel::DeviationSourceType>("ExternalOrderModel::DeviationSourceType");
        qRegisterMetaType<Evernus::ExternalOrderModel::DeviationSourceType>("DeviationSourceType");
        qRegisterMetaType<Evernus::ContractFilterProxyModel::StatusFilters>("ContractFilterProxyModel::StatusFilters");

        Evernus::EvernusApplication app{argc, argv};

        app.registerImporter(Evernus::ExternalOrderImporterNames::webImporter,
                             std::make_unique<Evernus::CRESTExternalOrderImporter>(app.getDataProvider()));
        app.registerImporter(Evernus::ExternalOrderImporterNames::logImporter,
                             std::make_unique<Evernus::MarketLogExternalOrderImporter>());
        app.registerImporter(Evernus::ExternalOrderImporterNames::cacheImporter,
                             std::make_unique<Evernus::CacheExternalOrderImporter>());

        try
        {
            Evernus::MainWindow mainWnd{app,
                                        app.getMarketOrderProvider(),
                                        app.getCorpMarketOrderProvider(),
                                        app,
                                        app.getContractProvider(),
                                        app.getCorpContractProvider(),
                                        app.getDataProvider(),
                                        app,
                                        app,
                                        app};

            app.connect(&mainWnd, SIGNAL(refreshCharacters()), SLOT(refreshCharacters()));
            app.connect(&mainWnd, SIGNAL(refreshConquerableStations()), SLOT(refreshConquerableStations()));
            app.connect(&mainWnd, SIGNAL(importCharacter(Character::IdType)), SLOT(refreshCharacter(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importAssets(Character::IdType)), SLOT(refreshAssets(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importContracts(Character::IdType)), SLOT(refreshContracts(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importWalletJournal(Character::IdType)), SLOT(refreshWalletJournal(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importWalletTransactions(Character::IdType)), SLOT(refreshWalletTransactions(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importMarketOrdersFromAPI(Character::IdType)), SLOT(refreshMarketOrdersFromAPI(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importMarketOrdersFromLogs(Character::IdType)), SLOT(refreshMarketOrdersFromLogs(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importCorpContracts(Character::IdType)), SLOT(refreshCorpContracts(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importCorpWalletJournal(Character::IdType)), SLOT(refreshCorpWalletJournal(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importCorpWalletTransactions(Character::IdType)), SLOT(refreshCorpWalletTransactions(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importCorpMarketOrdersFromAPI(Character::IdType)), SLOT(refreshCorpMarketOrdersFromAPI(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importCorpMarketOrdersFromLogs(Character::IdType)), SLOT(refreshCorpMarketOrdersFromLogs(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &)), SLOT(refreshExternalOrdersFromWeb(const ExternalOrderImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(importExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &)), SLOT(refreshExternalOrdersFromFile(const ExternalOrderImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(importExternalOrdersFromCache(const ExternalOrderImporter::TypeLocationPairs &)), SLOT(refreshExternalOrdersFromCache(const ExternalOrderImporter::TypeLocationPairs &)));
            app.connect(&mainWnd, SIGNAL(preferencesChanged()), SLOT(handleNewPreferences()));
            app.connect(&mainWnd, SIGNAL(importFromMentat()), SLOT(importFromMentat()));
            app.connect(&mainWnd, SIGNAL(syncLMeve(Character::IdType)), SLOT(syncLMeve(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(showInEve(EveType::IdType)), SIGNAL(showInEve(EveType::IdType)));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, const QString &)), SLOT(addNewTaskInfo(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, uint, const QString &)), SIGNAL(newSubTaskInfoAdded(uint, uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskInfoChanged(uint, const QString &)), SIGNAL(taskInfoChanged(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskEnded(uint, const QString &)), SIGNAL(taskEnded(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(apiError(const QString &)), SLOT(showError(const QString &)));
            mainWnd.connect(&app, SIGNAL(conquerableStationsChanged()), SIGNAL(conquerableStationsChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SLOT(updateCharacters()));
            mainWnd.connect(&app, SIGNAL(assetsChanged()), SIGNAL(assetsChanged()));
            mainWnd.connect(&app, SIGNAL(externalOrdersChanged()), SIGNAL(externalOrdersChanged()));
            mainWnd.connect(&app, SIGNAL(externalOrdersChangedWithMarketOrders()), SIGNAL(externalOrdersChangedWithMarketOrders()));
            mainWnd.connect(&app, SIGNAL(walletJournalChanged()), SIGNAL(walletJournalChanged()));
            mainWnd.connect(&app, SIGNAL(walletTransactionsChanged()), SIGNAL(walletTransactionsChanged()));
            mainWnd.connect(&app, SIGNAL(marketOrdersChanged()), SIGNAL(marketOrdersChanged()));
            mainWnd.connect(&app, SIGNAL(contractsChanged()), SIGNAL(contractsChanged()));
            mainWnd.connect(&app, SIGNAL(corpWalletJournalChanged()), SIGNAL(corpWalletJournalChanged()));
            mainWnd.connect(&app, SIGNAL(corpWalletTransactionsChanged()), SIGNAL(corpWalletTransactionsChanged()));
            mainWnd.connect(&app, SIGNAL(corpMarketOrdersChanged()), SIGNAL(corpMarketOrdersChanged()));
            mainWnd.connect(&app, SIGNAL(corpContractsChanged()), SIGNAL(corpContractsChanged()));
            mainWnd.connect(&app, SIGNAL(itemCostsChanged()), SIGNAL(itemCostsChanged()));
            mainWnd.connect(&app, SIGNAL(itemVolumeChanged()), SIGNAL(itemVolumeChanged()));
            mainWnd.connect(&app, SIGNAL(lMeveTasksChanged()), SIGNAL(lMeveTasksChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SLOT(updateIskData()));
            mainWnd.connect(&app, SIGNAL(openMarginTool()), SLOT(showMarginTool()));
            mainWnd.connect(&app, SIGNAL(uploaderStatusChanged(const QString &)), SLOT(setUploaderStatus(const QString &)));
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
