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

#include <QHBoxLayout>
#include <QVBoxLayout>
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

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto mainGroup = new QGroupBox{this};
        mainLayout->addWidget(mainGroup);

        auto mainGroupLayout = new QVBoxLayout{};
        mainGroup->setLayout(mainGroupLayout);

        mEnabledBtn = new QCheckBox{tr("Enabled"), this};
        mainGroupLayout->addWidget(mEnabledBtn);
        mEnabledBtn->setChecked(settings.value(IGBSettings::enabledKey, true).toBool());

        auto portLayout = new QHBoxLayout{};
        mainGroupLayout->addLayout(portLayout);

        portLayout->addWidget(new QLabel{tr("Port:"), this});

        mPortEdit = new QSpinBox{this};
        portLayout->addWidget(mPortEdit);
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
