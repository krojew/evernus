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
    namespace PathSettings
    {
        const auto characterLogWildcardDefault = "My Orders-*.txt";
        const auto corporationLogWildcardDefault = "Corporation Orders-*.txt";

        const auto marketLogsPathKey = "path/marketLogs/path";
        const auto deleteLogsKey = "path/marketLogs/delete";
        const auto characterLogWildcardKey = "path/marketLogs/characterWildcard";
        const auto corporationLogWildcardKey = "path/marketLogs/corporationWildcard";

        const auto eveCachePathKey = "path/eve/cache";
    }
}
