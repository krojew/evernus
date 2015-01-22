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
#include <QRegularExpression>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QTreeView>
#include <QSettings>

#include "EveDataProvider.h"

#include "RegionTypeSelectDialog.h"

namespace Evernus
{
    const QString RegionTypeSelectDialog::settingsRegionsKey = "regionTypeSelect/regions";
    const QString RegionTypeSelectDialog::settingsTypesKey = "regionTypeSelect/types";

    RegionTypeSelectDialog::RegionTypeSelectDialog(const EveDataProvider &dataProvider,
                                                   const EveTypeRepository &typeRepo,
                                                   const MarketGroupRepository &groupRepo,
                                                   QWidget *parent)
        : QDialog(parent)
        , mTypeModel(typeRepo, groupRepo)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto listsLayout = new QHBoxLayout{};
        mainLayout->addLayout(listsLayout);

        auto regionGroup = new QGroupBox{tr("Regions"), this};
        listsLayout->addWidget(regionGroup);

        auto regionLayout = new QVBoxLayout{regionGroup};

        mRegionList = new QListWidget{this};
        regionLayout->addWidget(mRegionList);
        mRegionList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        QSettings settings;
        const auto selectedRegions = settings.value(settingsRegionsKey).toList();

        const auto &regions = dataProvider.getRegions();
        for (const auto &region : regions)
        {
            auto item = new QListWidgetItem{region.second, mRegionList};
            item->setData(Qt::UserRole, region.first);

            if (selectedRegions.contains(region.first))
                item->setSelected(true);
        }

        auto regionBtnsLayout = new QGridLayout{};
        regionLayout->addLayout(regionBtnsLayout);

        auto selectBtn = new QPushButton{tr("Select all"), this};
        regionBtnsLayout->addWidget(selectBtn, 0, 0);
        connect(selectBtn, &QPushButton::clicked, mRegionList, &QListWidget::selectAll);

        selectBtn = new QPushButton{tr("Deselect all"), this};
        regionBtnsLayout->addWidget(selectBtn, 0, 1);
        connect(selectBtn, &QPushButton::clicked, mRegionList, &QListWidget::clearSelection);

        selectBtn = new QPushButton{tr("Select without wormholes"), this};
        regionBtnsLayout->addWidget(selectBtn, 1, 0, 1, 2);
        connect(selectBtn, &QPushButton::clicked, this, [=] {
            QRegularExpression re{"^[A-Z]-R\\d+$"};

            const auto count = mRegionList->count();
            for (auto i = 0; i < count; ++i)
            {
                auto item = mRegionList->item(i);
                item->setSelected(!re.match(item->text()).hasMatch());
            }
        });

        auto typesGroup = new QGroupBox{tr("Types"), this};
        listsLayout->addWidget(typesGroup);

        auto typeLayout = new QVBoxLayout{typesGroup};

        mTypeProxy.setSourceModel(&mTypeModel);
        mTypeProxy.sort(0);

        mTypeView = new QTreeView{this};
        typeLayout->addWidget(mTypeView);
        mTypeView->setHeaderHidden(true);
        mTypeView->setModel(&mTypeProxy);

        auto buttonBox = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
        mainLayout->addWidget(buttonBox);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &RegionTypeSelectDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &RegionTypeSelectDialog::reject);

        setWindowTitle(tr("Select regions and types"));
    }

    void RegionTypeSelectDialog::accept()
    {
        ExternalOrderImporter::TypeLocationPairs result;

        const auto regions = mRegionList->selectedItems();
        const auto types = mTypeModel.getSelectedTypes();

        QVariantList selectedRegions;

        for (const auto region : regions)
        {
            const auto regionId = region->data(Qt::UserRole).value<ExternalOrderImporter::TypeLocationPair::second_type>();
            for (const auto type : types)
                result.emplace(type, regionId);

            selectedRegions << regionId;
        }

        if (result.empty())
        {
            QMessageBox::information(this, tr("Order import"), tr("Please select at least one region and type."));
            return;
        }

        QSettings settings;
        settings.setValue(settingsRegionsKey, selectedRegions);

        emit selected(result);
        QDialog::accept();
    }
}
