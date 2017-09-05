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
#include <QQmlContext>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QQmlEngine>
#include <QComboBox>
#include <QSplitter>
#include <QGroupBox>
#include <QSettings>
#include <QDebug>
#include <QLabel>

#include "CachingNetworkAccessManagerFactory.h"
#include "FavoriteLocationsButton.h"
#include "TradeableTypesTreeView.h"
#include "StationSelectButton.h"
#include "TypeLocationPairs.h"
#include "IndustrySettings.h"
#include "EveDataProvider.h"
#include "RegionComboBox.h"
#include "ImportSettings.h"
#include "SSOMessageBox.h"
#include "IndustryUtils.h"
#include "TaskManager.h"
#include "FlowLayout.h"

#include "IndustryManufacturingWidget.h"

namespace Evernus
{
    IndustryManufacturingWidget::IndustryManufacturingWidget(const EveDataProvider &dataProvider,
                                                             const RegionStationPresetRepository &regionStationPresetRepository,
                                                             const EveTypeRepository &typeRepo,
                                                             const MarketGroupRepository &groupRepo,
                                                             const CharacterRepository &characterRepo,
                                                             TaskManager &taskManager,
                                                             const AssetProvider &assetProvider,
                                                             QByteArray clientId,
                                                             QByteArray clientSecret,
                                                             QWidget *parent)
        : QWidget{parent}
        , mDataProvider{dataProvider}
        , mTaskManager{taskManager}
        , mSetupModel{mSetup, mDataProvider, assetProvider, characterRepo}
        , mDataFetcher{std::move(clientId), std::move(clientSecret), mDataProvider, characterRepo}
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

        toolBarLayout->addWidget(new QLabel{tr("Facility type:"), this});

        const auto rememberSetting = [](const auto combo, const auto &key) {
            QSettings settings;
            settings.setValue(key, combo->currentData().toInt());
        };

        mFacilityTypeCombo = new QComboBox{this};
        toolBarLayout->addWidget(mFacilityTypeCombo);
        mFacilityTypeCombo->addItem(tr("Station"), static_cast<int>(IndustryUtils::FacilityType::Station));
        mFacilityTypeCombo->addItem(tr("Engineering Complex"), static_cast<int>(IndustryUtils::FacilityType::EngineeringComplex));
        mFacilityTypeCombo->addItem(tr("Assembly Array"), static_cast<int>(IndustryUtils::FacilityType::AssemblyArray));
        mFacilityTypeCombo->addItem(tr("Thukker Component Array"), static_cast<int>(IndustryUtils::FacilityType::ThukkerComponentArray));
        mFacilityTypeCombo->addItem(tr("Rapid Assembly Array"), static_cast<int>(IndustryUtils::FacilityType::RapidAssemblyArray));
        mFacilityTypeCombo->setCurrentIndex(mFacilityTypeCombo->findData(
            settings.value(IndustrySettings::manufacturingFacilityTypeKey, IndustrySettings::manufacturingFacilityTypeDefault).toInt()));

        toolBarLayout->addWidget(new QLabel{tr("Structure size:"), this});

