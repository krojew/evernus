#pragma once

#include <QDialog>

namespace Evernus
{
    class CharacterRepository;
    class KeyRepository;

    class CharacterManagerDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        CharacterManagerDialog(const CharacterRepository &characterRepository,
                               const KeyRepository &keyRepository,
                               QWidget *parent = nullptr);
        virtual ~CharacterManagerDialog() = default;

    private slots:
        void addKey();
        void editKey();
        void removeKey();

    private:
        const CharacterRepository &mCharacterRepository;
        const KeyRepository &mKeyRepository;

        static QWidget *createCharacterTab();
        QWidget *createKeyTab();
    };
}
