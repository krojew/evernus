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
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>
#include <QLabel>

#include "LanguageComboBox.h"
#include "UpdaterSettings.h"
#include "UISettings.h"

#include "GeneralPreferencesWidget.h"

namespace Evernus
{
    GeneralPreferencesWidget::GeneralPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto languageGroup = new QGroupBox{this};
        mainLayout->addWidget(languageGroup);

        auto languageGroupLayout = new QVBoxLayout{};
        languageGroup->setLayout(languageGroupLayout);

        auto languageEditLayout = new QHBoxLayout{};
        languageGroupLayout->addLayout(languageEditLayout);

        languageEditLayout->addWidget(new QLabel{tr("Language:"), this});

        mLanguageEdit = new LanguageComboBox{this};
        languageEditLayout->addWidget(mLanguageEdit);

        languageEditLayout->addStretch();

        auto languageInfoLabel = new QLabel{tr("Language changes require application restart."), this};
        languageGroupLayout->addWidget(languageInfoLabel);
        languageInfoLabel->setWordWrap(true);

        auto generalGroup = new QGroupBox{this};
        mainLayout->addWidget(generalGroup);

        auto generalGroupLayout = new QVBoxLayout{};
        generalGroup->setLayout(generalGroupLayout);

        mMinimizeToTrayBtn = new QCheckBox{tr("Minimize to tray"), this};
        generalGroupLayout->addWidget(mMinimizeToTrayBtn);
        mMinimizeToTrayBtn->setChecked(settings.value(UISettings::minimizeToTrayKey, false).toBool());

        mAutoUpdateBtn = new QCheckBox{tr("Check for updates on startup"), this};
        generalGroupLayout->addWidget(mAutoUpdateBtn);
        mAutoUpdateBtn->setChecked(settings.value(UpdaterSettings::autoUpdateKey, true).toBool());

        auto dtFormatLayout = new QHBoxLayout{};
        generalGroupLayout->addLayout(dtFormatLayout);

        dtFormatLayout->addWidget(new QLabel{tr("Date/time format:"), this});

        mDateFormEdit
            = new QLineEdit{settings.value(UISettings::dateTimeFormatKey, locale().dateTimeFormat(QLocale::ShortFormat)).toString(), this};
        dtFormatLayout->addWidget(mDateFormEdit, 1);

        mainLayout->addStretch();
    }

    void GeneralPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(UISettings::languageKey, mLanguageEdit->currentData(Qt::UserRole));
        settings.setValue(UISettings::minimizeToTrayKey, mMinimizeToTrayBtn->isChecked());
        settings.setValue(UpdaterSettings::autoUpdateKey, mAutoUpdateBtn->isChecked());
        settings.setValue(UISettings::dateTimeFormatKey, mDateFormEdit->text());
    }
}
