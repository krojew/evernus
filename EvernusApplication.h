#pragma once

#include <QApplication>

namespace Evernus
{
    class EvernusApplication
        : public QApplication
    {
    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;
    };
}
