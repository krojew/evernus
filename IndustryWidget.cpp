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
#include <QVBoxLayout>
#include <QTabWidget>

#include "IndustryManufacturingWidget.h"

#include "IndustryWidget.h"

namespace Evernus
{
    IndustryWidget::IndustryWidget(const EveDataProvider &dataProvider,
                                   const RegionStationPresetRepository &regionStationPresetRepository,
                                   const EveTypeRepository &typeRepo,
                                   const MarketGroupRepository &groupRepo,
                                   const CharacterRepository &characterRepo,
                                   TaskManager &taskManager,
                                   const AssetProvider &assetProvider,
                                   const ItemCostProvider &costProvider,
                                   QByteArray clientId,
                                   QByteArray clientSecret,
                                   QWidget *parent)
        : QWidget{parent}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs);

        mManufacturingWidget = new IndustryManufacturingWidget{dataProvider,
                                                               regionStationPresetRepository,
                                                               typeRepo,
                                                               groupRepo,
                                                               characterRepo,
                                                               taskManager,
                                                               assetProvider,
                                                               costProvider,
                                                               std::move(clientId),
                                                               std::move(clientSecret),
                                                               this};
        tabs->addTab(mManufacturingWidget, tr("Manufacturing analysis"));
        connect(mManufacturingWidget, &IndustryManufacturingWidget::updateExternalOrders,
                this, &IndustryWidget::updateExternalOrders);
    }

    void IndustryWidget::handleNewPreferences()
    {
        mManufacturingWidget->handleNewPreferences();
    }

    void IndustryWidget::setCharacter(Character::IdType id)
    {
        mManufacturingWidget->setCharacter(id);
    }

    void IndustryWidget::refreshAssets()
    {
        mManufacturingWidget->refreshAssets();
    }
}
