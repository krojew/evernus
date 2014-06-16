#include <QApplication>

#include "EvernusApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    Evernus::EvernusApplication app{argc, argv};

    Evernus::MainWindow mainWnd;
    mainWnd.showAsSaved();

    return app.exec();
}
