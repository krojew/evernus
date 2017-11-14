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
#include <unordered_set>
#include <memory>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSqlRecord>
#include <QTabWidget>
#include <QGroupBox>
#include <QTreeView>
#include <QLabel>

#include "CharacterRepository.h"
#include "CorpKeyRepository.h"
#include "CorpKeyEditDialog.h"
#include "KeyEditDialog.h"
#include "Repository.h"
#include "APIManager.h"
#include "Key.h"

#include "CharacterManagerDialog.h"

namespace Evernus
{
    CharacterManagerDialog::CharacterManagerDialog(const CharacterRepository &characterRepository,
                                                   const Repository<Key> &keyRepository,
                                                   const CorpKeyRepository &corpKeyRepository,
                                                   QWidget *parent)
        : QDialog{parent}
        , mCharacterRepository{characterRepository}
        , mKeyRepository{keyRepository}
        , mCorpKeyRepository{corpKeyRepository}
        , mCorpKeyModel{nullptr, mCorpKeyRepository.getDatabase()}
        , mCharacterModel{mCharacterRepository}
    {
        auto mainLayout = new QVBoxLayout{this};

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs);
        tabs->addTab(createKeyTab(), tr("Character keys"));
        tabs->addTab(createCorpKeyTab(), tr("Corporation keys"));
        tabs->addTab(createCharacterTab(), tr("Characters"));

        auto btnBox = new QDialogButtonBox{QDialogButtonBox::Close, this};
        mainLayout->addWidget(btnBox);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        connect(&mCharacterModel, &CharacterModel::dataChanged,
                this, &CharacterManagerDialog::charactersChanged);

        mCorpKeyModel.setTable(mCorpKeyRepository.getTableName());
        mCorpKeyModel.setRelation(
            1, QSqlRelation{mCharacterRepository.getTableName(), mCharacterRepository.getIdColumn(), mCharacterRepository.getNameColumn()});
        mCorpKeyModel.setHeaderData(0, Qt::Horizontal, tr("Key ID"));
        mCorpKeyModel.setHeaderData(1, Qt::Horizontal, tr("Character"));
        mCorpKeyModel.setHeaderData(2, Qt::Horizontal, tr("Verification code"));

        setWindowTitle(tr("Character Manager"));

