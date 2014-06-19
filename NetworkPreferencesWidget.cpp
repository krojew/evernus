#include <QNetworkProxy>
#include <QIntValidator>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QLabel>

#include "NetworkSettings.h"

#include "NetworkPreferencesWidget.h"

namespace Evernus
{
    NetworkPreferencesWidget::NetworkPreferencesWidget(QWidget *parent)
        : QWidget{parent}
        , mCrypt{Q_UINT64_C(0x468c4a0e33a6fe01)}
    {
        QSettings settings;

        const auto useProxy = settings.value(NetworkSettings::useProxyKey).toBool();
        const auto proxyType = settings.value(NetworkSettings::proxyTypeKey).toInt();
        const auto proxyHost = settings.value(NetworkSettings::proxyHostKey).toString();
        const auto proxyPort = settings.value(NetworkSettings::proxyPortKey).toInt();
        const auto proxyUser = settings.value(NetworkSettings::proxyUserKey).toString();
        const auto proxyPassword = mCrypt.decryptToString(settings.value(NetworkSettings::proxyPasswordKey).toString());

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto proxyGroup = new QGroupBox{tr("Proxy"), this};
        mainLayout->addWidget(proxyGroup);

        auto proxyLayout = new QVBoxLayout{};
        proxyGroup->setLayout(proxyLayout);

        mNoProxyBtn = new QRadioButton{tr("No proxy"), this};
        proxyLayout->addWidget(mNoProxyBtn);

        auto customProxyBtn = new QRadioButton{tr("Custom proxy"), this};
        proxyLayout->addWidget(customProxyBtn);

        auto proxySettingsContainerWidget = new QWidget{this};
        proxyLayout->addWidget(proxySettingsContainerWidget);
        connect(customProxyBtn, &QRadioButton::toggled, proxySettingsContainerWidget, &QWidget::setEnabled);

        auto proxySettingLayout = new QFormLayout{};
        proxySettingsContainerWidget->setLayout(proxySettingLayout);

        mProxyTypeCombo = new QComboBox{this};
        proxySettingLayout->addRow(tr("Type:"), mProxyTypeCombo);
        mProxyTypeCombo->addItem(tr("SOCKS5"), QNetworkProxy::Socks5Proxy);
        mProxyTypeCombo->addItem(tr("HTTP"), QNetworkProxy::HttpProxy);

        mProxyHostEdit = new QLineEdit{proxyHost, this};
        proxySettingLayout->addRow(tr("Host:"), mProxyHostEdit);

        auto portValidator = new QIntValidator{this};
        portValidator->setBottom(0);

        mProxyPortEdit = new QLineEdit{QString::number(proxyPort), this};
        proxySettingLayout->addRow(tr("Port:"), mProxyPortEdit);
        mProxyPortEdit->setValidator(portValidator);

        mProxyUserEdit = new QLineEdit{proxyUser, this};
        proxySettingLayout->addRow(tr("User:"), mProxyUserEdit);

        mProxyPasswordEdit = new QLineEdit{proxyPassword, this};
        proxySettingLayout->addRow(tr("Password:"), mProxyPasswordEdit);
        mProxyPasswordEdit->setEchoMode(QLineEdit::Password);

        proxyLayout->addWidget(new QLabel{tr("Warning: password store uses weak encryption - do not use sensitive passwords."), this});

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
    }

    void NetworkPreferencesWidget::applySettings()
    {
        QSettings settings;

        settings.setValue(NetworkSettings::useProxyKey, mNoProxyBtn->isChecked());
        settings.setValue(NetworkSettings::proxyTypeKey, mProxyTypeCombo->itemData(mProxyTypeCombo->currentIndex()).toInt());
        settings.setValue(NetworkSettings::proxyHostKey, mProxyHostEdit->text());
        settings.setValue(NetworkSettings::proxyPortKey, mProxyPortEdit->text());
        settings.setValue(NetworkSettings::proxyUserKey, mProxyUserEdit->text());
        settings.setValue(NetworkSettings::proxyPasswordKey, mCrypt.encryptToString(mProxyPasswordEdit->text()));
    }
}
