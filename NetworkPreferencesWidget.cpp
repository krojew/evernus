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
#include <limits>

#include <QNetworkProxy>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QSpinBox>
#include <QLabel>

#include "NetworkSettings.h"
#include "CRESTSettings.h"

#include "NetworkPreferencesWidget.h"

namespace Evernus
{
    NetworkPreferencesWidget::NetworkPreferencesWidget(QWidget *parent)
        : QWidget(parent)
        , mCrypt{NetworkSettings::cryptKey}
    {
        QSettings settings;

        const auto useProxy = settings.value(NetworkSettings::useProxyKey, NetworkSettings::useProxyDefault).toBool();
        const auto proxyType = settings.value(NetworkSettings::proxyTypeKey).toInt();
        const auto proxyHost = settings.value(NetworkSettings::proxyHostKey).toString();
        const quint16 proxyPort = settings.value(NetworkSettings::proxyPortKey).toUInt();
        const auto proxyUser = settings.value(NetworkSettings::proxyUserKey).toString();
        const auto proxyPassword = mCrypt.decryptToString(settings.value(NetworkSettings::proxyPasswordKey).toString());

        const auto usemCustomProvider
            = settings.value(NetworkSettings::useCustomProviderKey, NetworkSettings::useCustomProviderDefault).toBool();
        const auto providerHost = settings.value(NetworkSettings::providerHostKey).toString();

        auto mainLayout = new QVBoxLayout{this};

        auto proxyGroup = new QGroupBox{tr("Proxy"), this};
        mainLayout->addWidget(proxyGroup);

        auto proxyLayout = new QVBoxLayout{proxyGroup};

        mNoProxyBtn = new QRadioButton{tr("No proxy"), this};
        proxyLayout->addWidget(mNoProxyBtn);

        auto customProxyBtn = new QRadioButton{tr("Custom proxy"), this};
        proxyLayout->addWidget(customProxyBtn);

        auto proxySettingsContainerWidget = new QWidget{this};
        proxyLayout->addWidget(proxySettingsContainerWidget);
        connect(customProxyBtn, &QRadioButton::toggled, proxySettingsContainerWidget, &QWidget::setEnabled);

        auto proxySettingLayout = new QFormLayout{proxySettingsContainerWidget};

        mProxyTypeCombo = new QComboBox{this};
        proxySettingLayout->addRow(tr("Type:"), mProxyTypeCombo);
        mProxyTypeCombo->addItem(tr("SOCKS5"), QNetworkProxy::Socks5Proxy);
        mProxyTypeCombo->addItem(tr("HTTP"), QNetworkProxy::HttpProxy);

        mProxyHostEdit = new QLineEdit{proxyHost, this};
        proxySettingLayout->addRow(tr("Host:"), mProxyHostEdit);

        mProxyPortEdit = new QSpinBox{this};
        proxySettingLayout->addRow(tr("Port:"), mProxyPortEdit);
        mProxyPortEdit->setMinimum(1);
        mProxyPortEdit->setMaximum(std::numeric_limits<quint16>::max());
        mProxyPortEdit->setValue(proxyPort);

        mProxyUserEdit = new QLineEdit{proxyUser, this};
        proxySettingLayout->addRow(tr("User:"), mProxyUserEdit);

        mProxyPasswordEdit = new QLineEdit{proxyPassword, this};
        proxySettingLayout->addRow(tr("Password:"), mProxyPasswordEdit);
        mProxyPasswordEdit->setEchoMode(QLineEdit::Password);

        auto warningLabel = new QLabel{tr("Warning: password store uses weak encryption - do not use sensitive passwords."), this};
        proxyLayout->addWidget(warningLabel);
        warningLabel->setWordWrap(true);

        auto providerGroup = new QGroupBox{tr("API provider"), this};
        mainLayout->addWidget(providerGroup);

        auto providerLayout = new QVBoxLayout{providerGroup};

        mUseDefaultProviderBtn = new QRadioButton{tr("Use default provider"), this};
        providerLayout->addWidget(mUseDefaultProviderBtn);

        providerLayout->addWidget(new QLabel{NetworkSettings::defaultAPIProvider, this});

        auto customProviderBtn = new QRadioButton{tr("Use custom provider"), this};
        providerLayout->addWidget(customProviderBtn);

        mProviderHostEdit = new QLineEdit{providerHost, this};
        providerLayout->addWidget(mProviderHostEdit);
        connect(customProviderBtn, &QRadioButton::toggled, mProviderHostEdit, &QWidget::setEnabled);

        auto crestGroup = new QGroupBox{tr("CREST"), this};
        mainLayout->addWidget(crestGroup);

        auto crestGroupLayout = new QFormLayout{crestGroup};

        mCRESTThreadsEdit = new QSpinBox{this};
        crestGroupLayout->addRow(tr("Max. threads:"), mCRESTThreadsEdit);
        mCRESTThreadsEdit->setValue(settings.value(CRESTSettings::maxThreadsKey, CRESTSettings::maxThreadsDefault).toUInt());

        auto infoLabel = new QLabel{tr(
            "This value affects the speed of importing data via CREST. "
            "Higher number gives more speed, but too high value can cause the speed to drop and/or create import errors."), this};
        crestGroupLayout->addRow(infoLabel);
        infoLabel->setWordWrap(true);

        mainLayout->addStretch();

        if (useProxy)
        {
            customProxyBtn->setChecked(true);
        }
        else
        {
            mNoProxyBtn->setChecked(true);
            proxySettingsContainerWidget->setDisabled(true);
        }

        if (proxyType == QNetworkProxy::HttpProxy)
            mProxyTypeCombo->setCurrentIndex(1);

        if (usemCustomProvider)
        {
            customProviderBtn->setChecked(true);
        }
        else
        {
            mUseDefaultProviderBtn->setChecked(true);
            mProviderHostEdit->setDisabled(true);
        }
    }

    void NetworkPreferencesWidget::applySettings()
    {
        QSettings settings;

        const auto useProxy = !mNoProxyBtn->isChecked();
        const auto proxyType = mProxyTypeCombo->itemData(mProxyTypeCombo->currentIndex()).toInt();
        const auto proxyHost = mProxyHostEdit->text();
        const quint16 proxyPort = mProxyPortEdit->value();
        const auto proxyUser = mProxyUserEdit->text();
        const auto proxyPassword = mProxyPasswordEdit->text();

        settings.setValue(NetworkSettings::useProxyKey, useProxy);
        settings.setValue(NetworkSettings::proxyTypeKey, proxyType);
        settings.setValue(NetworkSettings::proxyHostKey, proxyHost);
        settings.setValue(NetworkSettings::proxyPortKey, proxyPort);
        settings.setValue(NetworkSettings::proxyUserKey, proxyUser);
        settings.setValue(NetworkSettings::proxyPasswordKey, mCrypt.encryptToString(proxyPassword));

        settings.setValue(NetworkSettings::useCustomProviderKey, !mUseDefaultProviderBtn->isChecked());
        settings.setValue(NetworkSettings::providerHostKey, mProviderHostEdit->text());

        settings.setValue(CRESTSettings::maxThreadsKey, mCRESTThreadsEdit->value());
    }
}
