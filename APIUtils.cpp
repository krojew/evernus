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
#include "APIUtils.h"

#include <QXmlQuery>

namespace Evernus
{
    namespace APIUtils
    {
        QDateTime getCachedUntil(const QString &xml)
        {
            QString out;

            QXmlQuery query;
            query.setFocus(xml);
            query.setQuery("/eveapi/cachedUntil/text()");
            query.evaluateTo(&out);

            auto cachedUntil = QDateTime::fromString(out.trimmed(), eveTimeFormat);
            cachedUntil.setTimeSpec(Qt::UTC);

            return cachedUntil;
        }
    }
}
