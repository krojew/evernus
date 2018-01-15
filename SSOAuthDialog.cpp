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
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

#include "SSOAuthDialog.h"

namespace Evernus
{
    SSOAuthDialog::SSOAuthDialog(QWidget *parent)
        : QDialog{parent}
    {
        auto mainLayout = new QVBoxLayout{this};

        auto infoLabel = new QLabel{tr("Please authorize in the browser and paste the resulting code below."), this};
        mainLayout->addWidget(infoLabel);

        auto codeLayout = new QHBoxLayout{};
        mainLayout->addLayout(codeLayout);

        mCodeEdit = new QLineEdit{this};
        codeLayout->addWidget(mCodeEdit);
        mCodeEdit->setPlaceholderText(tr("paste the resulting code here"));

        auto authBtn = new QPushButton{tr("Authorize"), this};
        codeLayout->addWidget(authBtn);
        connect(authBtn, &QPushButton::clicked, this, &SSOAuthDialog::applyCode);

        setModal(true);
        adjustSize();
    }

    void SSOAuthDialog::closeEvent(QCloseEvent *event)
    {
        Q_UNUSED(event);
        emit aboutToClose();
    }

    void SSOAuthDialog::applyCode()
    {
        const auto code = mCodeEdit->text().toLatin1();
        if (code.isEmpty())
        {
            QMessageBox::warning(this,
                                 tr("SSO Authentication"),
                                 tr("The supplied code is empty. Please make sure all characters were copied."));
            return;
        }

        emit acquiredCode(code);
    }
}
