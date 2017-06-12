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
#include <algorithm>

#include <QDataStream>
#include <QByteArray>

#include "RegionTypePresetRepository.h"

namespace Evernus
{
    QString RegionTypePresetRepository::getTableName() const
    {
        return QStringLiteral("region_type_presets");
    }

    QString RegionTypePresetRepository::getIdColumn() const
    {
        return QStringLiteral("name");
    }

    RegionTypePresetRepository::EntityPtr RegionTypePresetRepository::populate(const QSqlRecord &record) const
    {
        auto preset = std::make_shared<RegionTypePreset>(record.value(getIdColumn()).value<RegionTypePreset::IdType>());
        preset->setTypes(decodeRawSet<RegionTypePreset::TypeSet::key_type>(record, "types"));
        preset->setRegions(decodeRawSet<RegionTypePreset::RegionSet::key_type>(record, "regions"));
        preset->setNew(false);

        return preset;
    }

    void RegionTypePresetRepository::create() const
    {
        exec(QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
            "%2 TEXT PRIMARY KEY,"
            "types BLOB NOT NULL,"
            "regions BLOB NOT NULL"
        ")").arg(getTableName()).arg(getIdColumn()));
    }

    QStringList RegionTypePresetRepository::getAllNames() const
    {
        auto query = exec(QString{"SELECT %1 FROM %2 ORDER BY %1"}.arg(getIdColumn()).arg(getTableName()));

        QStringList result;
        while (query.next())
            result << query.value(0).toString();

        return result;
    }

    QStringList RegionTypePresetRepository::getColumns() const
    {
        return {
            getIdColumn(),
            QStringLiteral("types"),
            QStringLiteral("regions")
        };
    }

    void RegionTypePresetRepository::bindValues(const RegionTypePreset &entity, QSqlQuery &query) const
    {
        if (entity.getId() != RegionTypePreset::invalidId)
            query.bindValue(":" + getIdColumn(), entity.getId());

        query.bindValue(":types", encodeRawSet(entity.getTypes()));
        query.bindValue(":regions", encodeRawSet(entity.getRegions()));
    }

    void RegionTypePresetRepository::bindPositionalValues(const RegionTypePreset &entity, QSqlQuery &query) const
    {
        if (entity.getId() != RegionTypePreset::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(encodeRawSet(entity.getTypes()));
        query.addBindValue(encodeRawSet(entity.getRegions()));
    }

    template<class T>
    std::unordered_set<T> RegionTypePresetRepository::decodeRawSet(const QSqlRecord &record, const QString &name)
    {
        QDataStream stream{record.value(name).toByteArray()};

        QVariantList list;
        stream >> list;

        std::unordered_set<T> result;
        result.reserve(list.size());

        for (const auto &value : list)
            result.emplace(value.template value<T>());

        return result;
    }

    template<class T>
    QByteArray RegionTypePresetRepository::encodeRawSet(const std::unordered_set<T> &values)
    {
        QVariantList list;
        std::copy(std::begin(values), std::end(values), std::back_inserter(list));

        QByteArray data;

        QDataStream stream{&data, QIODevice::WriteOnly};
        stream << list;

        return data;
    }
}
