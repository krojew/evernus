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
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>
#include <QLabel>

#include "ImportSettings.h"

#include "AssetsImportPreferencesWidget.h"

namespace Evernus
{
    AssetsImportPreferencesWidget::AssetsImportPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto importBox = new QGroupBox{tr("Assets import"), this};
        mainLayout->addWidget(importBox);

        auto importBoxLayout = new QVBoxLayout{};
        importBox->setLayout(importBoxLayout);

        mImportAssetsBox = new QCheckBox{tr("Import assets"), this};
        importBoxLayout->addWidget(mImportAssetsBox);
        mImportAssetsBox->setChecked(settings.value(ImportSettings::importAssetsKey, true).toBool());

        mAutoUpdateValueBox = new QCheckBox{tr("Store total asset value on import/price import*"), this};
        importBoxLayout->addWidget(mAutoUpdateValueBox);
        mAutoUpdateValueBox->setChecked(settings.value(ImportSettings::autoUpdateAssetValueKey, true).toBool());

        auto autoUpdateLabel = new QLabel{tr("* Requires full price data to be present. If there is no stored value, try importing prices."), this};
        importBoxLayout->addWidget(autoUpdateLabel);
        autoUpdateLabel->setWordWrap(true);

        mainLayout->addStretch();
    }

    void AssetsImportPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(ImportSettings::importAssetsKey, mImportAssetsBox->isChecked());
        settings.setValue(ImportSettings::autoUpdateAssetValueKey, mAutoUpdateValueBox->isChecked());
    }
}
