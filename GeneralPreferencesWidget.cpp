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
#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
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

        auto mainLayout = new QVBoxLayout{this};

        auto languageGroup = new QGroupBox{this};
        mainLayout->addWidget(languageGroup);

        auto languageGroupLayout = new QVBoxLayout{languageGroup};

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

        auto generalGroupLayout = new QVBoxLayout{generalGroup};

        mMinimizeToTrayBtn = new QCheckBox{tr("Minimize to tray"), this};
        generalGroupLayout->addWidget(mMinimizeToTrayBtn);
        mMinimizeToTrayBtn->setChecked(settings.value(UISettings::minimizeToTrayKey, UISettings::minimizeToTrayDefault).toBool());

        mMinimizeByMarginToolBtn = new QCheckBox{tr("Minimize when opening the Margin Tool"), this};
        generalGroupLayout->addWidget(mMinimizeByMarginToolBtn);
        mMinimizeByMarginToolBtn->setChecked(settings.value(UISettings::minimizeByMarginToolKey, UISettings::minimizeByMarginToolDefault).toBool());

        mAutoUpdateBtn = new QCheckBox{tr("Check for updates on startup"), this};
        generalGroupLayout->addWidget(mAutoUpdateBtn);
        mAutoUpdateBtn->setChecked(settings.value(UpdaterSettings::autoUpdateKey, UpdaterSettings::autoUpdateDefault).toBool());

        mUsePackagedVolumeBtn = new QCheckBox{tr("Use packaged size for ships"), this};
        generalGroupLayout->addWidget(mUsePackagedVolumeBtn);
        mUsePackagedVolumeBtn->setChecked(settings.value(UISettings::usePackagedVolumeKey, UISettings::usePackagedVolumeDefault).toBool());

        mOmitCurrencySymbolBtn = new QCheckBox{tr("Omit currency symbol (requires restart)"), this};
        generalGroupLayout->addWidget(mOmitCurrencySymbolBtn);
        mOmitCurrencySymbolBtn->setChecked(settings.value(UISettings::omitCurrencySymbolKey, UISettings::omitCurrencySymbolDefault).toBool());

        mUseUTCDatesBtn = new QCheckBox{tr("Force UTC date/time (requires restart)"), this};
        generalGroupLayout->addWidget(mUseUTCDatesBtn);
        mUseUTCDatesBtn->setChecked(settings.value(UISettings::useUTCDatesKey, UISettings::useUTCDatesDefault).toBool());

        auto uiLayout = new QFormLayout{};
        generalGroupLayout->addLayout(uiLayout);

        mDateFormEdit
            = new QLineEdit{settings.value(UISettings::dateTimeFormatKey, locale().dateTimeFormat(QLocale::ShortFormat)).toString(), this};
        uiLayout->addRow(tr("Date/time format (requires restart):"), mDateFormEdit);

        mApplyDateFormatToGraphsBtn = new QCheckBox{tr("Apply date fromat to graphs (requires restart)"), this};
        uiLayout->addRow(mApplyDateFormatToGraphsBtn);
        mApplyDateFormatToGraphsBtn->setChecked(
            settings.value(UISettings::applyDateFormatToGraphsKey, UISettings::applyDateFormatToGraphsDefault).toBool());

        mColumnDelimiterEdit = new QComboBox{this};
        mColumnDelimiterEdit->addItem(tr("Tab"), '\t');
        mColumnDelimiterEdit->addItem(tr("Space"), ' ');
        mColumnDelimiterEdit->addItem(tr(";"), ';');
        mColumnDelimiterEdit->addItem(tr(","), ',');
        mColumnDelimiterEdit->setCurrentIndex(mColumnDelimiterEdit->findData(
            settings.value(UISettings::columnDelimiterKey, UISettings::columnDelimiterDefault).value<char>()));
        uiLayout->addRow(tr("Column data delimiter:"), mColumnDelimiterEdit);

        mainLayout->addStretch();
    }

    void GeneralPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(UISettings::languageKey, mLanguageEdit->currentData(Qt::UserRole));
        settings.setValue(UISettings::minimizeToTrayKey, mMinimizeToTrayBtn->isChecked());
        settings.setValue(UISettings::minimizeByMarginToolKey, mMinimizeByMarginToolBtn->isChecked());
        settings.setValue(UpdaterSettings::autoUpdateKey, mAutoUpdateBtn->isChecked());
        settings.setValue(UISettings::usePackagedVolumeKey, mUsePackagedVolumeBtn->isChecked());
        settings.setValue(UISettings::omitCurrencySymbolKey, mOmitCurrencySymbolBtn->isChecked());
        settings.setValue(UISettings::useUTCDatesKey, mUseUTCDatesBtn->isChecked());
        settings.setValue(UISettings::dateTimeFormatKey, mDateFormEdit->text());
        settings.setValue(UISettings::applyDateFormatToGraphsKey, mApplyDateFormatToGraphsBtn->isChecked());
        settings.setValue(UISettings::columnDelimiterKey, mColumnDelimiterEdit->currentData().value<char>());
    }
}
