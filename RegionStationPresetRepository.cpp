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
#include "DatabaseUtils.h"

#include "RegionStationPresetRepository.h"

namespace Evernus
{
    QString RegionStationPresetRepository::getTableName() const
    {
        return QStringLiteral("region_station_presets");
    }

    QString RegionStationPresetRepository::getIdColumn() const
    {
        return QStringLiteral("name");
    }

    RegionStationPresetRepository::EntityPtr RegionStationPresetRepository::populate(const QSqlRecord &record) const
    {
        const auto stationId = record.value(QStringLiteral("station_id"));

        auto preset = std::make_shared<RegionStationPreset>(record.value(getIdColumn()).value<RegionStationPreset::IdType>());
        preset->setRegions(DatabaseUtils::decodeRawSet<RegionStationPreset::RegionSet::key_type>(record, QStringLiteral("regions")));
        preset->setNew(false);

        if (!stationId.isNull())
            preset->setStationId(stationId.value<RegionStationPreset::StationId::value_type>());

        return preset;
    }

    void RegionStationPresetRepository::create() const
    {
        exec(QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
            "%2 TEXT PRIMARY KEY,"
            "station_id BIGINT NULL,"
            "regions BLOB NOT NULL"
        ")").arg(getTableName()).arg(getIdColumn()));
    }

    QStringList RegionStationPresetRepository::getColumns() const
    {
        return {
            getIdColumn(),
            QStringLiteral("station_id"),
            QStringLiteral("regions")
        };
    }

    void RegionStationPresetRepository::bindValues(const RegionStationPreset &entity, QSqlQuery &query) const
    {
        if (entity.getId() != RegionStationPreset::invalidId)
            query.bindValue(QStringLiteral(":") + getIdColumn(), entity.getId());

        const auto stationId = entity.getStationId();

        query.bindValue(QStringLiteral(":station_id"), (stationId) ? (*stationId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(QStringLiteral(":regions"), DatabaseUtils::encodeRawSet(entity.getRegions()));
    }

    void RegionStationPresetRepository::bindPositionalValues(const RegionStationPreset &entity, QSqlQuery &query) const
    {
        if (entity.getId() != RegionStationPreset::invalidId)
            query.addBindValue(entity.getId());

        const auto stationId = entity.getStationId();

        query.addBindValue((stationId) ? (*stationId) : (QVariant{QVariant::ULongLong}));
        query.addBindValue(DatabaseUtils::encodeRawSet(entity.getRegions()));
    }
}
