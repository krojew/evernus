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
#include <QCoreApplication>

#include "TextUtils.h"

namespace Evernus
{
    namespace TextUtils
    {
        QString secondsToString(uint duration)
        {
            QString res;

            duration /= 60;

            const auto minutes = duration % 60;
            duration /= 60;

            const auto hours = duration % 24;
            const auto days = duration / 24;

            if (hours == 0 && days == 0)
                return res.sprintf(QCoreApplication::translate("TextUtils", "%02dmin").toLatin1().data(), minutes);

            if (days == 0)
                return res.sprintf(QCoreApplication::translate("TextUtils", "%02dh %02dmin").toLatin1().data(), hours, minutes);

            return res.sprintf(QCoreApplication::translate("TextUtils", "%dd %02dh").toLatin1().data(), days, hours);
        }
    }
}
