#include <QDialogButtonBox>
#include <QIntValidator>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>

#include "Key.h"

#include "KeyEditDialog.h"

namespace Evernus
{
    KeyEditDialog::KeyEditDialog(Key &key, QWidget *parent)
        : QDialog{parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint}
        , mKey{key}
    {
        auto mainLayout = new QVBoxLayout{this};

        auto formLayout = new QFormLayout{this};

        auto idValidator = new QIntValidator{this};
        idValidator->setBottom(0);

        mIdEdit = new QLineEdit{QString::number(mKey.getId()), this};
        mIdEdit->setValidator(idValidator);
        formLayout->addRow(tr("Key ID:"), mIdEdit);

        mCodeEdit = new QLineEdit{mKey.getCode(), this};
        formLayout->addRow(tr("Verification Code:"), mCodeEdit);

        mainLayout->addLayout(formLayout);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        connect(buttons, &QDialogButtonBox::accepted, this, &KeyEditDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLayout->addWidget(buttons);

        setLayout(mainLayout);
    }

    void KeyEditDialog::accept()
    {
        mKey.setId(mIdEdit->text().toInt());
        mKey.setCode(mCodeEdit->text());

        QDialog::accept();
    }
}
