#include <stdexcept>

#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include <QDir>

#include "EvernusApplication.h"

namespace Evernus
{
    EvernusApplication::EvernusApplication(int &argc, char *argv[])
        : QApplication{argc, argv}
        , mMainDb{QSqlDatabase::addDatabase("QSQLITE")}
    {
        setApplicationName("Evernus");
        setApplicationVersion("0.1 BETA");
        setOrganizationDomain("evernus.com");
        setOrganizationName("evernus.com");

        try
        {
            createDb();
            createDbSchema();
        }
        catch (const std::exception &e)
        {
            QMessageBox::critical(nullptr, tr("Error"), e.what());
            throw;
        }
    }

    const KeyRepository &EvernusApplication::getKeyRepository() const noexcept
    {
        return *mKeyRepository;
    }

    const CharacterRepository &EvernusApplication::getCharacterRepository() const noexcept
    {
        return *mCharacterRepository;
    }

    void EvernusApplication::createDb()
    {
        if (!mMainDb.isValid())
            throw std::runtime_error{tr("Error crating DB object!").toStdString()};

        const auto dbPath =
            QStandardPaths::writableLocation(QStandardPaths::DataLocation) +
            QDir::separator() +
            "db" +
            QDir::separator();

        qDebug() << "DB path: " << dbPath;

        if (!QDir{}.mkpath(dbPath))
            throw std::runtime_error{tr("Error creating DB path!").toStdString()};

        mMainDb.setDatabaseName(dbPath + "main.db");
        if (!mMainDb.open())
            throw std::runtime_error{tr("Error opening DB!").toStdString()};

        mKeyRepository.reset(new KeyRepository{mMainDb});
        mCharacterRepository.reset(new CharacterRepository{mMainDb});
    }

    void EvernusApplication::createDbSchema()
    {
        mKeyRepository->create();
        mCharacterRepository->create(*mKeyRepository);
    }
}
