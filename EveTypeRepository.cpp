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

    EveType EveTypeRepository::populate(const QSqlRecord &record) const
    {
        const auto marketGroupId = record.value("marketGroupID");
        const auto description = record.value("description");
        const auto raceId = record.value("raceID");

        EveType type{record.value("typeID").value<EveType::IdType>()};
        type.setGroupId(record.value("groupID").toUInt());
        type.setName(record.value("typeName").toString());
        type.setDescription((description.isNull()) ? (EveType::DescriptionType{}) : (description.toString()));
        type.setMass(record.value("mass").toDouble());
        type.setVolume(record.value("volume").toDouble());
        type.setCapacity(record.value("capacity").toDouble());
        type.setPortionSize(record.value("portionSize").toInt());
        type.setRaceId((raceId.isNull()) ? (EveType::RaceIdType{}) : (raceId.value<EveType::RaceIdType::value_type>()));
        type.setBasePrice(record.value("basePrice").toDouble());
        type.setPublished(record.value("published").toInt() != 0);
        type.setMarketGroupId((marketGroupId.isNull()) ? (EveType::MarketGroupIdType{}) : (marketGroupId.value<MarketGroup::IdType>()));
        type.setChanceOfDuplicating(record.value("chanceOfDuplicating").toDouble());
        type.setNew(false);

        return type;
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
}
