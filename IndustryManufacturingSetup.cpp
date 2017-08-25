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

#include "IndustryManufacturingSetup.h"

namespace Evernus
{
    IndustryManufacturingSetup::IndustryManufacturingSetup(const EveDataProvider &dataProvider)
        : mDataProvider{dataProvider}
    {
    }

    void IndustryManufacturingSetup::setOutputTypes(TypeSet types)
    {
        mOutputTypes = std::move(types);
        mSourceInfo.clear();

        TypeSet usedTypes;

        // build source info
        for (const auto output : mOutputTypes)
            fillManufacturingInfo(output, usedTypes);

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
            if (mTypeSettings.find(*it) != std::end(mTypeSettings))
                it = mOutputTypes.erase(it);
            else
                ++it;
        }
    }

    IndustryManufacturingSetup::TypeSet IndustryManufacturingSetup::getOutputTypes() const
    {
        return mOutputTypes;
    }

    const std::vector<EveDataProvider::MaterialInfo> &IndustryManufacturingSetup::getMaterialInfo(EveType::IdType typeId) const
    {
        const auto it = mSourceInfo.find(typeId);
        if (Q_LIKELY(it != std::end(mSourceInfo)))
            return it->second;

        throw std::logic_error{"Missing mnufacturing info for: " + std::to_string(typeId)};
    }

    void IndustryManufacturingSetup::fillManufacturingInfo(EveType::IdType typeId, TypeSet &usedTypes)
    {
        if (mSourceInfo.find(typeId) != std::end(mSourceInfo))
            return;

        const auto sources = mSourceInfo.emplace(typeId, mDataProvider.getTypeManufacturingInfo(typeId)).first;
        for (const auto &source : sources->second)
        {
            mTypeSettings.insert(std::make_pair(source.mMaterialId, TypeSettings{}));

            usedTypes.insert(source.mMaterialId);
            fillManufacturingInfo(source.mMaterialId, usedTypes);
        }
    }
}
