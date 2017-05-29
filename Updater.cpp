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
#include <stdexcept>

#include <QDesktopServices>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QDebug>
#include <QUrl>
#include <QDir>

#include "CorpMarketOrderValueSnapshotRepository.h"
#include "MarketOrderValueSnapshotRepository.h"
#include "WalletJournalEntryRepository.h"
#include "WalletTransactionRepository.h"
#include "ExternalOrderRepository.h"
#include "CachingEveDataProvider.h"
#include "RegionTypeSelectDialog.h"
#include "MarketOrderRepository.h"
#include "UpdateTimerRepository.h"
#include "CacheTimerRepository.h"
#include "EvernusApplication.h"
#include "RepositoryProvider.h"
#include "StatisticsSettings.h"
#include "UpdaterSettings.h"
#include "ItemRepository.h"
#include "ImportSettings.h"
#include "PriceSettings.h"
#include "OrderSettings.h"
#include "PathSettings.h"
#include "UISettings.h"
#include "Version.h"

#include "Updater.h"

namespace Evernus
{
    void Updater::performVersionMigration(const RepositoryProvider &provider) const
    {
        uint coreMajorVersion = 0;
        uint coreMinorVersion = 0;

        std::tie(coreMajorVersion, coreMinorVersion) = getCoreVersion();

        if ((coreMajorVersion == 0 && coreMinorVersion == 0) || (QString::number(coreMajorVersion) == version::majorStr() && QString::number(coreMinorVersion) == version::minorStr()))
            qDebug() << "Not updating core from" << coreMajorVersion << coreMinorVersion;
        else
            updateCore(coreMajorVersion, coreMinorVersion);

        uint dbMajorVersion = coreMajorVersion;
        uint dbMinorVersion = coreMinorVersion;

        std::tie(dbMajorVersion, dbMinorVersion) = getDbVersion(provider.getKeyRepository().getDatabase(), dbMajorVersion, dbMinorVersion);

        updateDatabase(dbMajorVersion, dbMinorVersion, provider);
    }

