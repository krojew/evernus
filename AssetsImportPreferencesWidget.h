#pragma once

#include <QWidget>

class QCheckBox;

namespace Evernus
{
    class AssetsImportPreferencesWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit AssetsImportPreferencesWidget(QWidget *parent = nullptr);
        virtual ~AssetsImportPreferencesWidget() = default;

    public slots:
        void applySettings();

    private:
        QCheckBox *mImportAssetsBox = nullptr;
    };
}
