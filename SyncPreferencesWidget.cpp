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

#include "SyncSettings.h"

#include "SyncPreferencesWidget.h"

namespace Evernus
{
    SyncPreferencesWidget::SyncPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

#ifdef EVERNUS_DROPBOX_ENABLED
        QSettings settings;

        auto dropboxGroup = new QGroupBox{this};
        mainLayout->addWidget(dropboxGroup);

        auto dropboxGroupLayout = new QVBoxLayout{};
        dropboxGroup->setLayout(dropboxGroupLayout);

        mEnabledBtn = new QCheckBox{tr("Enabled"), this};
        dropboxGroupLayout->addWidget(mEnabledBtn);
        mEnabledBtn->setChecked(settings.value(SyncSettings::enabledKey, SyncSettings::enabledDefault).toBool());
#else
        auto info = new QLabel{tr(
            "Evernus was compiled without Dropbox app key and secret values. In order to enable Dropbox support, register "
            "a new app key with Dropbox and recompile Evernus with EVERNUS_DROPBOX_APP_KEY and EVERNUS_DROPBOX_APP_SECRET "
            "defined."
        ), this};
        info->setWordWrap(true);

        mainLayout->addWidget(info);
#endif

        mainLayout->addStretch();
    }

    void SyncPreferencesWidget::applySettings()
    {
#ifdef EVERNUS_DROPBOX_ENABLED
        QSettings settings;
        settings.setValue(SyncSettings::enabledKey, mEnabledBtn->isChecked());
#endif
    }
}
