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
#include "IndustryManufacturingSetupModel.h"

#include "IndustryManufacturingSetupController.h"

namespace Evernus
{
    IndustryManufacturingSetupController::IndustryManufacturingSetupController(IndustryManufacturingSetupModel &model)
        : mModel{model}
    {
    }

    void IndustryManufacturingSetupController::setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source)
    {
        mModel.setSource(id, source);
        emit sourceChanged(id, source);
    }

    void IndustryManufacturingSetupController::setRuns(EveType::IdType id, uint runs)
    {
        mModel.setRuns(id, runs);
    }

    void IndustryManufacturingSetupController::setMaterialEfficiency(EveType::IdType id, uint value)
    {
        mModel.setMaterialEfficiency(id, value);
        emit materialEfficiencyChanged(id, value);
    }

    void IndustryManufacturingSetupController::setTimeEfficiency(EveType::IdType id, uint value)
    {
        mModel.setTimeEfficiency(id, value);
        emit timeEfficiencyChanged(id, value);
    }
}
