#pragma once

#include <QSqlQueryModel>
#include <QModelIndex>
#include <QDialog>

class QItemSelection;
class QPushButton;

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

        void selectKey(const QItemSelection &selected, const QItemSelection &deselected);

    private:
        const Repository<Character> &mCharacterRepository;
        const Repository<Key> &mKeyRepository;

        QSqlQueryModel mKeyModel;

        QPushButton *mEditKeyBtn = nullptr;
        QPushButton *mRemoveKeyBtn = nullptr;

        QModelIndexList mSelectedKeys;

        void refreshKeys();
        void showEditKeyDialog(Key &key);

        QWidget *createKeyTab();
        QWidget *createCharacterTab();
    };
}
