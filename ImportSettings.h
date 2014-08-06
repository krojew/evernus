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
    namespace ImportSettings
    {
        const auto importTimerDefault = 60;

        const auto importSkillsKey = "import/character/importSkills";
        const auto importPortraitKey = "import/character/importPortrait";
        const auto importAssetsKey = "import/assets/import";
        const auto autoUpdateAssetValueKey = "import/assets/autoUpdateValue";
        const auto updateOnlyFullAssetValueKey = "import/assets/updateOnlyFullAssetValue";
        const auto maxCharacterAgeKey = "import/character/maxAge";
        const auto maxAssetListAgeKey = "import/assetList/maxAge";
        const auto maxWalletAgeKey = "import/wallet/maxAge";
        const auto maxMarketOrdersAgeKey = "import/marketOrders/maxAge";
    }
}
