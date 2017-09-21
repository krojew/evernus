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

#include <boost/throw_exception.hpp>

#include <QDesktopServices>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QtDebug>
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
#include "SSOSettings.h"
#include "UISettings.h"
#include "Version.h"

#include "Updater.h"

namespace Evernus
{
    Updater::Updater()
        : QObject{}
        , mCoreUpdateSteps{
            { {0, 3}, &Updater::migrateCoreTo03 },
            { {1, 13}, &Updater::migrateCoreTo113 },
            { {1, 30}, &Updater::migrateCoreTo130 },
            { {1, 36}, &Updater::migrateCoreTo136 },
            { {2, 3}, &Updater::migrateCoreTo23 },
            { {2, 7}, &Updater::migrateCoreTo27 },
        }
        , mDbUpdateSteps{
            { {0, 5}, [=](const auto &provider) {
                migrateDatabaseTo05(provider.getCacheTimerRepository(),
                                    provider.getCharacterRepository(),
                                    provider.getMarketOrderRepository(),
                                    provider.getCorpMarketOrderRepository());
            } },
            { {1, 8}, [=](const auto &provider) {
                migrateDatabaseTo18(provider.getExternalOrderRepository());
            } },
            { {1, 9}, [=](const auto &provider) {
                migrateDatabaseTo19(provider.getCharacterRepository(),
                                    provider.getWalletJournalEntryRepository(),
                                    provider.getCorpWalletJournalEntryRepository(),
                                    provider.getWalletTransactionRepository(),
                                    provider.getCorpWalletTransactionRepository());
            } },
            { {1, 11}, [=](const auto &provider) {
                migrateDatabaseTo111(provider.getCacheTimerRepository(),
                                     provider.getUpdateTimerRepository(),
                                     provider.getCharacterRepository());
            } },
            { {1, 16}, [=](const auto &provider) {
                migrateDatabaseTo116(provider.getMarketOrderValueSnapshotRepository(),
                                     provider.getCorpMarketOrderValueSnapshotRepository());
            } },
            { {1, 23}, [=](const auto &provider) {
                migrateDatabaseTo123(provider.getExternalOrderRepository(),
                                     provider.getItemRepository());
            } },
            { {1, 27}, [=](const auto &provider) {
                migrateDatabaseTo127(provider.getMarketOrderRepository(),
                                     provider.getCorpMarketOrderRepository());
            } },
            { {1, 41}, [=](const auto &provider) {
                migrateDatabaseTo141(provider.getCharacterRepository());
            } },
            { {1, 45}, [=](const auto &provider) {
                migrateDatabaseTo145(provider.getCharacterRepository(),
                                     provider.getKeyRepository(),
                                     provider.getMarketOrderRepository(),
                                     provider.getCorpMarketOrderRepository());
            } },
            { {1, 47}, [=](const auto &provider) {
                migrateDatabaseTo147(provider.getMarketOrderRepository(),
                                     provider.getCorpMarketOrderRepository());
            } },
            { {1, 49}, [=](const auto &provider) {
                migrateDatabaseTo149(provider.getCitadelRepository());
            } },
            { {1, 50}, [=](const auto &provider) {
                migrateDatabaseTo150(provider.getCitadelRepository());
            } },
            { {1, 53}, [=](const auto &provider) {
                migrateDatabaseTo153(provider.getItemRepository());
            } },
            { {2, 0}, [=](const auto &provider) {
                migrateDatabaseTo20(provider.getCharacterRepository());
            } },
            { {2, 2}, [=](const auto &provider) {
                migrateDatabaseTo22(provider.getCitadelRepository(),
                                    provider.getCorpItemRepository());
            } },
            { {2, 3}, [=](const auto &provider) {
                migrateDatabaseTo23(provider.getCitadelRepository());
            } },
            { {2, 6}, [=](const auto &provider) {
                migrateDatabaseTo26(provider.getWalletJournalEntryRepository(),
                                    provider.getCorpWalletJournalEntryRepository(),
                                    provider.getCharacterRepository());
            } },
            { {2, 11}, [=](const auto &provider) {
                migrateDatabaseTo211(provider.getWalletTransactionRepository(),
                                     provider.getCorpWalletTransactionRepository(),
                                     provider.getCharacterRepository());
            } },
        }
    {
    }

    void Updater::performVersionMigration(const RepositoryProvider &provider) const
    {
        const auto savedCoreVersion = getSavedCoreVersion();
        const auto curCoreVersion = getCurrentCoreVersion();

        if ((savedCoreVersion.majorVersion() == 0 && savedCoreVersion.minorVersion() == 0) || (savedCoreVersion >= curCoreVersion))
            qInfo() << "Not updating core from" << savedCoreVersion;
        else
            updateCore(savedCoreVersion);

        const auto dbVersion = getDbVersion(provider.getKeyRepository().getDatabase(), savedCoreVersion);
        updateDatabase(dbVersion, provider);
    }

