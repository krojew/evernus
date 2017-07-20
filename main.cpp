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
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMessageBox>
#include <QSettings>
#include <QSysInfo>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QUrl>

#ifdef Q_OS_OSX
#   include "QMacPasteboardMimeUnicodeText.h"
#endif

#ifdef Q_OS_LINUX
#   include <iostream>

#   include <client/linux/handler/exception_handler.h>

#   include "DumpUploader.h"
#endif

#include "MarketLogExternalOrderImporterThread.h"
#include "MarketLogExternalOrderImporter.h"
#include "ProxyWebExternalOrderImporter.h"
#include "MarketOrderFilterProxyModel.h"
#include "ExternalOrderImporterNames.h"
#include "MarketAnalysisDataFetcher.h"
#include "ContractFilterProxyModel.h"
#include "MarketOrderRepository.h"
#include "ChainableFileLogger.h"
#include "ExternalOrderModel.h"
#include "EvernusApplication.h"
#include "UpdaterSettings.h"
#include "MainWindow.h"
#include "VolumeType.h"
#include "Version.h"
#include "Defines.h"

#if EVERNUS_CREATE_DUMPS
namespace
{
    bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool succeeded)
    {
        Q_UNUSED(context);

        std::cerr << "Created dump: " << descriptor.path() << std::endl;
        return succeeded;
    }
}
#endif

