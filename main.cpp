#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "EveMarketDataItemPriceImporter.h"
#include "ItemPriceImporterNames.h"
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

        app.registerImporter(Evernus::ItemPriceImporterNames::webImporter,
                             std::make_unique<Evernus::EveMarketDataItemPriceImporter>());

        try
        {
            Evernus::MainWindow mainWnd{app.getCharacterRepository(),
                                        app.getKeyRepository(),
                                        app,
                                        app,
                                        app.getAPIManager()};

            app.connect(&mainWnd, SIGNAL(refreshCharacters()), SLOT(refreshCharacters()));
            app.connect(&mainWnd, SIGNAL(refreshConquerableStations()), SLOT(refreshConquerableStations()));
            app.connect(&mainWnd, SIGNAL(importCharacter(Character::IdType)), SLOT(refreshCharacter(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importAssets(Character::IdType)), SLOT(refreshAssets(Character::IdType)));
            app.connect(&mainWnd, SIGNAL(importItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &)), SLOT(refreshItemPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &)));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, const QString &)), SLOT(addNewTaskInfo(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStarted(uint, uint, const QString &)), SIGNAL(newSubTaskInfoAdded(uint, uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(taskStatusChanged(uint, const QString &)), SIGNAL(taskStatusChanged(uint, const QString &)));
            mainWnd.connect(&app, SIGNAL(apiError(const QString &)), SLOT(showError(const QString &)));
            mainWnd.connect(&app, SIGNAL(conquerableStationsChanged()), SIGNAL(conquerableStationsChanged()));
            mainWnd.connect(&app, SIGNAL(charactersChanged()), SIGNAL(charactersChanged()));
            mainWnd.connect(&app, SIGNAL(assetsChanged()), SIGNAL(assetsChanged()));
            mainWnd.connect(&app, SIGNAL(itemPricesChanged()), SIGNAL(itemPricesChanged()));
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
