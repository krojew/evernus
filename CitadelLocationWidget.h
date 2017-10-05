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

#include <QWidget>

#include "CitadelLocationModel.h"
#include "Character.h"

namespace Evernus
{
    class CitadelAccessCache;
    class EveDataProvider;

    class CitadelLocationWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        using CitadelList = CitadelLocationModel::CitadelList;

        CitadelLocationWidget(const EveDataProvider &dataProvider,
                              const CitadelAccessCache &citadelAccessCache,
                              Character::IdType charId,
                              QWidget *parent = nullptr);
        CitadelLocationWidget(const CitadelLocationWidget &) = default;
        CitadelLocationWidget(CitadelLocationWidget &&) = default;
        virtual ~CitadelLocationWidget() = default;

        CitadelList getSelectedCitadels() const;

        CitadelLocationWidget &operator =(const CitadelLocationWidget &) = default;
        CitadelLocationWidget &operator =(CitadelLocationWidget &&) = default;

    public slots:
        void refresh();

    private:
        CitadelLocationModel mModel;
    };
}