        refreshKeys();
        refreshCorpKeys();
    }

    void CharacterManagerDialog::updateCharacters()
    {
        mCharacterModel.reset();
    }

    void CharacterManagerDialog::addKey()
    {
        Key newKey;
        showEditKeyDialog(newKey);
    }

    void CharacterManagerDialog::editKey()
    {
        Q_ASSERT(mSelectedKeys.count() > 0);

        auto key = mKeyRepository.populate(mKeyModel.record(mSelectedKeys.first().row()));
        showEditKeyDialog(*key);
    }

    void CharacterManagerDialog::removeKey()
    {
        const auto index = mSelectedKeys.first();
        const auto id = mKeyModel.data(mKeyModel.index(index.row(), 0)).value<Key::IdType>();

        mCharacterRepository.disableByKey(id);
        mKeyRepository.remove(id);
        refreshKeys();
        updateCharacters();

        emit charactersChanged();
    }

    void CharacterManagerDialog::addCorpKey()
    {
        CorpKey newKey;
        showEditCorpKeyDialog(newKey);
    }

    void CharacterManagerDialog::editCorpKey()
    {
        Q_ASSERT(mSelectedCorpKeys.count() > 0);

        auto key = mCorpKeyRepository.find(
            mCorpKeyModel.record(mSelectedCorpKeys.first().row()).value(mCorpKeyRepository.getIdColumn()).value<CorpKey::IdType>());
        showEditCorpKeyDialog(*key);
    }

    void CharacterManagerDialog::removeCorpKey()
    {
        const auto index = mSelectedCorpKeys.first();
        const auto id = mCorpKeyModel.data(mCorpKeyModel.index(index.row(), 0)).value<Key::IdType>();

        mCorpKeyRepository.remove(id);
        refreshCorpKeys();
    }

    void CharacterManagerDialog::selectKey(const QItemSelection &selected)
    {
        mSelectedKeys = selected.indexes();

        mEditKeyBtn->setDisabled(mSelectedKeys.isEmpty());
        mRemoveKeyBtn->setDisabled(mSelectedKeys.isEmpty());
    }

    void CharacterManagerDialog::selectCorpKey(const QItemSelection &selected)
    {
        mSelectedCorpKeys = selected.indexes();

        mEditCorpKeyBtn->setDisabled(mSelectedCorpKeys.isEmpty());
        mRemoveCorpKeyBtn->setDisabled(mSelectedCorpKeys.isEmpty());
    }

    void CharacterManagerDialog::removeCharacter()
    {
        Q_ASSERT(!mSelectedCharacters.isEmpty());

        mCharacterModel.removeRow(mCharacterModelProxy.mapToSource(mSelectedCharacters.first()).row());
        mCharacterModelProxy.invalidate();

        emit charactersChanged();
    }

    void CharacterManagerDialog::selectCharacter(const QItemSelection &selected)
    {
        mRemoveCharacterBtn->setEnabled(!selected.isEmpty());
        mSelectedCharacters = selected.indexes();
    }

    void CharacterManagerDialog::refreshKeys()
    {
        mKeyModel.setQuery(QStringLiteral("SELECT id, code FROM %1").arg(mKeyRepository.getTableName()), mKeyRepository.getDatabase());

        mKeyModel.setHeaderData(0, Qt::Horizontal, tr("Key ID"));
        mKeyModel.setHeaderData(1, Qt::Horizontal, tr("Verification code"));

        mEditKeyBtn->setDisabled(true);
        mRemoveKeyBtn->setDisabled(true);
    }

    void CharacterManagerDialog::refreshCorpKeys()
    {
        mCorpKeyModel.select();

        mEditCorpKeyBtn->setDisabled(true);
        mRemoveCorpKeyBtn->setDisabled(true);
    }

    void CharacterManagerDialog::showEditKeyDialog(Key &key)
    {
        KeyEditDialog dlg{key, this};
        if (dlg.exec() == QDialog::Accepted)
        {
            mKeyRepository.store(key);
            refreshKeys();

            emit refreshCharacters();
        }
    }

    void CharacterManagerDialog::showEditCorpKeyDialog(CorpKey &key)
    {
        CorpKeyEditDialog dlg{mCharacterRepository, key, this};
        if (dlg.exec() == QDialog::Accepted)
        {
            mCorpKeyRepository.store(key);
            refreshCorpKeys();
        }
    }

    QWidget *CharacterManagerDialog::createKeyTab()
    {
        auto page = new QWidget{this};

        auto pageLayout = new QVBoxLayout{page};

        auto keyGroup = new QGroupBox{tr("Added keys"), this};
        pageLayout->addWidget(keyGroup);

        auto groupLayout = new QVBoxLayout{keyGroup};

        auto keyView = new QTreeView{this};
        groupLayout->addWidget(keyView);
        keyView->setModel(&mKeyModel);
        connect(keyView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &CharacterManagerDialog::selectKey);

        auto btnLayout = new QHBoxLayout{};
        pageLayout->addLayout(btnLayout);

        auto addBtn = new QPushButton{QIcon{":/images/add.png"}, tr("Add..."), this};
        btnLayout->addWidget(addBtn);
        connect(addBtn, &QPushButton::clicked, this, &CharacterManagerDialog::addKey);

        mEditKeyBtn = new QPushButton{QIcon{":/images/pencil.png"}, tr("Edit..."), this};
        btnLayout->addWidget(mEditKeyBtn);
        mEditKeyBtn->setDisabled(true);
        connect(mEditKeyBtn, &QPushButton::clicked, this, &CharacterManagerDialog::editKey);

        mRemoveKeyBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        btnLayout->addWidget(mRemoveKeyBtn);
        mRemoveKeyBtn->setDisabled(true);
        connect(mRemoveKeyBtn, &QPushButton::clicked, this, &CharacterManagerDialog::removeKey);

        return page;
    }

    QWidget *CharacterManagerDialog::createCorpKeyTab()
    {
        auto page = new QWidget{this};

        auto pageLayout = new QVBoxLayout{page};

        auto keyGroup = new QGroupBox{tr("Added keys"), this};
        pageLayout->addWidget(keyGroup);

        auto groupLayout = new QVBoxLayout{keyGroup};

        auto keyView = new QTreeView{this};
        groupLayout->addWidget(keyView);
        keyView->setModel(&mCorpKeyModel);
        keyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        connect(keyView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &CharacterManagerDialog::selectCorpKey);

        auto btnLayout = new QHBoxLayout{};
        pageLayout->addLayout(btnLayout);

        auto addBtn = new QPushButton{QIcon{":/images/add.png"}, tr("Add..."), this};
        btnLayout->addWidget(addBtn);
        connect(addBtn, &QPushButton::clicked, this, &CharacterManagerDialog::addCorpKey);

        mEditCorpKeyBtn = new QPushButton{QIcon{":/images/pencil.png"}, tr("Edit..."), this};
        btnLayout->addWidget(mEditCorpKeyBtn);
        mEditCorpKeyBtn->setDisabled(true);
        connect(mEditCorpKeyBtn, &QPushButton::clicked, this, &CharacterManagerDialog::editCorpKey);

        mRemoveCorpKeyBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        btnLayout->addWidget(mRemoveCorpKeyBtn);
        mRemoveCorpKeyBtn->setDisabled(true);
        connect(mRemoveCorpKeyBtn, &QPushButton::clicked, this, &CharacterManagerDialog::removeCorpKey);

        return page;
    }

    QWidget *CharacterManagerDialog::createCharacterTab()
    {
        auto page = new QWidget{this};
        auto pageLayout = new QVBoxLayout{page};

        auto charGroup = new QGroupBox{tr("Available characters"), this};
        pageLayout->addWidget(charGroup);

        auto groupLayout = new QVBoxLayout{charGroup};

        mCharacterModelProxy.setSourceModel(&mCharacterModel);

        auto characterView = new QTreeView{this};
        groupLayout->addWidget(characterView);
        characterView->setModel(&mCharacterModelProxy);
        characterView->setMinimumWidth(320);
        characterView->setSortingEnabled(true);
        connect(characterView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &CharacterManagerDialog::selectCharacter);

        pageLayout->addWidget(new QLabel{tr("In order to manage characters, add keys first in the Keys tab."), this});

        auto btnLayout = new QHBoxLayout{};
        pageLayout->addLayout(btnLayout);

        auto refreshCharsBtn = new QPushButton{QIcon{":/images/arrow_refresh.png"}, tr("Refresh"), this};
        btnLayout->addWidget(refreshCharsBtn);
        connect(refreshCharsBtn, &QPushButton::clicked, this, &CharacterManagerDialog::refreshCharacters);

        mRemoveCharacterBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        btnLayout->addWidget(mRemoveCharacterBtn);
        mRemoveCharacterBtn->setDisabled(true);
        connect(mRemoveCharacterBtn, &QPushButton::clicked, this, &CharacterManagerDialog::removeCharacter);

        return page;
    }
}
