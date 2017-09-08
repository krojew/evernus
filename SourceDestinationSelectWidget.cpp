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
#include <QHBoxLayout>
#include <QLabel>

#include "FavoriteLocationsButton.h"
#include "StationSelectButton.h"

#include "SourceDestinationSelectWidget.h"

namespace Evernus
{
    SourceDestinationSelectWidget::SourceDestinationSelectWidget(const QVariantList &srcStationPath,
                                                                 const QVariantList &dstStationPath,
                                                                 const QString &srcRegionsSettingsKey,
                                                                 const QString &dstRegionsSettingsKey,
                                                                 const EveDataProvider &dataProvider,
                                                                 const RegionStationPresetRepository &regionStationPresetRepository,
                                                                 QWidget *parent)
        : QWidget{parent}
    {
        const auto mainLayout = new QHBoxLayout{this};
        mainLayout->setContentsMargins(0, 0, 0, 0);

        mainLayout->addWidget(new QLabel{tr("Source:"), this});
        mSrcRegionCombo = new RegionComboBox{dataProvider, srcRegionsSettingsKey, this};
        mainLayout->addWidget(mSrcRegionCombo);

        const auto srcStationBtn = new StationSelectButton{dataProvider, srcStationPath, this};
        mainLayout->addWidget(srcStationBtn);
        connect(srcStationBtn, &StationSelectButton::stationChanged,
                this, &SourceDestinationSelectWidget::srcStationChanged);

        mainLayout->addWidget(new QLabel{tr("Destination:"), this});
        mDstRegionCombo = new RegionComboBox{dataProvider, dstRegionsSettingsKey, this};
        mainLayout->addWidget(mDstRegionCombo);

        const auto dstStationBtn = new StationSelectButton{dataProvider, dstStationPath, this};
        mainLayout->addWidget(dstStationBtn);
        connect(dstStationBtn, &StationSelectButton::stationChanged,
                this, &SourceDestinationSelectWidget::dstStationChanged);

        const auto locationFavBtn = new FavoriteLocationsButton{regionStationPresetRepository, dataProvider, this};
        mainLayout->addWidget(locationFavBtn);
        connect(locationFavBtn, &FavoriteLocationsButton::locationsChosen,
                this, [=](const auto &srcRegions, auto srcStationId, const auto &dstRegions, auto dstStationId) {
            mSrcRegionCombo->setSelectedRegionList(srcRegions);
            mDstRegionCombo->setSelectedRegionList(dstRegions);

            srcStationBtn->setSelectedStationId(srcStationId);
            dstStationBtn->setSelectedStationId(dstStationId);
        });
    }

    SourceDestinationSelectWidget::RegionList SourceDestinationSelectWidget::getSrcSelectedRegionList() const
    {
        return mSrcRegionCombo->getSelectedRegionList();
    }

    SourceDestinationSelectWidget::RegionList SourceDestinationSelectWidget::getDstSelectedRegionList() const
    {
        return mDstRegionCombo->getSelectedRegionList();
    }
}
