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
#include <algorithm>
#include <cmath>

#include "IndustryUtils.h"

namespace Evernus
{
    namespace IndustryUtils
    {
        quint64 getRequiredQuantity(uint runs,
                                    uint baseQuantity,
                                    uint materialEfficiency)
        {
            const auto materialModifier = 1.f - materialEfficiency / 100.f; // TODO: implement
            return std::max(static_cast<float>(runs), std::ceil(runs * baseQuantity * materialModifier));
        }
    }
}
