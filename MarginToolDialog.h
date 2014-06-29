#pragma once

#include <QDialog>

namespace Evernus
{
    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit MarginToolDialog(QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;
    };
}