    void Updater::updateDatabaseVersion(const QSqlDatabase &db) const
    {
        const auto curCoreVersion = getCurrentCoreVersion();

        db.exec(QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS %1 (
                major INTEGER NOT NULL,
                minor INTEGER NOT NULL,
                PRIMARY KEY (major, minor)
            )
        )").arg(version::dbTableName()));

        const auto error = db.lastError();
        if (error.isValid())
            BOOST_THROW_EXCEPTION(std::runtime_error{tr("Error updating db version: %1").arg(error.text()).toStdString()});

        const auto dbVersion = getDbVersion(db, getSavedCoreVersion());
        if (dbVersion < curCoreVersion)
        {
            db.exec(QStringLiteral("DELETE FROM %1").arg(version::dbTableName()));

            QSqlQuery query{db};
            if (!query.prepare(QStringLiteral("REPLACE INTO %1 (major, minor) VALUES (? ,?)").arg(version::dbTableName())))
                BOOST_THROW_EXCEPTION(std::runtime_error{tr("Error updating db version: %1").arg(query.lastError().text()).toStdString()});

            query.bindValue(0, curCoreVersion.majorVersion());
            query.bindValue(1, curCoreVersion.minorVersion());

            if (!query.exec())
                BOOST_THROW_EXCEPTION(std::runtime_error{tr("Error updating db version: %1").arg(query.lastError().text()).toStdString()});
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

        qInfo() << "Checking for updates...";

        mCheckingForUpdates = true;

        auto reply = mAccessManager.get(QNetworkRequest{QUrl{QStringLiteral("http://evernus.com/latest_version.xml")}});
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

        const auto nextVersionString = doc.documentElement().attribute(QStringLiteral("id"));
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

        const QUrl downloadUrl{QStringLiteral("http://evernus.com/download")};

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
            qInfo() << "Starting maintenance tool...";
            if (QProcess::startDetached(QDir{QCoreApplication::applicationDirPath()}.filePath(QStringLiteral("../maintenancetool.exe")), QStringList()))
            {
                QCoreApplication::exit();
            }
            else
            {
                qInfo() << "Failed.";
                if (QMessageBox::question(nullptr, tr("Update found"), tr("Couldn't launch updater. Download manually?")) == QMessageBox::Yes)
                    QDesktopServices::openUrl(downloadUrl);
            }
        }
#endif
    }

    void Updater::updateCore(const QVersionNumber &prevVersion) const
    {
        qInfo() << "Update core from" << prevVersion;

        auto nextVersion = mCoreUpdateSteps.upper_bound(prevVersion);
        if (nextVersion == std::end(mCoreUpdateSteps))
        {
            qInfo() << "No updates found.";
        }
        else
        {
            for (; nextVersion != std::end(mCoreUpdateSteps); ++nextVersion)
            {
                qInfo() << "Updating core to:" << nextVersion->first;
                (this->*nextVersion->second)();
            }
        }

        QSettings settings;
        settings.remove(RegionTypeSelectDialog::settingsTypesKey);
        settings.remove(UpdaterSettings::askedToShowReleaseNotesKey);
    }

