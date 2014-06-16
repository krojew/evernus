#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app{argc, argv};

    QCoreApplication::setApplicationName("Evernus");
    QCoreApplication::setApplicationVersion("0.1 BETA");
    QCoreApplication::setOrganizationDomain("evernus.com");
    QCoreApplication::setOrganizationName("evernus.com");

    return app.exec();
}
