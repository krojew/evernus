#pragma once

#include <memory>

#include <QApplication>
#include <QSqlDatabase>

#include "CharacterRepository.h"

namespace Evernus
{
    class EvernusApplication
        : public QApplication
    {
    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

    private:
        QSqlDatabase mMainDb;

        std::unique_ptr<CharacterRepository> mCharacterRepository;

        void createDb();
        void createDbSchema();
    };
}
