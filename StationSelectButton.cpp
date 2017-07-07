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
#include "StationSelectDialog.h"
#include "EveDataProvider.h"

#include "StationSelectButton.h"

namespace Evernus
{
    StationSelectButton::StationSelectButton(const EveDataProvider &dataProvider,
                                             QVariantList initialStationPath,
                                             QWidget *parent)
        : QPushButton((EveDataProvider::getStationIdFromPath(initialStationPath) != 0) ?
                      (dataProvider.getLocationName(EveDataProvider::getStationIdFromPath(initialStationPath))) :
                      (tr("- any station -")), parent)
        , mDataProvider(dataProvider)
        , mStationPath(std::move(initialStationPath))
    {
        connect(this, &StationSelectButton::clicked, this, &StationSelectButton::selectStation);
    }

    void StationSelectButton::selectStation()
    {
        StationSelectDialog dlg{mDataProvider, true, this};

        dlg.selectPath(mStationPath);
        if (dlg.exec() == QDialog::Accepted)
        {
            const auto id = dlg.getStationId();
            if (id == 0)
            {
                mStationPath.clear();

                setText(tr("- any station -"));
                emit stationChanged({});
            }
            else
            {
                mStationPath = dlg.getSelectedPath();

                setText(mDataProvider.getLocationName(id));
                emit stationChanged(mStationPath);
            }
        }
    }
}
