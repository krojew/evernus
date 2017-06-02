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
#include <QCheckBox>
#include <QSettings>

#include "WalletTransactionRepository.h"
#include "RepositoryProvider.h"
#include "FlowLayout.h"
#include "UISettings.h"

#include "ReportsWidget.h"

namespace Evernus
{
    ReportsWidget::ReportsWidget(const RepositoryProvider &repositoryProvider,
                                 QWidget *parent)
        : QWidget{parent}
        , mTransactionRepository{repositoryProvider.getWalletTransactionRepository()}
        , mCorpTransactionRepository{repositoryProvider.getCorpWalletTransactionRepository()}
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        QSettings settings;

        mCombineBtn = new QCheckBox{tr("Combine for all characters"), this};
        toolBarLayout->addWidget(mCombineBtn);
        mCombineBtn->setChecked(settings.value(UISettings::combineReportsKey, UISettings::combineReportsDefault).toBool());
        connect(mCombineBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(UISettings::combineReportsKey, checked);

            recalculateData();
        });
    }

    void ReportsWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        if (!mCombineBtn->isChecked())
            recalculateData();
    }

    void ReportsWidget::recalculateData()
    {

    }
}
