#pragma once

#include <QWidget>

class QLineEdit;

namespace Evernus
{
    class PathPreferencesWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit PathPreferencesWidget(QWidget *parent = nullptr);
        virtual ~PathPreferencesWidget() = default;

    public slots:
        void applySettings();

    private slots:
        void browseForFolder();

    private:
        QLineEdit *mMarketLogPathEdit = nullptr;
    };
}
