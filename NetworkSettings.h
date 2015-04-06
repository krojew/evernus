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
    namespace NetworkSettings
    {
        const auto useProxyDefault = false;
        const auto useCustomProviderDefault = false;
        const auto defaultAPIProvider = "https://api.eveonline.com";

        const auto cryptKey = Q_UINT64_C(0x468c4a0e33a6fe01);

        const auto useProxyKey = "network/useProxy";
        const auto proxyTypeKey = "network/proxy/type";
        const auto proxyHostKey = "network/proxy/host";
        const auto proxyPortKey = "network/proxy/port";
        const auto proxyUserKey = "network/proxy/user";
        const auto proxyPasswordKey = "network/proxy/password";
        const auto useCustomProviderKey = "network/useCustomProvider";
        const auto providerHostKey = "network/provider/host";
    }
}
