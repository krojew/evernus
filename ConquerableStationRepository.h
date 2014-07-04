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

#include "ConquerableStationList.h"
#include "Repository.h"

namespace Evernus
{
    class ConquerableStationRepository
        : public Repository<ConquerableStation>
    {
    public:
        using Repository::Repository;
        virtual ~ConquerableStationRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual ConquerableStation populate(const QSqlRecord &record) const override;

        void create() const;
        void batchStore(const ConquerableStationList &list) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const ConquerableStation &entity, QSqlQuery &query) const override;
    };
}
