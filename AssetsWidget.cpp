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
#include <QSortFilterProxyModel>
#include <QRadioButton>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QSettings>
#include <QAction>
#include <QLocale>
#include <QLabel>
#include <QDebug>

#include "LeafFilterProxyModel.h"
#include "CacheTimerProvider.h"
#include "WarningBarWidget.h"
#include "TextFilterWidget.h"
#include "ButtonWithTimer.h"
#include "StyledTreeView.h"
#include "ImportSettings.h"
#include "AssetProvider.h"
#include "StationView.h"
#include "UISettings.h"
#include "AssetList.h"
#include "TextUtils.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(const AssetProvider &assetProvider,
                               const EveDataProvider &dataProvider,
                               const CacheTimerProvider &cacheTimerProvider,
                               const FilterTextRepository &filterRepo,
                               bool corp,
                               QWidget *parent)
        : CharacterBoundWidget(std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpAssetList) : (TimerType::AssetList)),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpAssetList) : (TimerType::AssetList)),
                               ImportSettings::maxAssetListAgeKey,
                               parent)
        , mAssetProvider(assetProvider)
        , mModel(mAssetProvider, dataProvider, !corp)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import prices from Web"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &AssetsWidget::prepareItemImportFromWeb);

        auto importFromFile = new QPushButton{QIcon{":/images/page_refresh.png"}, tr("Import prices from logs"), this};
        toolBarLayout->addWidget(importFromFile);
        importFromFile->setFlat(true);
        connect(importFromFile, &QPushButton::clicked, this, &AssetsWidget::prepareItemImportFromFile);

        auto expandAll = new QPushButton{QIcon{":/images/arrow_out.png"}, tr("Expand all"), this};
        toolBarLayout->addWidget(expandAll);
        expandAll->setFlat(true);

        auto collapseAll = new QPushButton{QIcon{":/images/arrow_in.png"}, tr("Collapse all"), this};
        toolBarLayout->addWidget(collapseAll);
        collapseAll->setFlat(true);

        auto filterEdit = new TextFilterWidget{filterRepo, this};
        toolBarLayout->addWidget(filterEdit, 1);
        connect(filterEdit, &TextFilterWidget::filterEntered, this, &AssetsWidget::applyWildcard);

        QSettings settings;

        auto combineBtn = new QCheckBox{tr("Combine for all characters"), this};
        toolBarLayout->addWidget(combineBtn);
        combineBtn->setChecked(settings.value(UISettings::combineAssetsKey, UISettings::combineAssetsDefault).toBool());
        connect(combineBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(UISettings::combineAssetsKey, checked);

            mModel.setCombineCharacters(checked);
            resetModel();
        });

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);

        auto assetLayout = new QHBoxLayout{};
        mainLayout->addLayout(assetLayout);

        mModel.setCombineCharacters(combineBtn->isChecked());

        mModelProxy = new LeafFilterProxyModel{this};
        mModelProxy->setSourceModel(&mModel);
        mModelProxy->setSortRole(Qt::UserRole);
        mModelProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        mAssetView = new StyledTreeView{"assetView", this};
        assetLayout->addWidget(mAssetView, 1);
        mAssetView->setModel(mModelProxy);
        connect(mAssetView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &AssetsWidget::handleSelection);
        connect(expandAll, &QPushButton::clicked, mAssetView, &StyledTreeView::expandAll);
        connect(collapseAll, &QPushButton::clicked, mAssetView, &StyledTreeView::collapseAll);

        mSetDestinationAct = new QAction{tr("Set destination in EVE"), this};
        mSetDestinationAct->setEnabled(false);
        mAssetView->addAction(mSetDestinationAct);
        connect(mSetDestinationAct, &QAction::triggered, this, &AssetsWidget::setDestinationForCurrent);

        auto stationGroup = new QGroupBox{tr("Price station"), this};
        assetLayout->addWidget(stationGroup);

        auto stationGroupLayout = new QVBoxLayout{stationGroup};

        const auto useCustomStation
            = settings.value(ImportSettings::useCustomAssetStationKey, ImportSettings::useCustomAssetStationDefault).toBool();

        mUseAssetStationBtn = new QRadioButton{tr("Use asset location"), this};
        stationGroupLayout->addWidget(mUseAssetStationBtn);
        mUseAssetStationBtn->setChecked(!useCustomStation);
        connect(mUseAssetStationBtn, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked)
                setCustomStation(0);
            else
                setCustomStation(mStationView->getStationId());
        });

        auto customStationBtn = new QRadioButton{tr("Use custom station"), this};
        stationGroupLayout->addWidget(customStationBtn);
        customStationBtn->setChecked(useCustomStation);

        mStationView = new StationView{dataProvider, this};
        stationGroupLayout->addWidget(mStationView);
        mStationView->setEnabled(useCustomStation);
        mStationView->setMaximumWidth(260);
        mStationView->selectPath(settings.value(ImportSettings::customAssetStationKey).toList());
        connect(mStationView, &StationView::stationChanged, this, &AssetsWidget::setCustomStation);
        connect(customStationBtn, &QRadioButton::toggled, mStationView, &StationView::setEnabled);

        mInfoLabel = new QLabel{this};
        mainLayout->addWidget(mInfoLabel);

        if (useCustomStation)
            mModel.setCustomStation(mStationView->getStationId());
    }

    void AssetsWidget::updateData()
    {
        refreshImportTimer();
        mModel.reset();
        mAssetView->expandAll();

        setNewInfo();
    }

    void AssetsWidget::prepareItemImportFromWeb()
    {
        emit importPricesFromWeb(getImportTarget());
    }

    void AssetsWidget::prepareItemImportFromFile()
    {
        emit importPricesFromFile(getImportTarget());
    }

    void AssetsWidget::applyWildcard(const QString &text)
    {
        mModelProxy->setFilterWildcard(text);
        mAssetView->expandAll();
    }

    void AssetsWidget::setCustomStation(quint64 id)
    {
        mCustomStationId = id;

        if (mCustomStationId != 0)
        {
            QSettings settings;
            settings.setValue(ImportSettings::customAssetStationKey, mStationView->getSelectedPath());
        }
        else if (!mUseAssetStationBtn->isChecked())
        {
            return;
        }

        mModel.setCustomStation(mCustomStationId);
        mModel.reset();

        mAssetView->expandAll();

        setNewInfo();
    }

    void AssetsWidget::setDestinationForCurrent()
    {
        const auto id = mModel.getAssetLocationId(mModelProxy->mapToSource(mAssetView->currentIndex()));
        emit setDestinationInEve(id);
    }

    void AssetsWidget::handleSelection(const QItemSelection &selected)
    {
        const auto enable = !selected.isEmpty() &&
                            mModel.getAssetLocationId(mModelProxy->mapToSource(mAssetView->currentIndex())) != 0;

        mSetDestinationAct->setEnabled(enable);
    }

    void AssetsWidget::setCombine(int state)
    {
        QSettings settings;
    }

    void AssetsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching assets to" << id;

        mModel.setCharacter(id);
        resetModel();
    }

    void AssetsWidget::resetModel()
    {
        mModel.reset();
        mAssetView->expandAll();
        mAssetView->header()->resizeSections(QHeaderView::ResizeToContents);

        setNewInfo();
    }

    void AssetsWidget::setNewInfo()
    {
        QLocale locale;
        mInfoLabel->setText(QString{"Total assets: <strong>%1</strong> Total volume: <strong>%2m³</strong> Total sell price: <strong>%3</strong>"}
            .arg(locale.toString(mModel.getTotalAssets()))
            .arg(locale.toString(mModel.getTotalVolume(), 'f', 2))
            .arg(TextUtils::currencyToString(mModel.getTotalSellPrice(), locale)));
    }

    ExternalOrderImporter::TypeLocationPairs AssetsWidget::getImportTarget() const
    {
        ExternalOrderImporter::TypeLocationPairs target;

        auto buildImportTargetFromList = [&target, this](const auto &assets) {
            for (const auto &item : *assets)
            {
                if (mCustomStationId == 0)
                {
                    const auto locationId = item->getLocationId();
                    if (!locationId)
                        continue;

                    buildImportTarget(target, *item, *locationId);
                }
                else
                {
                    buildImportTarget(target, *item, mCustomStationId);
                }
            }
        };

        if (mModel.isCombiningCharacters())
        {
            const auto list = mAssetProvider.fetchAllAssets();
            for (const auto &assets : list)
                buildImportTargetFromList(assets);
        }
        else
        {
            buildImportTargetFromList(mAssetProvider.fetchAssetsForCharacter(getCharacterId()));
        }

        return target;
    }

    void AssetsWidget::buildImportTarget(ExternalOrderImporter::TypeLocationPairs &target, const Item &item, quint64 locationId)
    {
        target.insert(std::make_pair(item.getTypeId(), locationId));
        for (const auto &child : item)
            buildImportTarget(target, *child, locationId);
    }
}
