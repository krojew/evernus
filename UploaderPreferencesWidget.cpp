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

#include "UploaderSettings.h"

#include "UploaderPreferencesWidget.h"

namespace Evernus
{
    UploaderPreferencesWidget::UploaderPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{this};

        QSettings settings;

        auto uploaderGroup = new QGroupBox{this};
        mainLayout->addWidget(uploaderGroup);

        auto uploaderGroupLayout = new QVBoxLayout{uploaderGroup};

        mEnabledBtn = new QCheckBox{tr("Enabled"), this};
        uploaderGroupLayout->addWidget(mEnabledBtn);
        mEnabledBtn->setChecked(settings.value(UploaderSettings::enabledKey, UploaderSettings::enabledDefault).toBool());

        mainLayout->addStretch();
    }

    void UploaderPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(UploaderSettings::enabledKey, mEnabledBtn->isChecked());
    }
}
