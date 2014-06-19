#include <memory>

#include <QDialogButtonBox>
#include <QMessageBox>
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
                                                   APIManager &apiManager,
                                                   QWidget *parent)
        : QDialog{parent}
        , mCharacterRepository{characterRepository}
        , mKeyRepository{keyRepository}
        , mApiManager{apiManager}
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
        for (const auto &index : mSelectedKeys)
        {
            const auto id = mKeyModel.data(mKeyModel.index(index.row(), 0)).value<Key::IdType>();
            mKeyRepository.remove(id);
        }

        refreshKeys();
    }

    void CharacterManagerDialog::selectKey(const QItemSelection &selected, const QItemSelection &deselected)
    {
        mEditKeyBtn->setEnabled(true);
        mRemoveKeyBtn->setEnabled(true);

        mSelectedKeys = selected.indexes();
    }

    void CharacterManagerDialog::refreshKeys()
    {
        mKeyModel.setQuery(QString{"SELECT id, code FROM %1"}.arg(mKeyRepository.getTableName()));

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
            fetchCharacters();
        }
    }

    void CharacterManagerDialog::fetchCharacters()
    {
        const auto keys = mKeyRepository.fetchAll();
        for (const auto &key : keys)
        {
            mApiManager.fetchCharacterList(key, [key, this](const APIManager::CharacterList &characters) {
                try
                {
                    if (characters.empty())
                    {
                        auto query = mCharacterRepository.prepare(
                            QString{"UPDATE %1 SET key_id = NULL WHERE key_id = ?"}.arg(mCharacterRepository.getTableName()));
                        query.bindValue(0, key.getCode());
                        query.exec();
                    }
                    else
                    {
                        QStringList ids;
                        for (auto i = 0; i < characters.size(); ++i)
                            ids << "?";

                        auto query = mCharacterRepository.prepare(QString{"UPDATE %1 SET key_id = NULL WHERE %2 NOT IN (%3)"}
                            .arg(mCharacterRepository.getTableName())
                            .arg(mCharacterRepository.getIdColumn())
                            .arg(ids.join(", ")));

                        for (auto i = 0; i < characters.size(); ++i)
                            query.bindValue(i, characters[i]);

                        query.exec();
                    }
                }
                catch (...)
                {
                    QMessageBox::warning(this, tr("Evernus"), tr("An error occurred updating character key information. "
                        "Data sync should work, but character tab will display incorrect information."));
                }
            });
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

        return page.release();
    }
}
