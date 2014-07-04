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
#include <QDateTime>

namespace Evernus
{
    template<class T>
    void CacheRepository<T>::clearOldData() const
    {
        auto query = this->prepare(QString{"DELETE FROM %1 WHERE cache_until < :dt"}.arg(this->getTableName()));
        query.bindValue(":dt", QDateTime::currentDateTimeUtc().toTime_t());
        query.exec();
    }
}
