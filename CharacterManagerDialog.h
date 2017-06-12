/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QSqlRelationalTableModel>
#include <QModelIndex>
#include <QDialog>

#include "CharacterModel.h"

class QSortFilterProxyModel;
class QItemSelection;
class QPushButton;

namespace Evernus
{
    template<class T>
    class Repository;
    class CharacterRepository;
    class CorpKeyRepository;
    class Character;
    class CorpKey;
    class Key;

    class CharacterManagerDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        CharacterManagerDialog(const CharacterRepository &characterRepository,
                               const Repository<Key> &keyRepository,
                               const CorpKeyRepository &corpKeyRepository,
                               QWidget *parent = nullptr);
        virtual ~CharacterManagerDialog() = default;

    signals:
        void refreshCharacters();
        void charactersChanged();

    public slots:
        void updateCharacters();

    private slots:
        void addKey();
        void editKey();
        void removeKey();

        void addCorpKey();
        void editCorpKey();
        void removeCorpKey();

        void selectKey(const QItemSelection &selected);
        void selectCorpKey(const QItemSelection &selected);

        void removeCharacter();

        void selectCharacter(const QItemSelection &selected);

    private:
        const CharacterRepository &mCharacterRepository;
        const Repository<Key> &mKeyRepository;
        const CorpKeyRepository &mCorpKeyRepository;

        QSqlQueryModel mKeyModel;
        QSqlRelationalTableModel mCorpKeyModel;
        CharacterModel mCharacterModel;
        QSortFilterProxyModel *mCharacterModelProxy = nullptr;

        QPushButton *mEditKeyBtn = nullptr;
        QPushButton *mRemoveKeyBtn = nullptr;
        QPushButton *mEditCorpKeyBtn = nullptr;
        QPushButton *mRemoveCorpKeyBtn = nullptr;
        QPushButton *mRemoveCharacterBtn = nullptr;

        QModelIndexList mSelectedKeys, mSelectedCorpKeys, mSelectedCharacters;

        void refreshKeys();
        void refreshCorpKeys();
        void showEditKeyDialog(Key &key);
        void showEditCorpKeyDialog(CorpKey &key);

        QWidget *createKeyTab();
        QWidget *createCorpKeyTab();
        QWidget *createCharacterTab();
    };
}
