#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "EvernusApplication.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    try
    {
        QCoreApplication::setApplicationName("Evernus");
        QCoreApplication::setApplicationVersion("0.1 BETA");
        QCoreApplication::setOrganizationDomain("evernus.com");
        QCoreApplication::setOrganizationName("evernus.com");

        Evernus::EvernusApplication app{argc, argv};

        try
        {
            Evernus::MainWindow mainWnd{app.getCharacterRepository(), app.getKeyRepository(), app.getAPIManager()};
            app.connect(&mainWnd, SIGNAL(refreshCharacters()), SLOT(fetchCharacters()));
            app.connect(&mainWnd, SIGNAL(importCharacter(Character::IdType)), SLOT(refreshCharacter(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importAssets(Character::IdType)), SLOT(refreshAssets(Character::IdType)));
            mainWnd.connect(&app, SIGNAL(taskStarted(quint32, const QString &)), SLOT(addNewTaskInfo(quint32, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStarted(quint32, quint32, const QString &)), SIGNAL(newSubTaskInfoAdded(quint32, quint32, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStatusChanged(quint32, const QString &)), SIGNAL(taskStatusChanged(quint32, const QString &)));
            mainWnd.connect(&app, SIGNAL(apiError(const QString &)), SLOT(showError(const QString &)));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SIGNAL(charactersChanged()));
            mainWnd.connect(&app, SIGNAL(iskChanged()), SLOT(updateStatus()));
            mainWnd.showAsSaved();

            return app.exec();
        }
        catch (const std::exception &e)
        {
            qCritical() << e.what();
            QMessageBox::critical(nullptr, QCoreApplication::translate("main", "Error"), e.what());
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        qCritical() << e.what();

        QApplication tempApp{argc, argv};
        QMessageBox::critical(nullptr, QCoreApplication::translate("main", "Initialization error"), e.what());
        return 1;
    }

    return 0;
}
