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

#include "EveTypeRepository.h"

namespace Evernus
{
    QString EveTypeRepository::getTableName() const
    {
        return "invTypes";
    }

    QString EveTypeRepository::getIdColumn() const
    {
        return "typeID";
    }

    EveTypeRepository::EntityPtr EveTypeRepository::populate(const QSqlRecord &record) const
    {
        const auto marketGroupId = record.value("marketGroupID");
        const auto description = record.value("description");
        const auto raceId = record.value("raceID");

        auto type = std::make_shared<EveType>(record.value("typeID").value<EveType::IdType>());
        type->setGroupId(record.value("groupID").toUInt());
        type->setName(record.value("typeName").toString());
        type->setDescription((description.isNull()) ? (EveType::DescriptionType{}) : (description.toString()));
        type->setMass(record.value("mass").toDouble());
        type->setVolume(record.value("volume").toDouble());
        type->setCapacity(record.value("capacity").toDouble());
        type->setPortionSize(record.value("portionSize").toInt());
        type->setRaceId((raceId.isNull()) ? (EveType::RaceIdType{}) : (raceId.value<EveType::RaceIdType::value_type>()));
        type->setBasePrice(record.value("basePrice").toDouble());
        type->setPublished(record.value("published").toInt() != 0);
        type->setMarketGroupId((marketGroupId.isNull()) ? (EveType::MarketGroupIdType{}) : (marketGroupId.value<MarketGroup::IdType>()));
        type->setChanceOfDuplicating(record.value("chanceOfDuplicating").toDouble());
        type->setNew(false);

        return type;
    }

    std::unordered_map<EveType::IdType, QString> EveTypeRepository::fetchAllTradeableNames() const
    {
        auto query = exec(QString{"SELECT %1, typeName FROM %2 WHERE marketGroupID IS NOT NULL"}
            .arg(getIdColumn()).arg(getTableName()));

        std::unordered_map<EveType::IdType, QString> result;
        while (query.next())
            result[query.value(0).toUInt()] = query.value(1).toString();

        return result;
    }

    QStringList EveTypeRepository::getColumns() const
    {
        return QStringList{}
            << "typeID"
            << "groupID"
            << "typeName"
            << "description"
            << "mass"
            << "volume"
            << "capacity"
            << "portionSize"
            << "raceID"
            << "basePrice"
            << "published"
            << "marketGroupID"
            << "chanceOfDuplicating";
    }

    void EveTypeRepository::bindValues(const EveType &entity, QSqlQuery &query) const
    {
        Q_UNUSED(entity);
        Q_UNUSED(query);

        throw std::logic_error{"Type repository is read-only."};
    }

    void EveTypeRepository::bindPositionalValues(const EveType &entity, QSqlQuery &query) const
    {
        Q_UNUSED(entity);
        Q_UNUSED(query);

        throw std::logic_error{"Type repository is read-only."};
    }
}