int main(int argc, char *argv[])
{
    try
    {
        QCoreApplication::setApplicationName(QStringLiteral("Evernus"));
        QCoreApplication::setApplicationVersion(version::fullStr());
        QCoreApplication::setOrganizationDomain(QStringLiteral("evernus.com"));
        QCoreApplication::setOrganizationName(QStringLiteral("evernus.com"));
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#if EVERNUS_CREATE_DUMPS
        const auto dumpPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/dump");
        QDir{}.mkpath(dumpPath);

        google_breakpad::MinidumpDescriptor descriptor{
            dumpPath.toStdString()
        };
        google_breakpad::ExceptionHandler eh{
            descriptor,
            nullptr,
            dumpCallback,
            nullptr,
            true,
            -1
        };
#endif

        Evernus::ChainableFileLogger::initialize();

        qSetMessagePattern(QStringLiteral("[%{type}] %{time} %{threadid} %{message}"));

#ifdef Q_OS_WIN
        const auto serverName = QCoreApplication::applicationName() + ".socket";
#else
        const auto serverName = QDir::tempPath() + "/" + QCoreApplication::applicationName() + ".socket";
#endif

        qInfo() << QSysInfo::prettyProductName();
        qInfo() << QCoreApplication::applicationName() << QCoreApplication::applicationVersion();

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
        qRegisterMetaType<Evernus::MarketOrder::IdType>("MarketOrder::IdType");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::StatusFilters>("MarketOrderFilterProxyModel::StatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::StatusFilters>("StatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::PriceStatusFilters>("MarketOrderFilterProxyModel::PriceStatusFilters");
        qRegisterMetaType<Evernus::MarketOrderFilterProxyModel::PriceStatusFilters>("PriceStatusFilters");
        qRegisterMetaType<std::vector<Evernus::ExternalOrder>>("std::vector<ExternalOrder>");
        qRegisterMetaType<Evernus::ExternalOrderImporter::TypeLocationPairs>("ExternalOrderImporter::TypeLocationPairs");
        qRegisterMetaType<Evernus::MarketOrderRepository::TypeLocationPairs>("MarketOrderRepository::TypeLocationPairs");
        qRegisterMetaType<Evernus::ExternalOrderModel::DeviationSourceType>("ExternalOrderModel::DeviationSourceType");
        qRegisterMetaType<Evernus::ExternalOrderModel::DeviationSourceType>("DeviationSourceType");
        qRegisterMetaType<Evernus::ContractFilterProxyModel::StatusFilters>("ContractFilterProxyModel::StatusFilters");
        qRegisterMetaType<Evernus::MarketAnalysisDataFetcher::OrderResultType>("MarketAnalysisDataFetcher::OrderResultType");
        qRegisterMetaType<Evernus::MarketAnalysisDataFetcher::OrderResultType>("OrderResultType");
        qRegisterMetaType<Evernus::MarketAnalysisDataFetcher::HistoryResultType>("MarketAnalysisDataFetcher::HistoryResultType");
        qRegisterMetaType<Evernus::MarketAnalysisDataFetcher::HistoryResultType>("HistoryResultType");
        qRegisterMetaType<Evernus::VolumeType>("VolumeType");

        Evernus::EvernusApplication app{argc, argv};

#if EVERNUS_CREATE_DUMPS
        // hopefully we'll reach this point
        Evernus::DumpUploader uploader{dumpPath};
        uploader.run();
#endif

#ifdef Q_OS_OSX
        new QMacPasteboardMimeUnicodeText;
#endif

        auto webImporter = std::make_unique<Evernus::ProxyWebExternalOrderImporter>(app.getSSOClientId(),
                                                                                    app.getSSOClientSecret(),
                                                                                    app.getDataProvider(),
                                                                                    app.getCharacterRepository());
        auto webImporterPtr = webImporter.get();

        app.registerImporter(Evernus::ExternalOrderImporterNames::webImporter, std::move(webImporter));
        app.registerImporter(Evernus::ExternalOrderImporterNames::logImporter,
                             std::make_unique<Evernus::MarketLogExternalOrderImporter>());

        try
        {
            Evernus::MainWindow mainWnd{app.getSSOClientId(),
                                        app.getSSOClientSecret(),
                                        app,
                                        app.getMarketOrderProvider(),
                                        app.getCorpMarketOrderProvider(),
                                        app.getAssetProvider(),
                                        app.getCorpAssetProvider(),
                                        app.getContractProvider(),
                                        app.getCorpContractProvider(),
                                        app.getDataProvider(),
                                        app,
                                        app,
                                        app,
                                        app};

            QObject::connect(&mainWnd, &Evernus::MainWindow::refreshCharacters,
                             &app, &Evernus::EvernusApplication::refreshCharacters);
            QObject::connect(&mainWnd, &Evernus::MainWindow::refreshConquerableStations,
                             &app, &Evernus::EvernusApplication::refreshConquerableStations);
            QObject::connect(&mainWnd, &Evernus::MainWindow::refreshCitadels,
                             &app, &Evernus::EvernusApplication::refreshCitadels);
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCharacter,
                             &app, [&app](auto id) { app.refreshCharacter(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importAssets,
                             &app, [&app](auto id) { app.refreshAssets(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importContracts,
                             &app, [&app](auto id) { app.refreshContracts(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importWalletJournal,
                             &app, [&app](auto id) { app.refreshWalletJournal(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importWalletTransactions,
                             &app, [&app](auto id) { app.refreshWalletTransactions(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importMarketOrdersFromAPI,
                             &app, [&app](auto id) { app.refreshMarketOrdersFromAPI(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importMarketOrdersFromLogs,
                             &app, [&app](auto id) { app.refreshMarketOrdersFromLogs(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCorpAssets,
                             &app, [&app](auto id) { app.refreshCorpAssets(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCorpContracts,
                             &app, [&app](auto id) { app.refreshCorpContracts(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCorpWalletJournal,
                             &app, [&app](auto id) { app.refreshCorpWalletJournal(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCorpWalletTransactions,
                             &app, [&app](auto id) { app.refreshCorpWalletTransactions(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCorpMarketOrdersFromAPI,
                             &app, [&app](auto id) { app.refreshCorpMarketOrdersFromAPI(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importCorpMarketOrdersFromLogs,
                             &app, [&app](auto id) { app.refreshCorpMarketOrdersFromLogs(id); });
            QObject::connect(&mainWnd, &Evernus::MainWindow::importExternalOrdersFromWeb,
                             &app, &Evernus::EvernusApplication::refreshExternalOrdersFromWeb);
            QObject::connect(&mainWnd, &Evernus::MainWindow::importExternalOrdersFromFile,
                             &app, &Evernus::EvernusApplication::refreshExternalOrdersFromFile);
            QObject::connect(&mainWnd, &Evernus::MainWindow::preferencesChanged,
                             &app, &Evernus::EvernusApplication::handleNewPreferences);
            QObject::connect(&mainWnd, &Evernus::MainWindow::importFromMentat,
                             &app, &Evernus::EvernusApplication::importFromMentat);
            QObject::connect(&mainWnd, &Evernus::MainWindow::syncLMeve,
                             &app, &Evernus::EvernusApplication::syncLMeve);
            QObject::connect(&mainWnd, &Evernus::MainWindow::openMarketInEve,
                             &app, &Evernus::EvernusApplication::showInEve);
            QObject::connect(&mainWnd, &Evernus::MainWindow::setDestinationInEve,
                             &app, &Evernus::EvernusApplication::setDestinationInEve);
            QObject::connect(&mainWnd, &Evernus::MainWindow::updateExternalOrders,
                             &app, &Evernus::EvernusApplication::updateExternalOrdersAndAssetValue);
            QObject::connect(&mainWnd, &Evernus::MainWindow::clearCorpWalletData,
                             &app, &Evernus::EvernusApplication::clearCorpWalletData);
            QObject::connect(&mainWnd, &Evernus::MainWindow::makeValueSnapshots,
                             &app, &Evernus::EvernusApplication::makeValueSnapshots);
            QObject::connect(&mainWnd, &Evernus::MainWindow::citadelsEdited,
                             &app, &Evernus::EvernusApplication::clearCitadelCache);
            QObject::connect(&app, static_cast<void (Evernus::EvernusApplication::*)(uint, const QString &)>(&Evernus::EvernusApplication::taskStarted),
                             &mainWnd, &Evernus::MainWindow::addNewTaskInfo);
            QObject::connect(&app, static_cast<void (Evernus::EvernusApplication::*)(uint, uint, const QString &)>(&Evernus::EvernusApplication::taskStarted),
                             &mainWnd, &Evernus::MainWindow::newSubTaskInfoAdded);
            QObject::connect(&app, &Evernus::EvernusApplication::taskInfoChanged,
                             &mainWnd, &Evernus::MainWindow::taskInfoChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::taskEnded,
                             &mainWnd, &Evernus::MainWindow::taskEnded);
            QObject::connect(&app, &Evernus::EvernusApplication::conquerableStationsChanged,
                             &mainWnd, &Evernus::MainWindow::conquerableStationsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::citadelsChanged,
                             &mainWnd, &Evernus::MainWindow::citadelsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::charactersChanged,
                             &mainWnd, &Evernus::MainWindow::updateCharacters);
            QObject::connect(&app, &Evernus::EvernusApplication::assetsChanged,
                             &mainWnd, &Evernus::MainWindow::assetsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::externalOrdersChanged,
                             &mainWnd, &Evernus::MainWindow::externalOrdersChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::externalOrdersChangedWithMarketOrders,
                             &mainWnd, &Evernus::MainWindow::externalOrdersChangedWithMarketOrders);
            QObject::connect(&app, &Evernus::EvernusApplication::walletJournalChanged,
                             &mainWnd, &Evernus::MainWindow::walletJournalChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::walletTransactionsChanged,
                             &mainWnd, &Evernus::MainWindow::walletTransactionsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::marketOrdersChanged,
                             &mainWnd, &Evernus::MainWindow::marketOrdersChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::contractsChanged,
                             &mainWnd, &Evernus::MainWindow::contractsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::corpAssetsChanged,
                             &mainWnd, &Evernus::MainWindow::corpAssetsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::corpWalletJournalChanged,
                             &mainWnd, &Evernus::MainWindow::corpWalletJournalChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::corpWalletTransactionsChanged,
                             &mainWnd, &Evernus::MainWindow::corpWalletTransactionsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::corpMarketOrdersChanged,
                             &mainWnd, &Evernus::MainWindow::corpMarketOrdersChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::corpContractsChanged,
                             &mainWnd, &Evernus::MainWindow::corpContractsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::itemCostsChanged,
                             &mainWnd, &Evernus::MainWindow::itemCostsChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::itemVolumeChanged,
                             &mainWnd, &Evernus::MainWindow::itemVolumeChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::lMeveTasksChanged,
                             &mainWnd, &Evernus::MainWindow::lMeveTasksChanged);
            QObject::connect(&app, &Evernus::EvernusApplication::charactersChanged,
                             &mainWnd, &Evernus::MainWindow::updateIskData);
            QObject::connect(&app, &Evernus::EvernusApplication::openMarginTool,
                             &mainWnd, &Evernus::MainWindow::showMarginTool);
            QObject::connect(&app, &Evernus::EvernusApplication::snapshotsTaken,
                             &mainWnd, &Evernus::MainWindow::snapshotsTaken);
            QObject::connect(&mainWnd, &Evernus::MainWindow::preferencesChanged,
                             webImporterPtr, &Evernus::ProxyWebExternalOrderImporter::handleNewPreferences);
            mainWnd.showAsSaved();

            QSettings settings;
            if (!settings.value(Evernus::UpdaterSettings::askedToShowReleaseNotesKey, false).toBool())
            {
                const auto ret = QMessageBox::question(&mainWnd,
                                                       QCoreApplication::translate("main", "New version"),
                                                       QCoreApplication::translate("main", "Would you like to see what's new in this version?"));
                if (ret == QMessageBox::Yes)
                    QDesktopServices::openUrl(QUrl{QStringLiteral("http://evernus.com/latest-evernus-version")});

                settings.setValue(Evernus::UpdaterSettings::askedToShowReleaseNotesKey, true);
            }

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
