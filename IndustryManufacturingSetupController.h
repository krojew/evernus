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

#include <QObject>

#include "IndustryManufacturingSetup.h"
#include "EveType.h"

namespace Evernus
{
    class IndustryManufacturingSetupModel;

    class IndustryManufacturingSetupController
        : public QObject
    {
        Q_OBJECT

    public:
        using QObject::QObject;

        explicit IndustryManufacturingSetupController(IndustryManufacturingSetupModel &model);
        IndustryManufacturingSetupController(const IndustryManufacturingSetupController &) = default;
        IndustryManufacturingSetupController(IndustryManufacturingSetupController &&) = default;
        virtual ~IndustryManufacturingSetupController() = default;

        IndustryManufacturingSetupController &operator =(const IndustryManufacturingSetupController &) = default;
        IndustryManufacturingSetupController &operator =(IndustryManufacturingSetupController &&) = default;

    public slots:
        void setSource(EveType::IdType id, IndustryManufacturingSetup::InventorySource source);
        void setRuns(EveType::IdType id, uint runs);
        void setMaterialEfficiency(EveType::IdType id, uint value);
        void setTimeEfficiency(EveType::IdType id, uint value);

    signals:
        void typeSelected(EveType::IdType id);
        void sourceChanged(EveType::IdType id, IndustryManufacturingSetup::InventorySource source);
        void materialEfficiencyChanged(EveType::IdType id, uint value);
        void timeEfficiencyChanged(EveType::IdType id, uint value);

    private:
        IndustryManufacturingSetupModel &mModel;
    };
}
