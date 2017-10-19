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
#include "IndustryMiningLedgerWidget.h"

#include "IndustryWidget.h"

namespace Evernus
{
    IndustryWidget::IndustryWidget(const EveDataProvider &dataProvider,
                                   const RegionStationPresetRepository &regionStationPresetRepository,
                                   const EveTypeRepository &typeRepo,
                                   const MarketGroupRepository &groupRepo,
                                   const CharacterRepository &characterRepo,
                                   const IndustryManufacturingSetupRepository &setupRepo,
                                   ESIInterfaceManager &interfaceManager,
                                   TaskManager &taskManager,
                                   const AssetProvider &assetProvider,
                                   const ItemCostProvider &costProvider,
                                   const CacheTimerProvider &cacheTimerProvider,
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
                                                               setupRepo,
                                                               interfaceManager,
                                                               taskManager,
                                                               assetProvider,
                                                               costProvider,
                                                               std::move(clientId),
                                                               std::move(clientSecret),
                                                               this};
        tabs->addTab(mManufacturingWidget, tr("Manufacturing planner"));
        connect(mManufacturingWidget, &IndustryManufacturingWidget::updateExternalOrders,
                this, &IndustryWidget::updateExternalOrders);

        mMiningLedgerWidget = new IndustryMiningLedgerWidget{cacheTimerProvider, this};
        tabs->addTab(mMiningLedgerWidget, tr("Mining ledger"));
        connect(mMiningLedgerWidget, &IndustryMiningLedgerWidget::importFromAPI,
                this, &IndustryWidget::importMiningLedger);
    }

    void IndustryWidget::handleNewPreferences()
    {
        mManufacturingWidget->handleNewPreferences();
    }

    void IndustryWidget::setCharacter(Character::IdType id)
    {
        mManufacturingWidget->setCharacter(id);
        mMiningLedgerWidget->setCharacter(id);
    }

    void IndustryWidget::refreshAssets()
    {
        mManufacturingWidget->refreshAssets();
    }
}
