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

#include "CitadelLocationWidget.h"

#include "CitadelManagerDialog.h"

namespace Evernus
{
    CitadelManagerDialog::CitadelManagerDialog(const EveDataProvider &dataProvider, QWidget *parent)
        : QDialog{parent}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto locationWidget = new CitadelLocationWidget{dataProvider, this};
        mainLayout->addWidget(locationWidget);
        connect(this, &CitadelManagerDialog::citadelsChanged, locationWidget, &CitadelLocationWidget::refresh);

        const auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &CitadelManagerDialog::applyChanges);
        connect(buttons, &QDialogButtonBox::rejected, this, &CitadelManagerDialog::reject);

        setWindowTitle(tr("Citadel manager"));
    }

    void CitadelManagerDialog::applyChanges()
    {
        accept();
    }
}
