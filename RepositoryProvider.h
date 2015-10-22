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

namespace Evernus
{
    class CorpMarketOrderValueSnapshotRepository;
    class MarketOrderValueSnapshotRepository;
    class CorpAssetValueSnapshotRepository;
    class WalletJournalEntryRepository;
    class AssetValueSnapshotRepository;
    class WalletJournalEntryRepository;
    class CorpWalletSnapshotRepository;
    class WalletTransactionRepository;
    class LocationBookmarkRepository;
    class WalletSnapshotRepository;
    class ExternalOrderRepository;
    class FavoriteItemRepository;
    class MarketOrderRepository;
    class OrderScriptRepository;
    class MarketGroupRepository;
    class UpdateTimerRepository;
    class FilterTextRepository;
    class CacheTimerRepository;
    class CharacterRepository;
    class ItemCostRepository;
    class CorpKeyRepository;
    class EveTypeRepository;
    class ItemRepository;
    class KeyRepository;

    class RepositoryProvider
    {
    public:
        RepositoryProvider() = default;
        virtual ~RepositoryProvider() = default;

        virtual const KeyRepository &getKeyRepository() const noexcept = 0;
        virtual const CorpKeyRepository &getCorpKeyRepository() const noexcept = 0;
        virtual const CharacterRepository &getCharacterRepository() const noexcept = 0;
        virtual const WalletSnapshotRepository &getWalletSnapshotRepository() const noexcept = 0;
        virtual const CorpWalletSnapshotRepository &getCorpWalletSnapshotRepository() const noexcept = 0;
        virtual const AssetValueSnapshotRepository &getAssetValueSnapshotRepository() const noexcept = 0;
        virtual const CorpAssetValueSnapshotRepository &getCorpAssetValueSnapshotRepository() const noexcept = 0;
        virtual const WalletJournalEntryRepository &getWalletJournalEntryRepository() const noexcept = 0;
        virtual const WalletTransactionRepository &getWalletTransactionRepository() const noexcept = 0;
        virtual const MarketOrderRepository &getMarketOrderRepository() const noexcept = 0;
        virtual const WalletJournalEntryRepository &getCorpWalletJournalEntryRepository() const noexcept = 0;
        virtual const WalletTransactionRepository &getCorpWalletTransactionRepository() const noexcept = 0;
        virtual const MarketOrderRepository &getCorpMarketOrderRepository() const noexcept = 0;
        virtual const ItemCostRepository &getItemCostRepository() const noexcept = 0;
        virtual const MarketOrderValueSnapshotRepository &getMarketOrderValueSnapshotRepository() const noexcept = 0;
        virtual const CorpMarketOrderValueSnapshotRepository &getCorpMarketOrderValueSnapshotRepository() const noexcept = 0;
        virtual const FilterTextRepository &getFilterTextRepository() const noexcept = 0;
        virtual const OrderScriptRepository &getOrderScriptRepository() const noexcept = 0;
        virtual const FavoriteItemRepository &getFavoriteItemRepository() const noexcept = 0;
        virtual const LocationBookmarkRepository &getLocationBookmarkRepository() const noexcept = 0;
        virtual const ExternalOrderRepository &getExternalOrderRepository() const noexcept = 0;
        virtual const EveTypeRepository &getEveTypeRepository() const noexcept = 0;
        virtual const MarketGroupRepository &getMarketGroupRepository() const noexcept = 0;
        virtual const CacheTimerRepository &getCacheTimerRepository() const noexcept = 0;
        virtual const UpdateTimerRepository &getUpdateTimerRepository() const noexcept = 0;
        virtual const ItemRepository &getItemRepository() const noexcept = 0;
    };
}