    void Updater
    ::updateDatabase(const QVersionNumber &prevVersion, const RepositoryProvider &provider) const
    {
        qInfo() << "Update db from" << prevVersion;

        const auto dbBak = DatabaseUtils::backupDatabase(provider.getCharacterRepository().getDatabase());

        try
        {
            auto nextVersion = mDbUpdateSteps.upper_bound(prevVersion);
            if (nextVersion == std::end(mDbUpdateSteps))
            {
                qInfo() << "No updates found.";
            }
            else
            {
                for (; nextVersion != std::end(mDbUpdateSteps); ++nextVersion)
                {
                    qInfo() << "Updating db to:" << nextVersion->first;
                    nextVersion->second(provider);
                }
            }

            updateDatabaseVersion(provider.getCharacterRepository().getDatabase());
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
        cacheTimerRepo.exec(QStringLiteral("DROP TABLE %1").arg(cacheTimerRepo.getTableName()));
        cacheTimerRepo.create(characterRepo);

        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0").arg(characterRepo.getTableName()));
        safelyExecQuery(characterOrderRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0").arg(characterOrderRepo.getTableName()));
        safelyExecQuery(corporationOrderRepo, QStringLiteral("ALTER TABLE %1 RENAME TO %1_temp").arg(corporationOrderRepo.getTableName()));

        corporationOrderRepo.dropIndexes(characterRepo);
        corporationOrderRepo.create(characterRepo);
        corporationOrderRepo.copyDataWithoutCorporationIdFrom(QStringLiteral("%1_temp").arg(corporationOrderRepo.getTableName()));
        corporationOrderRepo.exec(QStringLiteral("DROP TABLE %1_temp").arg(corporationOrderRepo.getTableName()));

        QMessageBox::information(nullptr, tr("Update"), tr(
            "This update requires re-importing all data.\n"
            "Please click on \"Import all\" after the update."));
    }

    void Updater::migrateDatabaseTo18(const ExternalOrderRepository &externalOrderRepo) const
    {
        QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing all item prices."));

        externalOrderRepo.exec(QStringLiteral("DROP TABLE %1").arg(externalOrderRepo.getTableName()));
        externalOrderRepo.create();
    }

    void Updater::migrateDatabaseTo19(const Repository<Character> &characterRepo,
                                      const WalletJournalEntryRepository &walletJournalRepo,
                                      const WalletJournalEntryRepository &corpWalletJournalRepo,
                                      const WalletTransactionRepository &walletTransactionRepo,
                                      const WalletTransactionRepository &corpWalletTransactionRepo) const
    {
        QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing all corporation transactions and journal."));

        corpWalletJournalRepo.exec(QStringLiteral("DROP TABLE %1").arg(corpWalletJournalRepo.getTableName()));
        corpWalletJournalRepo.create(characterRepo);
        corpWalletTransactionRepo.exec(QStringLiteral("DROP TABLE %1").arg(corpWalletTransactionRepo.getTableName()));
        corpWalletTransactionRepo.create(characterRepo);

        safelyExecQuery(walletJournalRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0").arg(walletJournalRepo.getTableName()));
        safelyExecQuery(walletTransactionRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN corporation_id INTEGER NOT NULL DEFAULT 0").arg(walletTransactionRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo111(const CacheTimerRepository &cacheTimerRepo,
                                       const UpdateTimerRepository &updateTimerRepo,
                                       const Repository<Character> &characterRepo) const
    {
        cacheTimerRepo.exec(QStringLiteral("DROP TABLE %1").arg(cacheTimerRepo.getTableName()));
        cacheTimerRepo.create(characterRepo);
        updateTimerRepo.exec(QStringLiteral("DROP TABLE %1").arg(updateTimerRepo.getTableName()));
        updateTimerRepo.create(characterRepo);
    }

    void Updater::migrateCoreTo113() const
    {
        QSettings settings;
        settings.setValue(ImportSettings::ignoreCachedImportKey, false);
        settings.setValue(StatisticsSettings::combineCorpAndCharPlotsKey,
                          settings.value(QStringLiteral("rpices/combineCorpAndCharPlots"), StatisticsSettings::combineCorpAndCharPlotsDefault));
    }

    void Updater::migrateDatabaseTo116(const MarketOrderValueSnapshotRepository &orderValueSnapshotRepo,
                                       const CorpMarketOrderValueSnapshotRepository &corpOrderValueSnapshotRepo) const
    {
        const auto updateShots = [](const auto &repo) {
            auto query = repo.prepare(QStringLiteral(
                "UPDATE %1 SET buy_value = buy_value / 2, sell_value = sell_value / 2 WHERE timestamp >= ?").arg(repo.getTableName()));
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

        externalOrderRepo.exec(QStringLiteral("DROP INDEX IF EXISTS %1_type_id_location").arg(externalOrderRepo.getTableName()));
        safelyExecQuery(itemRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN raw_quantity INTEGER NOT NULL DEFAULT 0").arg(itemRepo.getTableName()));
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
        safelyExecQuery(characterRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN brokers_fee FLOAT NULL DEFAULT NULL").arg(characterRepo.getTableName()));
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

    void Updater::migrateDatabaseTo22(const CitadelRepository &citadelRepo, const ItemRepository &corpItemRepo) const
    {
        safelyExecQuery(citadelRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN ignored INTEGER NOT NULL DEFAULT 0").arg(citadelRepo.getTableName()));
        safelyExecQuery(corpItemRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN custom_value NUMERIC NULL DEFAULT NULL").arg(corpItemRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo23(const CitadelRepository &citadelRepo) const
    {
        // re-add ignored column because of borked prev update
        safelyExecQuery(citadelRepo, QStringLiteral("ALTER TABLE %1 ADD COLUMN ignored INTEGER NOT NULL DEFAULT 0").arg(citadelRepo.getTableName()));
    }

    void Updater::migrateDatabaseTo26(const WalletJournalEntryRepository &walletJournalRepo,
                                      const WalletJournalEntryRepository &corpWalletJournalRepo,
                                      const Repository<Character> &characterRepo) const
    {
        const auto update = [&](const auto &repo) {
            safelyExecQuery(repo, QStringLiteral("DROP TABLE %1").arg(repo.getTableName()));
            repo.create(characterRepo);
        };

        update(walletJournalRepo);
        update(corpWalletJournalRepo);

        // ref type repo doesn't exist anymore...
        safelyExecQuery(walletJournalRepo, QStringLiteral("DROP TABLE IF EXISTS ref_types"));

        safelyExecQuery(characterRepo,
                        QStringLiteral("ALTER TABLE %1 ADD COLUMN manufacturing_time_implant_bonus FLOAT NOT NULL DEFAULT 0")
                            .arg(characterRepo.getTableName())
        );

        const auto addSkillColumn = [&](const auto &name) {
            safelyExecQuery(characterRepo,
                            QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 TINYINT NOT NULL DEFAULT 0")
                                .arg(characterRepo.getTableName())
                                .arg(name)
            );
        };

        addSkillColumn(QStringLiteral("industry"));
        addSkillColumn(QStringLiteral("advanced_industry"));
        addSkillColumn(QStringLiteral("advanced_small_ship_construction"));
        addSkillColumn(QStringLiteral("advanced_medium_ship_construction"));
        addSkillColumn(QStringLiteral("advanced_large_ship_construction"));
        addSkillColumn(QStringLiteral("avanced_industrial_ship_construction"));
        addSkillColumn(QStringLiteral("amarr_starship_engineering"));
        addSkillColumn(QStringLiteral("caldari_starship_engineering"));
        addSkillColumn(QStringLiteral("gallente_starship_engineering"));
        addSkillColumn(QStringLiteral("minmatar_starship_engineering"));
        addSkillColumn(QStringLiteral("electromagnetic_physics"));
        addSkillColumn(QStringLiteral("electronic_engineering"));
        addSkillColumn(QStringLiteral("graviton_physics"));
        addSkillColumn(QStringLiteral("high_energy_physics"));
        addSkillColumn(QStringLiteral("hydromagnetic_physics"));
        addSkillColumn(QStringLiteral("laser_physics"));
        addSkillColumn(QStringLiteral("mechanical_engineering"));
        addSkillColumn(QStringLiteral("molecular_engineering"));
        addSkillColumn(QStringLiteral("nuclear_physics"));
        addSkillColumn(QStringLiteral("plasma_physics"));
        addSkillColumn(QStringLiteral("quantum_physics"));
        addSkillColumn(QStringLiteral("rocket_science"));

        QMessageBox::information(nullptr, tr("Update"), tr("This update requires re-importing wallet journal."));
    }

    void Updater::migrateDatabaseTo211(const WalletTransactionRepository &walletTransactionRepo,
                                       const WalletTransactionRepository &corpWalletTransactionRepo,
                                       const Repository<Character> &characterRepo) const
    {
        safelyExecQuery(walletTransactionRepo, QStringLiteral("DROP TABLE IF EXISTS %1").arg(walletTransactionRepo.getTableName()));
        safelyExecQuery(corpWalletTransactionRepo, QStringLiteral("DROP TABLE IF EXISTS %1").arg(corpWalletTransactionRepo.getTableName()));

        walletTransactionRepo.create(characterRepo);
        corpWalletTransactionRepo.create(characterRepo);

        QMessageBox::information(nullptr, tr("Update"), tr("This update requires importing wallet transactions again."));
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

    void Updater::migrateCoreTo23() const
    {
        removeRefreshTokens();
    }

    void Updater::migrateCoreTo27() const
    {
        removeRefreshTokens();
    }

    void Updater::removeRefreshTokens() const
    {
        QSettings settings;
        settings.remove(RegionTypeSelectDialog::settingsTypesKey);

        settings.beginGroup(SSOSettings::refreshTokenGroup);
        settings.remove(QStringLiteral(""));
        settings.endGroup();
    }

    QVersionNumber Updater::getSavedCoreVersion()
    {
        QSettings settings;

        const auto curVersion
            = settings.value(EvernusApplication::versionKey, QCoreApplication::applicationVersion()).toString().split('.');

        return { curVersion[0].toInt(), curVersion[1].toInt() };
    }

    QVersionNumber Updater::getCurrentCoreVersion()
    {
        return { version::major(), version::minor() };
    }

    QVersionNumber Updater::getDbVersion(const QSqlDatabase &db, const QVersionNumber &defaultVersion)
    {
        auto query = db.exec(QStringLiteral("SELECT major, minor FROM %1").arg(version::dbTableName()));
        if (query.next())
            return { query.value(0).toInt(), query.value(1).toInt() };

        return defaultVersion;
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
            qWarning() << "Ignoring updater query error:" << query << e.what();
        }
    }
}
