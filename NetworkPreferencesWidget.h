#pragma once

#include <QWidget>

#include "SimpleCrypt.h"

class QRadioButton;
class QComboBox;
class QLineEdit;

namespace Evernus
{
    class NetworkPreferencesWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit NetworkPreferencesWidget(QWidget *parent = nullptr);
        virtual ~NetworkPreferencesWidget() = default;

    public slots:
        void applySettings();

    private:
        SimpleCrypt mCrypt;

        QRadioButton *mNoProxyBtn = nullptr;
        QComboBox *mProxyTypeCombo = nullptr;
        QLineEdit *mProxyHostEdit = nullptr;
        QLineEdit *mProxyPortEdit = nullptr;
        QLineEdit *mProxyUserEdit = nullptr;
        QLineEdit *mProxyPasswordEdit = nullptr;
    };
}
