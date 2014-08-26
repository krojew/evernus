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
#include <QLineEdit>
#include <QLabel>

#include "Key.h"

#include "KeyEditDialog.h"

namespace Evernus
{
    KeyEditDialog::KeyEditDialog(Key &key, QWidget *parent)
        : QDialog(parent)
        , mKey(key)
    {
        const auto keyLink = "https://community.eveonline.com/support/api-key/CreatePredefined?accessMask=73404426";

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto formLayout = new QFormLayout{};
        mainLayout->addLayout(formLayout);

        auto idValidator = new QIntValidator{this};
        idValidator->setBottom(0);

        mIdEdit = new QLineEdit{QString::number(mKey.getId()), this};
        formLayout->addRow(tr("Key ID:"), mIdEdit);
        mIdEdit->setValidator(idValidator);

        mCodeEdit = new QLineEdit{mKey.getCode(), this};
        formLayout->addRow(tr("Verification Code:"), mCodeEdit);

        mainLayout->addWidget(new QLabel{tr("To create a predefined key, use the following link:")});

        auto linkLabel = new QLabel{QString{"<a href='%1'>%1</a>"}.arg(keyLink), this};
        mainLayout->addWidget(linkLabel);
        linkLabel->setOpenExternalLinks(true);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setWindowTitle(tr("Key Edit"));
    }

    void KeyEditDialog::accept()
    {
        mKey.setId(mIdEdit->text().toInt());
        mKey.setCode(mCodeEdit->text());

        QDialog::accept();
    }
}
