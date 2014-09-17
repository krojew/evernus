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
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>

#include "EveDataProvider.h"

#include "ItemTypeSelectDialog.h"

namespace Evernus
{
    ItemTypeSelectDialog::ItemTypeSelectDialog(const EveDataProvider &dataProvider, QWidget *parent)
        : QDialog(parent)
    {
        auto mainLayout = new QVBoxLayout{this};

        mTypeCombo = new QComboBox{this};
        mainLayout->addWidget(mTypeCombo);

        const auto &types = dataProvider.getAllTypeNames();
        for (const auto &type : types)
            mTypeCombo->addItem(type.second, type.first);

        mTypeCombo->setEditable(true);
        mTypeCombo->setInsertPolicy(QComboBox::NoInsert);
        mTypeCombo->model()->sort(0);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setWindowTitle(tr("Select Item Type"));
    }

    EveType::IdType ItemTypeSelectDialog::getSelectedType() const
    {
        return mTypeCombo->currentData().value<EveType::IdType>();
    }
}
