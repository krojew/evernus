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

#include <QtGlobal>

namespace Evernus
{
    namespace ImportSettings
    {
        enum class SmtpConnectionSecurity
        {
            None,
            STARTTLS,
            TLS
        };

        enum class PriceImportSource
        {
            Web,
            Logs,
            Cache
        };

        enum class MarketOrderImportSource
        {
            API,
            Logs
        };

        const auto importSkillsDefault = true;
        const auto importPortraitDefault = true;
        const auto importAssetsDefault = true;
        const auto autoUpdateAssetValueDefault = true;
        const auto updateOnlyFullAssetValueDefault = false;
        const auto importTimerDefault = 60;
        const auto autoImportEnabledDefault = false;
        const auto autoImportTimerDefault = 60;
        const auto emailNotificationsEnabledDefault = true;
        const auto smtpHostDefault = "localhost";
        const auto smtpPortDefault = 25;
        const auto priceImportSourceDefault = PriceImportSource::Web;
        const auto corpOrdersWithCharacterDefault = true;
        const auto importContractsDefault = true;
        const auto ignoreCachedImportDefault = true;
        const auto marketOrderImportSourceDefault = MarketOrderImportSource::API;
        const auto makeCorpSnapshotsDefault = true;

        const auto smtpCryptKey = Q_UINT64_C(0x740376004af2acc9);

        const auto importSkillsKey = "import/character/importSkills";
        const auto importPortraitKey = "import/character/importPortrait";
        const auto importAssetsKey = "import/assets/import";
        const auto autoUpdateAssetValueKey = "import/assets/autoUpdateValue";
        const auto updateOnlyFullAssetValueKey = "import/assets/updateOnlyFullAssetValue";
        const auto maxCharacterAgeKey = "import/character/maxAge";
        const auto maxAssetListAgeKey = "import/assetList/maxAge";
        const auto maxWalletAgeKey = "import/wallet/maxAge";
        const auto maxMarketOrdersAgeKey = "import/marketOrders/maxAge";
        const auto maxContractsAgeKey = "import/contracts/maxAge";
        const auto updateCorpDataKey = "import/corp/update";
        const auto makeCorpSnapshotsKey = "import/corp/makeSnapshots";
        const auto autoImportEnabledKey = "import/autoImport/enabled";
        const auto autoImportTimeKey = "import/autoImport/time";
        const auto emailNotificationsEnabledKey = "import/email/enabled";
        const auto emailNotificationAddressKey = "import/email/address";
        const auto smtpConnectionSecurityKey = "import/email/connectionSecurity";
        const auto smtpHostKey = "import/email/smtpHost";
        const auto smtpPortKey = "import/email/smtpPort";
        const auto smtpUserKey = "import/email/smtpUser";
        const auto smtpPasswordKey = "import/email/smtpPassword";
        const auto priceImportSourceKey = "import/source/price";
        const auto corpOrdersWithCharacterKey = "import/corp/showWithCharacter";
        const auto importContractsKey = "import/contracts/import";
        const auto ignoreCachedImportKey = "import/ignoreCached";
        const auto marketOrderImportSourceKey = "import/source/marketOrder";
    }
}
