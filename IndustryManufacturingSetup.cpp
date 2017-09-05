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
#include "IndustryManufacturingSetup.h"

namespace Evernus
{
    IndustryManufacturingSetup::IndustryManufacturingSetup(const EveDataProvider &dataProvider)
        : mDataProvider{dataProvider}
    {
    }

    void IndustryManufacturingSetup::setOutputTypes(TypeSet types)
    {
        mOutputTypes.clear();
        mManufacturingInfo.clear();

        for (const auto type : types)
            mOutputTypes.emplace(type, OutputSettings{});

        TypeSet usedTypes;

        // build source info
        for (auto output = std::begin(mOutputTypes); output != std::end(mOutputTypes);)
        {
            fillManufacturingInfo(output->first, usedTypes);
            if (Q_UNLIKELY(mManufacturingInfo[output->first].mMaterials.empty()))
                output = mOutputTypes.erase(output);
            else
                ++output;
        }

        // clean setup of unused types; mTypeSettings now contains both new and old sources
        for (auto it = std::begin(mTypeSettings); it != std::end(mTypeSettings);)
        {
            if (usedTypes.find(it->first) == std::end(usedTypes))
                it = mTypeSettings.erase(it);
            else
                ++it;
        }

        // if type is already used as a source, remove from output; mTypeSettings should contain all types except for output
        for (auto it = std::begin(mOutputTypes); it != std::end(mOutputTypes);)
        {
            if (mTypeSettings.find(it->first) != std::end(mTypeSettings))
                it = mOutputTypes.erase(it);
            else
                ++it;
        }
    }

    const IndustryManufacturingSetup::OutputTypeMap &IndustryManufacturingSetup::getOutputTypes() const
    {
        return mOutputTypes;
    }

    const EveDataProvider::ManufacturingInfo &IndustryManufacturingSetup::getManufacturingInfo(EveType::IdType typeId) const
    {
        const auto it = mManufacturingInfo.find(typeId);
        if (Q_LIKELY(it != std::end(mManufacturingInfo)))
            return it->second;

        throw NotSourceTypeException{"Missing manufacturing info for: " + std::to_string(typeId)};
    }

    const IndustryManufacturingSetup::TypeSettings &IndustryManufacturingSetup::getTypeSettings(EveType::IdType typeId) const
    {
        const auto it = mTypeSettings.find(typeId);
        if (Q_LIKELY(it != std::end(mTypeSettings)))
            return it->second;

        throw NotSourceTypeException{"Missing type settings for: " + std::to_string(typeId)};
    }

    const IndustryManufacturingSetup::OutputSettings &IndustryManufacturingSetup::getOutputSettings(EveType::IdType typeId) const
    {
        const auto it = mOutputTypes.find(typeId);
        if (Q_LIKELY(it != std::end(mOutputTypes)))
            return it->second;

        throw NotOutputTypeException{"Missing output settings for: " + std::to_string(typeId)};
    }

    IndustryManufacturingSetup::TypeSet IndustryManufacturingSetup::getAllTypes() const
    {
        TypeSet types;

        for (const auto &output : mOutputTypes)
            types.insert(output.first);
        for (const auto &type : mTypeSettings)
            types.insert(type.first);

        return types;
    }

    void IndustryManufacturingSetup::clear() noexcept
    {
        mOutputTypes.clear();
        mTypeSettings.clear();
        mManufacturingInfo.clear();
    }

    void IndustryManufacturingSetup::setSource(EveType::IdType id, InventorySource source)
    {
        const auto it = mTypeSettings.find(id);
        if (Q_LIKELY(it != std::end(mTypeSettings)))
        {
            it->second.mSource = source;
            return;
        }

        throw NotSourceTypeException{"Missing type settings for: " + std::to_string(id)};
    }

    void IndustryManufacturingSetup::setRuns(EveType::IdType id, uint runs)
    {
        const auto it = mOutputTypes.find(id);
        if (Q_LIKELY(it != std::end(mOutputTypes)))
        {
            it->second.mRuns = runs;
            return;
        }

        throw NotOutputTypeException{"Missing output settings for: " + std::to_string(id)};
    }

    void IndustryManufacturingSetup::setMaterialEfficiency(EveType::IdType id, uint value)
    {
        const auto findInMap = [=](auto &map) {
            const auto it = map.find(id);
            if (it != std::end(map))
            {
                it->second.mMaterialEfficiency = value;
                return true;
            }

            return false;
        };

        if (!findInMap(mOutputTypes))
            findInMap(mTypeSettings);
    }

    void IndustryManufacturingSetup::setTimeEfficiency(EveType::IdType id, uint value)
    {
        const auto findInMap = [=](auto &map) {
            const auto it = map.find(id);
            if (it != std::end(map))
            {
                it->second.mTimeEfficiency = value;
                return true;
            }

            return false;
        };

        if (!findInMap(mOutputTypes))
            findInMap(mTypeSettings);
    }

    void IndustryManufacturingSetup::fillManufacturingInfo(EveType::IdType typeId, TypeSet &usedTypes)
    {
        if (mManufacturingInfo.find(typeId) != std::end(mManufacturingInfo))
            return;

        const auto info = mManufacturingInfo.emplace(typeId, mDataProvider.getTypeManufacturingInfo(typeId)).first;
        for (const auto &source : info->second.mMaterials)
        {
            mTypeSettings.insert(std::make_pair(source.mMaterialId, TypeSettings{}));

            usedTypes.insert(source.mMaterialId);
            fillManufacturingInfo(source.mMaterialId, usedTypes);
        }
    }
}
