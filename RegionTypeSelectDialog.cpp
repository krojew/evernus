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
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>

#include "EveDataProvider.h"

#include "RegionTypeSelectDialog.h"

namespace Evernus
{
    RegionTypeSelectDialog::RegionTypeSelectDialog(const EveDataProvider &dataProvider, QWidget *parent)
        : QDialog(parent)
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

        const auto &regions = dataProvider.getRegions();
        for (const auto &region : regions)
        {
            auto item = new QListWidgetItem{region.second, mRegionList};
            item->setData(Qt::UserRole, region.first);
        }

        mRegionList->sortItems();

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

        mTypeList = new QListWidget{this};
        typeLayout->addWidget(mTypeList);
        mTypeList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        const auto &types = dataProvider.getAllTradeableTypeNames();
        for (const auto &type : types)
        {
            auto item = new QListWidgetItem{type.second, mTypeList};
            item->setData(Qt::UserRole, type.first);
        }

        mTypeList->sortItems();

        auto typeBtnsLayout = new QHBoxLayout{};
        typeLayout->addLayout(typeBtnsLayout);

        selectBtn = new QPushButton{tr("Select all"), this};
        typeBtnsLayout->addWidget(selectBtn);
        connect(selectBtn, &QPushButton::clicked, mTypeList, &QListWidget::selectAll);

        selectBtn = new QPushButton{tr("Deselect all"), this};
        typeBtnsLayout->addWidget(selectBtn);
        connect(selectBtn, &QPushButton::clicked, mTypeList, &QListWidget::clearSelection);

        auto buttonBox = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
        mainLayout->addWidget(buttonBox);
        connect(buttonBox, &QDialogButtonBox::accepted, this, [=] {
            ExternalOrderImporter::TypeLocationPairs result;

            const auto regions = mRegionList->selectedItems();
            const auto types = mTypeList->selectedItems();

            for (const auto region : regions)
            {
                for (const auto type : types)
                {
                    result.emplace(type->data(Qt::UserRole).value<ExternalOrderImporter::TypeLocationPair::first_type>(),
                                   region->data(Qt::UserRole).value<ExternalOrderImporter::TypeLocationPair::second_type>());
                }
            }

            emit selected(result);
            accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, this, &RegionTypeSelectDialog::reject);

        setWindowTitle(tr("Select regions and types"));
    }
}
