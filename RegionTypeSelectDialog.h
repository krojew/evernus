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

#include <QDialog>

#include "ExternalOrderImporter.h"

class QListWidget;

namespace Evernus
{
    class EveDataProvider;

    class RegionTypeSelectDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit RegionTypeSelectDialog(const EveDataProvider &dataProvider, QWidget *parent = nullptr);
        virtual ~RegionTypeSelectDialog() = default;

    signals:
        void selected(const ExternalOrderImporter::TypeLocationPairs &pairs);

    private:
        QListWidget *mRegionList = nullptr;
        QListWidget *mTypeList = nullptr;
    };
}
