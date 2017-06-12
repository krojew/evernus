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
#include <QDialogButtonBox>
#include <QIntValidator>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

#include "Repository.h"
#include "CorpKey.h"

#include "CorpKeyEditDialog.h"

namespace Evernus
{
    CorpKeyEditDialog::CorpKeyEditDialog(const Repository<Character> &characterRepo, CorpKey &corpKey, QWidget *parent)
        : QDialog(parent)
        , mCharacterRepo(characterRepo)
        , mCorpKey(corpKey)
    {
        const auto corpKeyLink = "https://community.eveonline.com/support/api-key/CreatePredefined?accessMask=73404426";

        auto mainLayout = new QVBoxLayout{this};

        auto formLayout = new QFormLayout{};
        mainLayout->addLayout(formLayout);

        mCharacterEdit = new QComboBox{this};
        formLayout->addRow(tr("Character:"), mCharacterEdit);

        auto characters = characterRepo.fetchAll();
        std::sort(std::begin(characters), std::end(characters), [](const auto &a, const auto &b) {
            return a->getName() < b->getName();
        });

        for (const auto &character : characters)
        {
            mCharacterEdit->addItem(character->getName(), character->getId());
            if (character->getId() == mCorpKey.getCharacterId())
                mCharacterEdit->setCurrentIndex(mCharacterEdit->count() - 1);
        }

        auto idValidator = new QIntValidator{this};
        idValidator->setBottom(0);

        mIdEdit = new QLineEdit{QString::number(mCorpKey.getId()), this};
        formLayout->addRow(tr("Key ID:"), mIdEdit);
        mIdEdit->setValidator(idValidator);

        mCodeEdit = new QLineEdit{mCorpKey.getCode(), this};
        formLayout->addRow(tr("Verification Code:"), mCodeEdit);

        mainLayout->addWidget(new QLabel{tr("To create a predefined corporation key, use the following link:"), this});

        auto linkLabel = new QLabel{QString{"<a href='%1'>%1</a>"}.arg(corpKeyLink), this};
        mainLayout->addWidget(linkLabel);
        linkLabel->setOpenExternalLinks(true);

        mainLayout->addWidget(new QLabel{tr("Corporation keys require character keys added first."), this});

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setWindowTitle(tr("Corporation Key Edit"));
    }

    void CorpKeyEditDialog::accept()
    {
        const auto charId = mCharacterEdit->currentData().value<Character::IdType>();
        if (charId == Character::IdType{})
        {
            QMessageBox::warning(this, tr("Invalid character"), tr("Please select a valid character."));
            return;
        }

        mCorpKey.setId(mIdEdit->text().toInt());
        mCorpKey.setCharacterId(charId);
        mCorpKey.setCode(mCodeEdit->text().trimmed());

        QDialog::accept();
    }
}
