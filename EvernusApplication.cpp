#include "EvernusApplication.h"

namespace Evernus
{
    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication{argc, argv}
    {
        setApplicationName("Evernus");
        setApplicationVersion("0.1 BETA");
        setOrganizationDomain("evernus.com");
        setOrganizationName("evernus.com");
    }
}
