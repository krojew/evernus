#pragma once

#include <QDialog>

namespace Evernus
{
    template<class T>
    class Repository;
    class Character;
    class Key;

    class CharacterManagerDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        CharacterManagerDialog(const Repository<Character> &characterRepository,
                               const Repository<Key> &keyRepository,
                               QWidget *parent = nullptr);
        virtual ~CharacterManagerDialog() = default;

    private slots:
        void addKey();
        void editKey();
        void removeKey();

    private:
        const Repository<Character> &mCharacterRepository;
        const Repository<Key> &mKeyRepository;

        QWidget *createKeyTab();

        static QWidget *createCharacterTab();
    };
}
