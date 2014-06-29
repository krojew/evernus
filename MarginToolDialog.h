#pragma once

#include <QDialog>

class QLabel;

namespace Evernus
{
    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit MarginToolDialog(QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;

    private slots:
        void toggleAlwaysOnTop(int state);

    private:
        QLabel *mMarginLabel = nullptr;
        QLabel *mMarkupLabel = nullptr;

        void setNewWindowFlags(bool alwaysOnTop);
    };
}
