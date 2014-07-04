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
#include <QSqlRecord>
#include <QSqlQuery>

#include "MarketGroupRepository.h"

namespace Evernus
{
    QString MarketGroupRepository::getTableName() const
    {
        return "invMarketGroups";
    }

    QString MarketGroupRepository::getIdColumn() const
    {
        return "marketGroupID";
    }

    MarketGroup MarketGroupRepository::populate(const QSqlRecord &record) const
    {
        const auto description = record.value("description");
        const auto parentId = record.value("parentGroupID");
        const auto iconId = record.value("iconID");

        MarketGroup group{record.value("marketGroupID").value<MarketGroup::IdType>()};
        group.setParentId((parentId.isNull()) ? (MarketGroup::ParentIdType{}) : (parentId.value<MarketGroup::IdType>()));
        group.setName(record.value("marketGroupName").toString());
        group.setDescription((description.isNull()) ? (MarketGroup::DescriptionType{}) : (description.toString()));
        group.setIconId((iconId.isNull()) ? (MarketGroup::IconIdType{}) : (iconId.value<MarketGroup::IconIdType::value_type>()));
        group.setHasTypes(record.value("hasTypes").toBool());
        group.setNew(false);

        return group;
    }

    QStringList MarketGroupRepository::getColumns() const
    {
        return QStringList{}
            << "marketGroupID"
            << "parentGroupID"
            << "marketGroupName"
            << "description"
            << "iconID"
            << "hasTypes";
    }

    void MarketGroupRepository::bindValues(const MarketGroup &entity, QSqlQuery &query) const
    {
        Q_UNUSED(entity);
        Q_UNUSED(query);

        throw std::logic_error{"Market group repository is read-only."};
    }
}
