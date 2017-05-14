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

#include <QNetworkAccessManager>

#include "Repository.h"

class QSqlDatabase;

namespace Evernus
{
    template<class T>
    class Repository;
    class CorpMarketOrderValueSnapshotRepository;
    class MarketOrderValueSnapshotRepository;
    class WalletJournalEntryRepository;
    class WalletTransactionRepository;
    class ExternalOrderRepository;
    class MarketOrderRepository;
    class UpdateTimerRepository;
    class CacheTimerRepository;
    class CharacterRepository;
    class RepositoryProvider;
    class CitadelRepository;
    class ItemRepository;
    class KeyRepository;
    class ExternalOrder;
    class Character;

    class Updater
        : public QObject
    {
        Q_OBJECT

    public:
        void performVersionMigration(const RepositoryProvider &provider) const;
        void updateDatabaseVersion(const QSqlDatabase &db) const;

        static Updater &getInstance();

    public slots:
        void checkForUpdates(bool quiet) const;

    private:
        mutable QNetworkAccessManager mAccessManager;

        mutable bool mCheckingForUpdates = false;

        Updater() = default;
        virtual ~Updater() = default;

        void finishCheck(bool quiet) const;

        void updateCore(uint prevMajor, uint prevMinor) const;
        void updateDatabase(uint prevMajor, uint prevMinor, const RepositoryProvider &provider) const;

        void migrateCoreTo03() const;
        void migrateDatabaseTo05(const CacheTimerRepository &cacheTimerRepo,
                                 const Repository<Character> &characterRepo,
                                 const MarketOrderRepository &characterOrderRepo,
                                 const MarketOrderRepository &corporationOrderRepo) const;
        void migrateDatabaseTo18(const ExternalOrderRepository &externalOrderRepo) const;
        void migrateDatabaseTo19(const Repository<Character> &characterRepo,
                                 const WalletJournalEntryRepository &walletJournalRepo,
                                 const WalletJournalEntryRepository &corpWalletJournalRepo,
                                 const WalletTransactionRepository &walletTransactionRepo,
                                 const WalletTransactionRepository &corpWalletTransactionRepo) const;
        void migrateDatabaseTo111(const CacheTimerRepository &cacheTimerRepo,
                                  const UpdateTimerRepository &updateTimerRepo,
                                  const Repository<Character> &characterRepo) const;
        void migrateCoreTo113() const;
        void migrateDatabaseTo116(const MarketOrderValueSnapshotRepository &orderValueSnapshotRepo,
                                  const CorpMarketOrderValueSnapshotRepository &corpOrderValueSnapshotRepo) const;
        void migrateDatabaseTo123(const ExternalOrderRepository &externalOrderRepo,
                                  const ItemRepository &itemRepo) const;
        void migrateDatabaseTo127(const MarketOrderRepository &characterOrderRepo,
                                  const MarketOrderRepository &corporationOrderRepo) const;
        void migrateDatabaseTo141(const Repository<Character> &characterRepo) const;
        void migrateDatabaseTo145(const CharacterRepository &characterRepo,
                                  const KeyRepository &keyRepository,
                                  const MarketOrderRepository &characterOrderRepo,
                                  const MarketOrderRepository &corporationOrderRepo) const;
        void migrateDatabaseTo147(const MarketOrderRepository &characterOrderRepo,
                                  const MarketOrderRepository &corporationOrderRepo) const;
        void migrateDatabaseTo149(const CitadelRepository &citadelRepo) const;
        void migrateDatabaseTo150(const CitadelRepository &citadelRepo) const;
        void migrateDatabaseTo153(const ItemRepository &itemRepo) const;

        void migrateCoreTo130() const;
        void migrateCoreTo136() const;

        static std::pair<uint, uint> getCoreVersion();
        static std::pair<uint, uint> getDbVersion(const QSqlDatabase &db, uint defaultMajor, uint defaultMinor);

        template<class T>
        static void safelyExecQuery(const Repository<T> &repo, const QString &query);
    };
}