    void Updater::updateDatabaseVersion(const QSqlDatabase &db) const
    {
        const auto curVersion = QCoreApplication::applicationVersion().split('.');

        db.exec(QString{ R"(
            CREATE TABLE IF NOT EXISTS %1 (
                major INTEGER NOT NULL,
                minor INTEGER NOT NULL,
                PRIMARY KEY (major, minor)
            )
        )" }.arg(version::dbTableName()));

        const auto error = db.lastError();
        if (error.isValid())
            throw std::runtime_error{ tr("Error updating db version: %1").arg(error.text()).toStdString() };

        uint coreMajorVersion = 0;
        uint coreMinorVersion = 0;

        std::tie(coreMajorVersion, coreMinorVersion) = getCoreVersion();

        uint dbMajorVersion = coreMajorVersion;
        uint dbMinorVersion = coreMinorVersion;

        std::tie(dbMajorVersion, dbMinorVersion) = getDbVersion(db, dbMajorVersion, dbMinorVersion);

        if (dbMajorVersion < coreMajorVersion || (dbMajorVersion == coreMajorVersion && dbMinorVersion < coreMinorVersion))
        {
            db.exec(QString{"DELETE FROM %1"}.arg(version::dbTableName()));

            QSqlQuery query{ db };
            if (!query.prepare(QString{"REPLACE INTO %1 (major, minor) VALUES (? ,?)"}.arg(version::dbTableName())))
                throw std::runtime_error{tr("Error updating db version: %1").arg(query.lastError().text()).toStdString()};

            query.bindValue(0, curVersion[0]);
            query.bindValue(1, curVersion[1]);

            if (!query.exec())
                throw std::runtime_error{tr("Error updating db version: %1").arg(query.lastError().text()).toStdString()};
        }
    }

    Updater &Updater::getInstance()
    {
        static Updater updater;
        return updater;
    }

    void Updater::checkForUpdates(bool quiet) const
    {
        if (mCheckingForUpdates)
            return;

        qDebug() << "Checking for updates...";

        mCheckingForUpdates = true;

        auto reply = mAccessManager.get(QNetworkRequest{QUrl{"http://evernus.com/latest_version.xml"}});
        connect(reply, &QNetworkReply::finished, this, [quiet, this] {
            finishCheck(quiet);
        });
    }

    void Updater::finishCheck(bool quiet) const
    {
        mCheckingForUpdates = false;

        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        const auto error = reply->error();
        if (error != QNetworkReply::NoError)
        {
            if (!quiet)
                QMessageBox::warning(nullptr, tr("Error"), tr("Error contacting update server: %1").arg(error));

            return;
        }

        QString docError;

        QDomDocument doc;
        if (!doc.setContent(reply, false, &docError))
        {
            if (!quiet)
                QMessageBox::warning(nullptr, tr("Error"), tr("Error parsing response from the update server: %1").arg(docError));

            return;
        }

        const auto nextVersionString = doc.documentElement().attribute("id");
        const auto nextVersion = nextVersionString.split('.');
        if (nextVersion.count() != 2)
        {
            if (!quiet)
                QMessageBox::warning(nullptr, tr("Error"), tr("Missing update version information!"));

            return;
        }

        const auto curVersion = QCoreApplication::applicationVersion().split('.');

        const auto nextMajor = nextVersion[0].toUInt();
        const auto curMajor = curVersion[0].toUInt();

        if ((nextMajor < curMajor) || (nextMajor == curMajor && nextVersion[1].toUInt() <= curVersion[1].toUInt()))
        {
            if (!quiet)
                QMessageBox::information(nullptr, tr("No update found"), tr("Your current version is up-to-date."));

            return;
        }

        const QUrl downloadUrl{"http://evernus.com/download"};

#ifndef Q_OS_WIN
        const auto ret = QMessageBox::question(
            nullptr, tr("Update found"), tr("A new version is available: %1\nDo you wish to download it now?").arg(nextVersionString));
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(downloadUrl);
#else
        const auto ret = QMessageBox::question(
            nullptr, tr("Update found"), tr("A new version is available: %1\nDo you wish to launch the updater?").arg(nextVersionString));
        if (ret == QMessageBox::Yes)
        {
            qDebug() << "Starting maintenance tool...";
            if (QProcess::startDetached(QDir{QCoreApplication::applicationDirPath()}.filePath("../maintenancetool.exe"), QStringList()))
            {
                QCoreApplication::exit();
            }
            else
            {
                qDebug() << "Failed.";
                if (QMessageBox::question(nullptr, tr("Update found"), tr("Couldn't launch updater. Download manually?")) == QMessageBox::Yes)
                    QDesktopServices::openUrl(downloadUrl);
            }
        }
#endif
    }

    void Updater::updateCore(uint majorVersion, uint minorVersion) const
    {
        qDebug() << "Update core from" << majorVersion << "." << minorVersion;

        if (majorVersion < 2)
        {
            if (majorVersion == 0)
            {
                if (minorVersion < 3)
                    migrateCoreTo03();
            }

            if (minorVersion < 36)
            {
                if (minorVersion < 30)
                {
                    if (minorVersion < 13)
                        migrateCoreTo113();

                    migrateCoreTo130();
                }

                migrateCoreTo136();
            }
        }

        QSettings settings;
        settings.remove(RegionTypeSelectDialog::settingsTypesKey);
        settings.remove(UpdaterSettings::askedToShowReleaseNotesKey);
    }

    void Updater
    ::updateDatabase(uint majorVersion, uint minorVersion, const RepositoryProvider &provider) const
    {
        qDebug() << "Update db from" << majorVersion << "." << minorVersion;

        const auto &characterRepo = provider.getCharacterRepository();
        const auto &cacheTimerRepo = provider.getCacheTimerRepository();
        const auto &characterOrderRepo = provider.getMarketOrderRepository();
        const auto &corporationOrderRepo = provider.getCorpMarketOrderRepository();
        const auto &walletJournalRepo = provider.getWalletJournalEntryRepository();
        const auto &corpWalletJournalRepo = provider.getCorpWalletJournalEntryRepository();
        const auto &walletTransactionRepo = provider.getWalletTransactionRepository();
        const auto &corpWalletTransactionRepo = provider.getCorpWalletTransactionRepository();
        const auto &updateTimerRepo = provider.getUpdateTimerRepository();
        const auto &orderValueSnapshotRepo = provider.getMarketOrderValueSnapshotRepository();
        const auto &corpOrderValueSnapshotRepo = provider.getCorpMarketOrderValueSnapshotRepository();
        const auto &externalOrderRepo = provider.getExternalOrderRepository();
        const auto &itemRepo = provider.getItemRepository();
        const auto &keyRepo = provider.getKeyRepository();
        const auto &citadelRepo = provider.getCitadelRepository();

        const auto dbBak = DatabaseUtils::backupDatabase(characterRepo.getDatabase());

        try
        {
            // TODO: refactor someday...
            if (majorVersion < 2)
            {
                if (majorVersion == 0)
                {
                    if (minorVersion < 5)
                        migrateDatabaseTo05(cacheTimerRepo, characterRepo, characterOrderRepo, corporationOrderRepo);
                }

                if (minorVersion < 53)
                {
                    if (minorVersion < 50)
                    {
                        if (minorVersion < 49)
                        {
                            if (minorVersion < 47)
                            {
                                if (minorVersion < 45)
                                {
                                    if (minorVersion < 41)
                                    {
                                        if (minorVersion < 27)
                                        {
                                            if (minorVersion < 23)
                                            {
                                                if (minorVersion < 16)
                                                {
                                                    if (minorVersion < 11)
                                                    {
                                                        if (minorVersion < 9)
                                                        {
                                                            if (minorVersion < 8)
                                                                migrateDatabaseTo18(externalOrderRepo);

                                                            migrateDatabaseTo19(characterRepo,
                                                                                walletJournalRepo,
                                                                                corpWalletJournalRepo,
                                                                                walletTransactionRepo,
                                                                                corpWalletTransactionRepo);
                                                        }

                                                        migrateDatabaseTo111(cacheTimerRepo, updateTimerRepo, characterRepo);
                                                    }

                                                    migrateDatabaseTo116(orderValueSnapshotRepo, corpOrderValueSnapshotRepo);
                                                }

                                                migrateDatabaseTo123(externalOrderRepo, itemRepo);
                                            }

                                            migrateDatabaseTo127(characterOrderRepo, corporationOrderRepo);
                                        }

                                        migrateDatabaseTo141(characterRepo);
                                    }

                                    migrateDatabaseTo145(characterRepo, keyRepo, characterOrderRepo, corporationOrderRepo);
                                }

                                migrateDatabaseTo147(characterOrderRepo, corporationOrderRepo);
                            }

                            migrateDatabaseTo149(citadelRepo);
                        }

                        migrateDatabaseTo150(citadelRepo);
                    }

                    migrateDatabaseTo153(itemRepo);
                }

                migrateDatabaseTo20(characterRepo);
            }

            updateDatabaseVersion(provider.getKeyRepository().getDatabase());
        }
        catch (...)
        {
            QMessageBox::critical(nullptr, tr("Update"), tr(
                "An error occurred during the update process.\n"
                "Database backup was saved as %1. Please read online help how to deal with this situation.").arg(dbBak));

            throw;
        }
    }

    void Updater::migrateCoreTo03() const
    {
        QSettings settings;
        settings.setValue(PriceSettings::autoAddCustomItemCostKey, PriceSettings::autoAddCustomItemCostDefault);
    }

    void Updater::migrateDatabaseTo05(const CacheTimerRepository &cacheTimerRepo,
                                      const Repository<Character> &characterRepo,
                                      const MarketOrderRepository &characterOrderRepo,
                                      const MarketOrderRepository &corporationOrderRepo) const
    {
        cacheTimerRepo.exec(QString{"DROP TABLE %1"}.arg(cacheTimerRepo.getTableName()));
        cacheTimerRepo.create(characterRepo);

        safelyExecQuery(characterRepo, QString{"ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0"}.arg(characterRepo.getTableName()));
        safelyExecQuery(characterOrderRepo, QString{"ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0"}.arg(characterOrderRepo.getTableName()));
        safelyExecQuery(corporationOrderRepo, QString{"ALTER TABLE %1 RENAME TO %1_temp"}.arg(corporationOrderRepo.getTableName()));

        corporationOrderRepo.dropIndexes(characterRepo);
        corporationOrderRepo.create(characterRepo);
        corporationOrderRepo.copyDataWithoutCorporationIdFrom(QString{"%1_temp"}.arg(corporationOrderRepo.getTableName()));
        corporationOrderRepo.exec(QString{"DROP TABLE %1_temp"}.arg(corporationOrderRepo.getTableName()));

        QMessageBox::information(nullptr, tr("Update"), tr(
            "This update requires re-importing all data.\n"
            "Please click on \"Import all\" after the update."));
    }

    void Updater::migrateDatabaseTo18(const ExternalOrderRepository &externalOrderRepo) const
    {
        QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing all item prices."));

        externalOrderRepo.exec(QString{"DROP TABLE %1"}.arg(externalOrderRepo.getTableName()));
        externalOrderRepo.create();
    }

    void Updater::migrateDatabaseTo19(const Repository<Character> &characterRepo,
                                      const WalletJournalEntryRepository &walletJournalRepo,
                                      const WalletJournalEntryRepository &corpWalletJournalRepo,
                                      const WalletTransactionRepository &walletTransactionRepo,
                                      const WalletTransactionRepository &corpWalletTransactionRepo) const
    {
        QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing all corporation transactions and journal."));

        corpWalletJournalRepo.exec(QString{"DROP TABLE %1"}.arg(corpWalletJournalRepo.getTableName()));
        corpWalletJournalRepo.create(characterRepo);
        corpWalletTransactionRepo.exec(QString{"DROP TABLE %1"}.arg(corpWalletTransactionRepo.getTableName()));
        corpWalletTransactionRepo.create(characterRepo);

        safelyExecQuery(walletJournalRepo, QString{"ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0"}.arg(walletJournalRepo.getTableName()));
        safelyExecQuery(walletTransactionRepo, QString{"ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0"}.arg(walletTransactionRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo111(const CacheTimerRepository &cacheTimerRepo,
                                       const UpdateTimerRepository &updateTimerRepo,
                                       const Repository<Character> &characterRepo) const
    {
        cacheTimerRepo.exec(QString{"DROP TABLE %1"}.arg(cacheTimerRepo.getTableName()));
        cacheTimerRepo.create(characterRepo);
        updateTimerRepo.exec(QString{"DROP TABLE %1"}.arg(updateTimerRepo.getTableName()));
        updateTimerRepo.create(characterRepo);
    }

    void Updater::migrateCoreTo113() const
    {
        QSettings settings;
        settings.setValue(ImportSettings::ignoreCachedImportKey, false);
        settings.setValue(StatisticsSettings::combineCorpAndCharPlotsKey,
                          settings.value("rpices/combineCorpAndCharPlots", StatisticsSettings::combineCorpAndCharPlotsDefault));
    }

    void Updater::migrateDatabaseTo116(const MarketOrderValueSnapshotRepository &orderValueSnapshotRepo,
                                       const CorpMarketOrderValueSnapshotRepository &corpOrderValueSnapshotRepo) const
    {
        const auto updateShots = [](const auto &repo) {
            auto query = repo.prepare(QString{
                "UPDATE %1 SET buy_value = buy_value / 2, sell_value = sell_value / 2 WHERE timestamp >= ?"}.arg(repo.getTableName()));
            query.bindValue(0, QDateTime{QDate{2014, 9, 9}, QTime{0, 0}, Qt::UTC});

            Evernus::DatabaseUtils::execQuery(query);
        };

        updateShots(orderValueSnapshotRepo);
        updateShots(corpOrderValueSnapshotRepo);
    }

    void Updater::migrateDatabaseTo123(const ExternalOrderRepository &externalOrderRepo, const ItemRepository &itemRepo) const
    {
        QSettings settings;
        settings.setValue(OrderSettings::deleteOldMarketOrdersKey, false);

        externalOrderRepo.exec(QString{"DROP INDEX IF EXISTS %1_type_id_location"}.arg(externalOrderRepo.getTableName()));
        safelyExecQuery(itemRepo, QString{"ALTER TABLE %1 ADD COLUMN raw_quantity INTEGER NOT NULL DEFAULT 0"}.arg(itemRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo127(const MarketOrderRepository &characterOrderRepo,
                               const MarketOrderRepository &corporationOrderRepo) const
    {
        const QString sql = "ALTER TABLE %1 ADD COLUMN notes TEXT NULL DEFAULT NULL";
        safelyExecQuery(characterOrderRepo, sql.arg(characterOrderRepo.getTableName()));
        safelyExecQuery(corporationOrderRepo, sql.arg(corporationOrderRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo141(const Repository<Character> &characterRepo) const
    {
        safelyExecQuery(characterRepo, QString{"ALTER TABLE %1 ADD COLUMN brokers_fee FLOAT NULL DEFAULT NULL"}.arg(characterRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo145(const CharacterRepository &characterRepo,
                                       const KeyRepository &keyRepository,
                                       const MarketOrderRepository &characterOrderRepo,
                                       const MarketOrderRepository &corporationOrderRepo) const
    {
        QMessageBox::information(nullptr, tr("Update"), tr("This update requires settings your custom broker's fee again."));

        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN sell_brokers_fee FLOAT NULL DEFAULT NULL").arg(characterRepo.getTableName()));

        safelyExecQuery(characterOrderRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN custom_location_id INTEGER NULL DEFAULT NULL").arg(characterOrderRepo.getTableName()));
        safelyExecQuery(corporationOrderRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN custom_location_id INTEGER NULL DEFAULT NULL").arg(corporationOrderRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo147(const MarketOrderRepository &characterOrderRepo,
                                       const MarketOrderRepository &corporationOrderRepo) const
    {
        const auto query = QStringLiteral("ALTER TABLE %1 ADD COLUMN color_tag TEXT NULL");
        safelyExecQuery(characterOrderRepo, query.arg(characterOrderRepo.getTableName()));
        safelyExecQuery(corporationOrderRepo, query.arg(corporationOrderRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo149(const CitadelRepository &citadelRepo) const
    {
        // disable - never released
        //QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing citadels."));

        citadelRepo.deleteAll();
        safelyExecQuery(citadelRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN region_id INTEGER NOT NULL DEFAULT 0").arg(citadelRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo150(const CitadelRepository &citadelRepo) const
    {
        QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing citadels."));

        citadelRepo.deleteAll();
        safelyExecQuery(citadelRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN type_id INTEGER NOT NULL DEFAULT 0").arg(citadelRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo153(const ItemRepository &itemRepo) const
    {
        safelyExecQuery(itemRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN custom_value NUMERIC NULL DEFAULT NULL").arg(itemRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo20(const Repository<Character> &characterRepo) const
    {
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN reprocessing_implant_bonus FLOAT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN arkonor_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN bistot_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN crokite_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN dark_ochre_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN gneiss_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN hedbergite_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN hemorphite_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN ice_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN jaspet_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN kernite_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN mercoxit_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN omber_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN plagioclase_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN pyroxeres_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN reprocessing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN reprocessing_efficiency TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN scordite_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN scrapmetal_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN spodumain_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN veldspar_processing TINYINT NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
    }

    void Updater::migrateCoreTo130() const
    {
        QFile::remove(CachingEveDataProvider::getCacheDir().filePath(CachingEveDataProvider::systemDistanceCacheFileName));
    }

    void Updater::migrateCoreTo136() const
    {
        QSettings settings;
        settings.remove(UISettings::tabShowStateParentKey);
    }

    std::pair<uint, uint> Updater::getCoreVersion()
    {
        QSettings settings;

        const auto curVersion
            = settings.value(EvernusApplication::versionKey, QCoreApplication::applicationVersion()).toString().split('.');

        return std::make_pair(curVersion[0].toUInt(), curVersion[1].toUInt());
    }

    std::pair<uint, uint> Updater::getDbVersion(const QSqlDatabase &db, uint defaultMajor, uint defaultMinor)
    {
        auto query = db.exec(QString{"SELECT major, minor FROM %1"}.arg(version::dbTableName()));
        if (query.next())
        {
            defaultMajor = query.value(0).toUInt();
            defaultMinor = query.value(1).toUInt();
        }

        return std::make_pair(defaultMajor, defaultMinor);
    }

    template<class T>
    void Updater::safelyExecQuery(const Repository<T> &repo, const QString &query)
    {
        try
        {
            repo.exec(query);
        }
        catch (const std::runtime_error &e)
        {
            qWarning() << "Ignoring updater query error:" << query <<e.what();
        }
    }
}
