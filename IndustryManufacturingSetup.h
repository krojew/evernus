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
#include <unordered_set>
#include <vector>

#include "EveDataProvider.h"
#include "EveType.h"

namespace Evernus
{
    class EveDataProvider;

    class IndustryManufacturingSetup final
    {
    public:
        using TypeSet = std::unordered_set<EveType::IdType>;

        enum class InventorySource
        {
            BuyFromSource,
            Manufacture,
            TakeAssetsThenBuyFromSource,
            TakeAssetsThenManufacture,
        };

        explicit IndustryManufacturingSetup(const EveDataProvider &dataProvider);
        IndustryManufacturingSetup(const IndustryManufacturingSetup &) = default;
        IndustryManufacturingSetup(IndustryManufacturingSetup &&) = default;
        ~IndustryManufacturingSetup() = default;

        void setOutputTypes(TypeSet types);
        TypeSet getOutputTypes() const;

        const EveDataProvider::ManufacturingInfo &getManufacturingInfo(EveType::IdType typeId) const;

        TypeSet getAllTypes() const;

        void clear() noexcept;

        IndustryManufacturingSetup &operator =(const IndustryManufacturingSetup &) = default;
        IndustryManufacturingSetup &operator =(IndustryManufacturingSetup &&) = default;

    private:
        struct TypeSettings
        {
            InventorySource mSource = InventorySource::Manufacture;
        };

        const EveDataProvider &mDataProvider;

        std::unordered_map<EveType::IdType, TypeSettings> mTypeSettings;
        std::unordered_map<EveType::IdType, EveDataProvider::ManufacturingInfo> mManufacturingInfo;

        TypeSet mOutputTypes;

        void fillManufacturingInfo(EveType::IdType typeId, TypeSet &usedTypes);
    };
}
