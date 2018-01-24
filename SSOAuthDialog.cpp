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
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>

#include "SSOAuthDialog.h"

namespace Evernus
{
    SSOAuthDialog::SSOAuthDialog(const QUrl &url, QWidget *parent)
        : QDialog{parent}
    {
        auto mainLayout = new QVBoxLayout{this};

        auto infoLabel = new QLabel{
            tr("Please authorize in the browser and wait for Evernus to proceed. Click the following link, if the browser didn't open: <a href='%1'>%1</a>").arg(url.toString()),
            this
        };
        mainLayout->addWidget(infoLabel);
        infoLabel->setWordWrap(true);
        infoLabel->setTextFormat(Qt::RichText);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &SSOAuthDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &SSOAuthDialog::close);

        setModal(true);
        adjustSize();

        QDesktopServices::openUrl(url);
    }

    void SSOAuthDialog::closeEvent(QCloseEvent *event)
    {
        Q_UNUSED(event);
        emit aboutToClose();
    }
}
