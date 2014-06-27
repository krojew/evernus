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
