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

#include "IndustryManufacturingSetupModel.h"
#include "IndustryManufacturingSetup.h"
#include "Character.h"

namespace Evernus
{
    class RegionStationPresetRepository;
    class TradeableTypesTreeView;
    class MarketGroupRepository;
    class EveTypeRepository;
    class EveDataProvider;
    class RegionComboBox;

    class IndustryManufacturingWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        IndustryManufacturingWidget(const EveDataProvider &dataProvider,
                                    const RegionStationPresetRepository &regionStationPresetRepository,
                                    const EveTypeRepository &typeRepo,
                                    const MarketGroupRepository &groupRepo,
                                    QWidget *parent = nullptr);
        IndustryManufacturingWidget(const IndustryManufacturingWidget &) = default;
        IndustryManufacturingWidget(IndustryManufacturingWidget &&) = default;
        virtual ~IndustryManufacturingWidget() = default;

        IndustryManufacturingWidget &operator =(const IndustryManufacturingWidget &) = default;
        IndustryManufacturingWidget &operator =(IndustryManufacturingWidget &&) = default;

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void refreshTypes();
        void importData();

        void showSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

    private:
        const EveDataProvider &mDataProvider;

        RegionComboBox *mSourceRegionCombo = nullptr;
        RegionComboBox *mDestRegionCombo = nullptr;

        TradeableTypesTreeView *mTypeView = nullptr;

        quint64 mSrcStation = 0;
        quint64 mDstStation = 0;

        IndustryManufacturingSetup mSetup{mDataProvider};
        IndustryManufacturingSetupModel mSetupModel{mSetup, mDataProvider};

        void changeStation(quint64 &destination, const QVariantList &path, const QString &settingName);
    };
}
