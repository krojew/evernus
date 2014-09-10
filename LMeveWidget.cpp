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
#include <QLabel>

#include "StyledTreeView.h"

#include "LMeveWidget.h"

namespace Evernus
{
    LMeveWidget::LMeveWidget(const EveDataProvider &dataProvider, QWidget *parent)
        : QWidget(parent)
        , mTaskModel(dataProvider)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto infoLabel = new QLabel{tr("Before synchronizing, enter LMeve url and key in the <a href='#'>Preferences</a>."), this};
        mainLayout->addWidget(infoLabel);
        connect(infoLabel, &QLabel::linkActivated, this, &LMeveWidget::openPreferences);

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs, 1);

        tabs->addTab(createTaskTab(), tr("Tasks"));
    }

    void LMeveWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
    }

    QWidget *LMeveWidget::createTaskTab()
    {
        mTaskProxy.setSourceModel(&mTaskModel);

        auto view = new StyledTreeView{"lmeve-tasks", this};
        view->setModel(&mTaskProxy);

        return view;
    }
}
