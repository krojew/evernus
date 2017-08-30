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

#include <QQuickWindow>
#include <QVariant>
#include <QWidget>
#include <QString>

#include "IndustryManufacturingSetupController.h"
#include "IndustryManufacturingSetupModel.h"
#include "IndustryManufacturingSetup.h"
#include "MarketOrderDataFetcher.h"
#include "TaskConstants.h"
#include "Character.h"

class QByteArray;

namespace Evernus
{
    class RegionStationPresetRepository;
    class TradeableTypesTreeView;
    class MarketGroupRepository;
    class CharacterRepository;
    class EveTypeRepository;
    class EveDataProvider;
    class RegionComboBox;
    class AssetProvider;
    class TaskManager;

    class IndustryManufacturingWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        IndustryManufacturingWidget(const EveDataProvider &dataProvider,
                                    const RegionStationPresetRepository &regionStationPresetRepository,
                                    const EveTypeRepository &typeRepo,
                                    const MarketGroupRepository &groupRepo,
                                    const CharacterRepository &characterRepo,
                                    TaskManager &taskManager,
                                    const AssetProvider &assetProvider,
                                    QByteArray clientId,
                                    QByteArray clientSecret,
                                    QWidget *parent = nullptr);
        IndustryManufacturingWidget(const IndustryManufacturingWidget &) = default;
        IndustryManufacturingWidget(IndustryManufacturingWidget &&) = default;
        virtual ~IndustryManufacturingWidget() = default;

        void refreshAssets();

        IndustryManufacturingWidget &operator =(const IndustryManufacturingWidget &) = default;
        IndustryManufacturingWidget &operator =(IndustryManufacturingWidget &&) = default;

    public slots:
        void handleNewPreferences();
        void setCharacter(Character::IdType id);

    private slots:
        void refreshTypes();
        void importData();

        void showSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

        void updateOrderTask(const QString &text);
        void endOrderTask(const MarketOrderDataFetcher::OrderResultType &orders, const QString &error);

    private:
        const EveDataProvider &mDataProvider;
        TaskManager &mTaskManager;

        RegionComboBox *mSourceRegionCombo = nullptr;
        RegionComboBox *mDestRegionCombo = nullptr;

        TradeableTypesTreeView *mTypeView = nullptr;

        quint64 mSrcStation = 0;
        quint64 mDstStation = 0;

        Character::IdType mCharacterId = Character::invalidId;

        IndustryManufacturingSetup mSetup{mDataProvider};
        IndustryManufacturingSetupModel mSetupModel;
        IndustryManufacturingSetupController mSetupController{mSetupModel};

        MarketOrderDataFetcher mDataFetcher;

        uint mOrderSubtask = TaskConstants::invalidTask;

        void changeStation(quint64 &destination, const QVariantList &path, const QString &settingName);
    };
}
