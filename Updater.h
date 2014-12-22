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
    class ExternalOrder;
    class Character;

    class Updater
        : public QObject
    {
        Q_OBJECT

    public:
        void performVersionMigration(const CacheTimerRepository &cacheTimerRepo,
                                     const UpdateTimerRepository &updateTimerRepo,
                                     const Repository<Character> &characterRepo,
                                     const ExternalOrderRepository &externalOrderRepo,
                                     const MarketOrderRepository &characterOrderRepo,
                                     const MarketOrderRepository &corporationOrderRepo,
                                     const WalletJournalEntryRepository &walletJournalRepo,
                                     const WalletJournalEntryRepository &corpWalletJournalRepo,
                                     const WalletTransactionRepository &walletTransactionRepo,
                                     const WalletTransactionRepository &corpWalletTransactionRepo,
                                     const MarketOrderValueSnapshotRepository &orderValueSnapshotRepo,
                                     const CorpMarketOrderValueSnapshotRepository &corpOrderValueSnapshotRepo) const;

        static Updater &getInstance();

    public slots:
        void checkForUpdates(bool quiet) const;

    private:
        mutable QNetworkAccessManager mAccessManager;

        mutable bool mCheckingForUpdates = false;

        Updater() = default;
        virtual ~Updater() = default;

        void finishCheck(bool quiet) const;

        void migrateTo03() const;
        void migrateTo05(const CacheTimerRepository &cacheTimerRepo,
                         const Repository<Character> &characterRepo,
                         const MarketOrderRepository &characterOrderRepo,
                         const MarketOrderRepository &corporationOrderRepo) const;
        void migrateTo18(const ExternalOrderRepository &externalOrderRepo) const;
        void migrateTo19(const Repository<Character> &characterRepo,
                         const WalletJournalEntryRepository &walletJournalRepo,
                         const WalletJournalEntryRepository &corpWalletJournalRepo,
                         const WalletTransactionRepository &walletTransactionRepo,
                         const WalletTransactionRepository &corpWalletTransactionRepo) const;
        void migrateTo111(const CacheTimerRepository &cacheTimerRepo,
                          const UpdateTimerRepository &updateTimerRepo,
                          const Repository<Character> &characterRepo) const;
        void migrateTo113() const;
        void migrateTo116(const MarketOrderValueSnapshotRepository &orderValueSnapshotRepo,
                          const CorpMarketOrderValueSnapshotRepository &corpOrderValueSnapshotRepo) const;
        void migrateTo123(const ExternalOrderRepository &externalOrderRepo) const;
    };
}
