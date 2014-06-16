#include "EvernusApplication.h"

namespace Evernus
{
    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication{argc, argv}
    {
        QCoreApplication::setApplicationName("Evernus");
        QCoreApplication::setApplicationVersion("0.1 BETA");
        QCoreApplication::setOrganizationDomain("evernus.com");
        QCoreApplication::setOrganizationName("evernus.com");
    }
}