        mFacilitySizeCombo = new QComboBox{this};
        toolBarLayout->addWidget(mFacilitySizeCombo);
        mFacilitySizeCombo->addItem(tr("Medium"), static_cast<int>(IndustryUtils::Size::Medium));
        mFacilitySizeCombo->addItem(tr("Large"), static_cast<int>(IndustryUtils::Size::Large));
        mFacilitySizeCombo->addItem(tr("X-Large"), static_cast<int>(IndustryUtils::Size::XLarge));
        mFacilitySizeCombo->setCurrentIndex(mFacilitySizeCombo->findData(
            settings.value(IndustrySettings::manufacturingFacilitySizeKey, IndustrySettings::manufacturingFacilitySizeDefault).toInt()));
        connect(mFacilitySizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] {
            rememberSetting(mFacilitySizeCombo, IndustrySettings::manufacturingFacilitySizeKey);
            mSetupModel.setFacilitySize(static_cast<IndustryUtils::Size>(mFacilitySizeCombo->currentData().toInt()));
        });

        toolBarLayout->addWidget(new QLabel{tr("Security status:"), this});

        mSecurityStatusCombo = new QComboBox{this};
        toolBarLayout->addWidget(mSecurityStatusCombo);
        mSecurityStatusCombo->addItem(tr("High sec"), static_cast<int>(IndustryUtils::SecurityStatus::HighSec));
        mSecurityStatusCombo->addItem(tr("Low sec"), static_cast<int>(IndustryUtils::SecurityStatus::LowSec));
        mSecurityStatusCombo->addItem(tr("Null sec/WH"), static_cast<int>(IndustryUtils::SecurityStatus::NullSecWH));
        mSecurityStatusCombo->setCurrentIndex(mSecurityStatusCombo->findData(
            settings.value(IndustrySettings::manufacturingSecurityStatusKey, IndustrySettings::manufacturingSecurityStatusDefault).toInt()));
        connect(mSecurityStatusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] {
            rememberSetting(mSecurityStatusCombo, IndustrySettings::manufacturingSecurityStatusKey);
            mSetupModel.setSecurityStatus(static_cast<IndustryUtils::SecurityStatus>(mSecurityStatusCombo->currentData().toInt()));
        });

        toolBarLayout->addWidget(new QLabel{tr("Material rig:"), this});

        mMaterialRigCombo = new QComboBox{this};
        toolBarLayout->addWidget(mMaterialRigCombo);
        mMaterialRigCombo->addItem(tr("None"), static_cast<int>(IndustryUtils::RigType::None));
        mMaterialRigCombo->addItem(tr("T1"), static_cast<int>(IndustryUtils::RigType::T1));
        mMaterialRigCombo->addItem(tr("T2"), static_cast<int>(IndustryUtils::RigType::T2));
        mMaterialRigCombo->setCurrentIndex(mMaterialRigCombo->findData(
            settings.value(IndustrySettings::manufacturingMaterialRigKey, IndustrySettings::manufacturingMaterialRigDefault).toInt()));
        connect(mMaterialRigCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] {
            rememberSetting(mMaterialRigCombo, IndustrySettings::manufacturingMaterialRigKey);
            mSetupModel.setMaterialRigType(static_cast<IndustryUtils::RigType>(mMaterialRigCombo->currentData().toInt()));
        });

        toolBarLayout->addWidget(new QLabel{tr("Time rig:"), this});

        mTimeRigCombo = new QComboBox{this};
        toolBarLayout->addWidget(mTimeRigCombo);
        mTimeRigCombo->addItem(tr("None"), static_cast<int>(IndustryUtils::RigType::None));
        mTimeRigCombo->addItem(tr("T1"), static_cast<int>(IndustryUtils::RigType::T1));
        mTimeRigCombo->addItem(tr("T2"), static_cast<int>(IndustryUtils::RigType::T2));
        mTimeRigCombo->setCurrentIndex(mTimeRigCombo->findData(
            settings.value(IndustrySettings::manufacturingTimeRigKey, IndustrySettings::manufacturingTimeRigDefault).toInt()));
        connect(mTimeRigCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] {
            rememberSetting(mTimeRigCombo, IndustrySettings::manufacturingTimeRigKey);
            mSetupModel.setTimeRigType(static_cast<IndustryUtils::RigType>(mTimeRigCombo->currentData().toInt()));
        });

        connect(mFacilityTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] {
            rememberSetting(mFacilityTypeCombo, IndustrySettings::manufacturingFacilityTypeKey);
            mSetupModel.setFacilityType(static_cast<IndustryUtils::FacilityType>(mFacilityTypeCombo->currentData().toInt()));

            toggleFacilityCombos();
        });

        toggleFacilityCombos();

        mSetupModel.setFacilityType(static_cast<IndustryUtils::FacilityType>(mFacilityTypeCombo->currentData().toInt()));
        mSetupModel.setFacilitySize(static_cast<IndustryUtils::Size>(mFacilitySizeCombo->currentData().toInt()));
        mSetupModel.setSecurityStatus(static_cast<IndustryUtils::SecurityStatus>(mSecurityStatusCombo->currentData().toInt()));
        mSetupModel.setMaterialRigType(static_cast<IndustryUtils::RigType>(mMaterialRigCombo->currentData().toInt()));
        mSetupModel.setTimeRigType(static_cast<IndustryUtils::RigType>(mTimeRigCombo->currentData().toInt()));

        const auto contentSplitter = new QSplitter{this};
        mainLayout->addWidget(contentSplitter, 1);

        const auto manufacturingView = new QQuickWidget{this};
        contentSplitter->addWidget(manufacturingView);
        manufacturingView->setClearColor(QColor{40, 40, 40});
        manufacturingView->setResizeMode(QQuickWidget::SizeRootObjectToView);
        manufacturingView->engine()->setNetworkAccessManagerFactory(new CachingNetworkAccessManagerFactory{this});
        connect(manufacturingView, &QQuickWidget::sceneGraphError, this, &IndustryManufacturingWidget::showSceneGraphError);

        const auto ctxt = manufacturingView->rootContext();
        Q_ASSERT(ctxt != nullptr);

        ctxt->setContextProperty(QStringLiteral("setupModel"), &mSetupModel);
        ctxt->setContextProperty(QStringLiteral("SetupController"), &mSetupController);

        manufacturingView->setSource(QUrl{QStringLiteral("qrc:/qml/Industry/Manufacturing/View.qml")});

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

        connect(&mDataFetcher, &MarketOrderDataFetcher::orderStatusUpdated,
                this, &IndustryManufacturingWidget::updateOrderTask);
        connect(&mDataFetcher, &MarketOrderDataFetcher::orderImportEnded,
                this, &IndustryManufacturingWidget::endOrderTask);
        connect(&mDataFetcher, &MarketOrderDataFetcher::genericError,
                this, [=](const auto &text) {
            SSOMessageBox::showMessage(text, this);
        });
    }

    void IndustryManufacturingWidget::refreshAssets()
    {
        mSetupModel.refreshAssets();
    }

    void IndustryManufacturingWidget::handleNewPreferences()
    {
        mDataFetcher.handleNewPreferences();
    }

    void IndustryManufacturingWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        mSetup.clear();
        mSetupModel.setCharacter(mCharacterId);
        mTypeView->deselectAll();
    }

    void IndustryManufacturingWidget::refreshTypes()
    {
        mSetup.setOutputTypes(mTypeView->getSelectedTypes());
        mSetupModel.refreshData();
    }

    void IndustryManufacturingWidget::importData()
    {
        const auto types = mSetup.getAllTypes();

        auto regions = mSourceRegionCombo->getSelectedRegionList();
        const auto dstRegions = mDestRegionCombo->getSelectedRegionList();

        regions.insert(std::begin(dstRegions), std::end(dstRegions));

        TypeLocationPairs pairs;
        for (const auto type : types)
        {
            for (const auto region : regions)
                pairs.emplace(std::make_pair(type, region));
        }

        if (!mDataFetcher.hasPendingOrderRequests())
        {
            QSettings settings;
            const auto webImporter = static_cast<ImportSettings::WebImporterType>(
                settings.value(ImportSettings::webImportTypeKey, static_cast<int>(ImportSettings::webImportTypeDefault)).toInt());

            const auto mainTask = mTaskManager.startTask(tr("Importing data..."));
            const auto infoText = (webImporter == ImportSettings::WebImporterType::EveCentral) ?
                                  (tr("Making %1 Eve-Central order requests...")) :
                                  (tr("Making %1 ESI order requests..."));

            mOrderSubtask = mTaskManager.startTask(mainTask, infoText.arg(pairs.size()));
        }

        mDataFetcher.importData(pairs, mCharacterId);
    }

    void IndustryManufacturingWidget::showSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
    {
        qCritical() << "Scene graph error:" << error << message;
        QMessageBox::warning(this, tr("View error"), tr("There was an error initializing the manufacturing view: %1").arg(message));
    }

    void IndustryManufacturingWidget::updateOrderTask(const QString &text)
    {
        mTaskManager.updateTask(mOrderSubtask, text);
    }

    void IndustryManufacturingWidget::endOrderTask(const MarketOrderDataFetcher::OrderResultType &orders, const QString &error)
    {
        Q_ASSERT(orders);

        if (error.isEmpty())
        {
            // TODO: process
        }

        mTaskManager.endTask(mOrderSubtask, error);
    }

    void IndustryManufacturingWidget::changeStation(quint64 &destination, const QVariantList &path, const QString &settingName)
    {
        QSettings settings;
        settings.setValue(settingName, path);
    }

    void IndustryManufacturingWidget::toggleFacilityCombos()
    {
        const auto combosEnabled = shouldEnableFacilityCombos();
        mMaterialRigCombo->setEnabled(combosEnabled);
        mTimeRigCombo->setEnabled(combosEnabled);
        mFacilitySizeCombo->setEnabled(combosEnabled);
        mSecurityStatusCombo->setEnabled(combosEnabled);
    }

    bool IndustryManufacturingWidget::shouldEnableFacilityCombos() const
    {
        return static_cast<IndustryUtils::FacilityType>(mFacilityTypeCombo->currentData().toInt()) == IndustryUtils::FacilityType::EngineeringComplex;
    }
}
