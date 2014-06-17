#include <memory>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTabWidget>

#include "CharacterRepository.h"
#include "KeyRepository.h"
#include "KeyEditDialog.h"

#include "CharacterManagerDialog.h"

namespace Evernus
{
    CharacterManagerDialog::CharacterManagerDialog(const CharacterRepository &characterRepository,
                                                   const KeyRepository &keyRepository,
                                                   QWidget *parent)
        : QDialog{parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint}
        , mCharacterRepository{characterRepository}
        , mKeyRepository{keyRepository}
    {
        auto mainLayout = new QVBoxLayout{this};

        auto tabs = new QTabWidget{this};
        tabs->addTab(createCharacterTab(), tr("Characters"));
        tabs->addTab(createKeyTab(), tr("Keys"));
        mainLayout->addWidget(tabs);

        setLayout(mainLayout);
        setModal(true);
    }

    void CharacterManagerDialog::addKey()
    {
        Key newKey;
        KeyEditDialog dlg{newKey, this};
        if (dlg.exec() == QDialog::Accepted)
        {
            mKeyRepository.store(newKey);
        }
    }

    void CharacterManagerDialog::editKey()
    {

    }

    void CharacterManagerDialog::removeKey()
    {

    }

    QWidget *CharacterManagerDialog::createCharacterTab()
    {
        std::unique_ptr<QWidget> page{new QWidget{}};

        auto pageLayout = new QVBoxLayout{};
        page->setLayout(pageLayout);

        return page.release();
    }

    QWidget *CharacterManagerDialog::createKeyTab()
    {
        std::unique_ptr<QWidget> page{new QWidget{}};

        auto pageLayout = new QVBoxLayout{};
        page->setLayout(pageLayout);

        auto btnLayout = new QHBoxLayout{};
        pageLayout->addLayout(btnLayout);

        auto addBtn = new QPushButton{tr("Add...")};
        btnLayout->addWidget(addBtn);
        connect(addBtn, &QPushButton::clicked, this, &CharacterManagerDialog::addKey);

        auto editBtn = new QPushButton{tr("Edit...")};
        btnLayout->addWidget(editBtn);
        connect(editBtn, &QPushButton::clicked, this, &CharacterManagerDialog::editKey);

        auto removeBtn = new QPushButton{tr("Remove")};
        btnLayout->addWidget(removeBtn);
        connect(removeBtn, &QPushButton::clicked, this, &CharacterManagerDialog::removeKey);

        auto keys = mKeyRepository.fetchAll();

        return page.release();
    }
}
