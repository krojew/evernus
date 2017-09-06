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
#include "IndustryManufacturingSetupEntity.h"

namespace Evernus
{
    const QByteArray &IndustryManufacturingSetupEntity::getSetup() const &
    {
        return mSetup;
    }

    QByteArray &&IndustryManufacturingSetupEntity::getSetup() && noexcept
    {
        return std::move(mSetup);
    }

    void IndustryManufacturingSetupEntity::setSetup(QByteArray setup)
    {
        mSetup = std::move(setup);
    }
}
