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

#include <QSqlQueryModel>
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

    signals:
        void refreshCharacters();
        void charactersChanged();

    public slots:
        void updateCharacters();

    private slots:
        void addKey();
        void editKey();
        void removeKey();

        void selectKey(const QItemSelection &selected, const QItemSelection &deselected);

        void removeCharacter();

        void selectCharacter(const QItemSelection &selected, const QItemSelection &deselected);

    private:
        const Repository<Character> &mCharacterRepository;
        const Repository<Key> &mKeyRepository;

        QSqlQueryModel mKeyModel;
        CharacterModel mCharacterModel;
        QSortFilterProxyModel *mCharacterModelProxy = nullptr;

        QPushButton *mEditKeyBtn = nullptr;
        QPushButton *mRemoveKeyBtn = nullptr;
        QPushButton *mRemoveCharacterBtn = nullptr;

        QModelIndexList mSelectedKeys, mSelectedCharacters;

        void refreshKeys();
        void showEditKeyDialog(Key &key);

        QWidget *createKeyTab();
        QWidget *createCharacterTab();
    };
}
