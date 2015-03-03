/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU IGB Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU IGB Public License for more details.
 *
 *  You should have received a copy of the GNU IGB Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <limits>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>
#include <QSpinBox>
#include <QLabel>

#include "IGBSettings.h"

#include "IGBPreferencesWidget.h"

namespace Evernus
{
    IGBPreferencesWidget::IGBPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{this};

        auto mainGroup = new QGroupBox{this};
        mainLayout->addWidget(mainGroup);

        auto mainGroupLayout = new QFormLayout{mainGroup};

        mEnabledBtn = new QCheckBox{tr("Enabled"), this};
        mainGroupLayout->addRow(mEnabledBtn);
        mEnabledBtn->setChecked(settings.value(IGBSettings::enabledKey, IGBSettings::enabledDefault).toBool());

        mPortEdit = new QSpinBox{this};
        mainGroupLayout->addRow(tr("Port:"), mPortEdit);
        mPortEdit->setMinimum(1);
        mPortEdit->setMaximum(std::numeric_limits<quint16>::max());
        mPortEdit->setValue(settings.value(IGBSettings::portKey, IGBSettings::portDefault).toInt());

        mainLayout->addStretch();
    }

    void IGBPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(IGBSettings::enabledKey, mEnabledBtn->isChecked());
        settings.setValue(IGBSettings::portKey, mPortEdit->value());
    }
}
