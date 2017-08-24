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
#include <QQuickWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QGroupBox>
#include <QSettings>
#include <QDebug>
#include <QLabel>

#include "FavoriteLocationsButton.h"
#include "TradeableTypesTreeView.h"
#include "StationSelectButton.h"
#include "IndustrySettings.h"
#include "EveDataProvider.h"
#include "RegionComboBox.h"
#include "FlowLayout.h"

#include "IndustryManufacturingWidget.h"

namespace Evernus
{
    IndustryManufacturingWidget::IndustryManufacturingWidget(const EveDataProvider &dataProvider,
                                                             const RegionStationPresetRepository &regionStationPresetRepository,
                                                             const EveTypeRepository &typeRepo,
                                                             const MarketGroupRepository &groupRepo,
                                                             QWidget *parent)
        : QWidget{parent}
        , mDataProvider{dataProvider}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        const auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import data for current setup "), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &IndustryManufacturingWidget::importData);

        toolBarLayout->addWidget(new QLabel{tr("Source:"), this});

        mSourceRegionCombo = new RegionComboBox{mDataProvider, IndustrySettings::srcManufacturingRegionKey, this};
        toolBarLayout->addWidget(mSourceRegionCombo);

        QSettings settings;

        const auto srcStationPath = settings.value(IndustrySettings::srcManufacturingStationKey).toList();
        mSrcStation = EveDataProvider::getStationIdFromPath(srcStationPath);
        const auto dstStationPath = settings.value(IndustrySettings::dstManufacturingStationKey).toList();
        mDstStation = EveDataProvider::getStationIdFromPath(dstStationPath);

        const auto srcStationBtn = new StationSelectButton{mDataProvider, srcStationPath, this};
        toolBarLayout->addWidget(srcStationBtn);
        connect(srcStationBtn, &StationSelectButton::stationChanged, this, [=](const auto &path) {
            changeStation(mSrcStation, path, IndustrySettings::srcManufacturingStationKey);
        });

        toolBarLayout->addWidget(new QLabel{tr("Destination:"), this});

        mDestRegionCombo = new RegionComboBox{mDataProvider, IndustrySettings::dstManufacturingRegionKey, this};
        toolBarLayout->addWidget(mDestRegionCombo);

        const auto dstStationBtn = new StationSelectButton{mDataProvider, dstStationPath, this};
        toolBarLayout->addWidget(dstStationBtn);
        connect(dstStationBtn, &StationSelectButton::stationChanged, this, [=](const auto &path) {
            changeStation(mDstStation, path, IndustrySettings::dstManufacturingStationKey);
        });

        const auto locationFavBtn = new FavoriteLocationsButton{regionStationPresetRepository, mDataProvider, this};
        toolBarLayout->addWidget(locationFavBtn);
        connect(locationFavBtn, &FavoriteLocationsButton::locationsChosen,
                this, [=](const auto &srcRegions, auto srcStationId, const auto &dstRegions, auto dstStationId) {
            mSourceRegionCombo->setSelectedRegionList(srcRegions);
            mDestRegionCombo->setSelectedRegionList(dstRegions);

            srcStationBtn->setSelectedStationId(srcStationId);
            dstStationBtn->setSelectedStationId(dstStationId);
        });

        const auto contentSplitter = new QSplitter{this};
        mainLayout->addWidget(contentSplitter, 1);

        const auto manufacturingView = new QQuickWidget{QUrl{QStringLiteral("qrc:/qml/IndustryManufacturingView.qml")}, this};
        contentSplitter->addWidget(manufacturingView);
        manufacturingView->setClearColor(Qt::darkGray);
        connect(manufacturingView, &QQuickWidget::sceneGraphError, this, &IndustryManufacturingWidget::showSceneGraphError);

        const auto typesGroup = new QGroupBox{tr("Output"), this};
        contentSplitter->addWidget(typesGroup);

        contentSplitter->setStretchFactor(0, 1);

        const auto typesGroupLayout = new QVBoxLayout{typesGroup};

        mTypeView = new TradeableTypesTreeView{typeRepo, groupRepo, this};
        typesGroupLayout->addWidget(mTypeView);
        connect(mTypeView, &TradeableTypesTreeView::typesChanged, this, &IndustryManufacturingWidget::refreshTypes);

        const auto selectedTypes = settings.value(IndustrySettings::manufacturingTypesKey).toList();
        TradeableTypesTreeView::TypeSet types;

        for (const auto &type : selectedTypes)
            types.emplace(type.value<EveType::IdType>());

        mTypeView->selectTypes(types);
    }

    void IndustryManufacturingWidget::setCharacter(Character::IdType id)
    {
    }

    void IndustryManufacturingWidget::refreshTypes()
    {
        mSetup.setOutputTypes(mTypeView->getSelectedTypes());
    }

    void IndustryManufacturingWidget::importData()
    {
    }

    void IndustryManufacturingWidget::showSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
    {
        qCritical() << "Scene graph error:" << error << message;
        QMessageBox::warning(this, tr("View error"), tr("There was an error initializing the manufacturing view: %1").arg(message));
    }

    void IndustryManufacturingWidget::changeStation(quint64 &destination, const QVariantList &path, const QString &settingName)
    {
        QSettings settings;
        settings.setValue(settingName, path);
    }
}
