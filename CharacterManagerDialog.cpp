#include <unordered_set>
#include <memory>

#include <QSortFilterProxyModel>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSqlRecord>
#include <QTabWidget>
#include <QGroupBox>
#include <QTreeView>

#include "KeyEditDialog.h"
#include "Repository.h"
#include "APIManager.h"
#include "Key.h"

#include "CharacterManagerDialog.h"

namespace Evernus
{
    CharacterManagerDialog::CharacterManagerDialog(const Repository<Character> &characterRepository,
                                                   const Repository<Key> &keyRepository,
                                                   QWidget *parent)
        : QDialog{parent}
        , mCharacterRepository{characterRepository}
        , mKeyRepository{keyRepository}
        , mCharacterModel{mCharacterRepository}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs);
        tabs->addTab(createCharacterTab(), tr("Characters"));
        tabs->addTab(createKeyTab(), tr("Keys"));

        auto btnBox = new QDialogButtonBox{QDialogButtonBox::Close, this};
        mainLayout->addWidget(btnBox);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setWindowTitle(tr("Character Manager"));

        refreshKeys();
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
        showEditKeyDialog(key);
    }

    void CharacterManagerDialog::removeKey()
    {
        const auto index = mSelectedKeys.first();
        const auto id = mKeyModel.data(mKeyModel.index(index.row(), 0)).value<Key::IdType>();

        mKeyRepository.remove(id);
        refreshKeys();
    }

    void CharacterManagerDialog::selectKey(const QItemSelection &selected, const QItemSelection &deselected)
    {
        Q_UNUSED(deselected);

        mEditKeyBtn->setEnabled(true);
        mRemoveKeyBtn->setEnabled(true);

        mSelectedKeys = selected.indexes();
    }

    void CharacterManagerDialog::removeCharacter()
    {
        mCharacterModel.removeRow(mCharacterModelProxy->mapToSource(mSelectedCharacters.first()).row());
        mCharacterModelProxy->invalidate();

        emit charactersChanged();
    }

    void CharacterManagerDialog::selectCharacter(const QItemSelection &selected, const QItemSelection &deselected)
    {
        Q_UNUSED(deselected);

        mRemoveCharacterBtn->setEnabled(true);
        mSelectedCharacters = selected.indexes();
    }

    void CharacterManagerDialog::refreshKeys()
    {
        mKeyModel.setQuery(QString{"SELECT id, code FROM %1"}.arg(mKeyRepository.getTableName()), mKeyRepository.getDatabase());

        mKeyModel.setHeaderData(0, Qt::Horizontal, tr("Key ID"));
        mKeyModel.setHeaderData(1, Qt::Horizontal, tr("Verification code"));

        mEditKeyBtn->setDisabled(true);
        mRemoveKeyBtn->setDisabled(true);
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

    QWidget *CharacterManagerDialog::createKeyTab()
    {
        std::unique_ptr<QWidget> page{new QWidget{}};

        auto pageLayout = new QVBoxLayout{};
        page->setLayout(pageLayout);

        auto keyGroup = new QGroupBox{tr("Added keys"), this};
        pageLayout->addWidget(keyGroup);

        auto groupLayout = new QVBoxLayout{};
        keyGroup->setLayout(groupLayout);

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

        mEditKeyBtn = new QPushButton{QIcon{":/images/edit.png"}, tr("Edit..."), this};
        btnLayout->addWidget(mEditKeyBtn);
        mEditKeyBtn->setDisabled(true);
        connect(mEditKeyBtn, &QPushButton::clicked, this, &CharacterManagerDialog::editKey);

        mRemoveKeyBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        btnLayout->addWidget(mRemoveKeyBtn);
        mRemoveKeyBtn->setDisabled(true);
        connect(mRemoveKeyBtn, &QPushButton::clicked, this, &CharacterManagerDialog::removeKey);

        return page.release();
    }

    QWidget *CharacterManagerDialog::createCharacterTab()
    {
        std::unique_ptr<QWidget> page{new QWidget{}};

        auto pageLayout = new QVBoxLayout{};
        page->setLayout(pageLayout);

        auto charGroup = new QGroupBox{tr("Available characters"), this};
        pageLayout->addWidget(charGroup);

        auto groupLayout = new QVBoxLayout{};
        charGroup->setLayout(groupLayout);

        mCharacterModelProxy = new QSortFilterProxyModel{this};
        mCharacterModelProxy->setSourceModel(&mCharacterModel);

        auto characterView = new QTreeView{this};
        groupLayout->addWidget(characterView);
        characterView->setModel(mCharacterModelProxy);
        characterView->setMinimumWidth(320);
        characterView->setSortingEnabled(true);
        connect(characterView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &CharacterManagerDialog::selectCharacter);

        auto btnLayout = new QHBoxLayout{};
        pageLayout->addLayout(btnLayout);

        auto refreshCharsBtn = new QPushButton{QIcon{":/images/arrow_refresh.png"}, tr("Refresh"), this};
        btnLayout->addWidget(refreshCharsBtn);
        connect(refreshCharsBtn, &QPushButton::clicked, this, &CharacterManagerDialog::refreshCharacters);

        mRemoveCharacterBtn = new QPushButton{QIcon{":/images/delete.png"}, tr("Remove"), this};
        btnLayout->addWidget(mRemoveCharacterBtn);
        mRemoveCharacterBtn->setDisabled(true);
        connect(mRemoveCharacterBtn, &QPushButton::clicked, this, &CharacterManagerDialog::removeCharacter);

        return page.release();
    }
}
