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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QSettings>
#include <QLabel>
#include <QMovie>
#include <QFont>

#include "SyncSettings.h"

#include "SyncDialog.h"

#define EVERNUS_TEXT(s) #s

namespace Evernus
{
    SyncDialog::SyncDialog(QWidget *parent)
        : QDialog(parent)
        , mCrypt(Q_UINT64_C(0x4630e0cc6a00124b))
#ifdef EVERNUS_DROPBOX_ENABLED
        , mDb(EVERNUS_TEXT(EVERNUS_DROPBOX_APP_KEY), EVERNUS_TEXT(EVERNUS_DROPBOX_APP_SECRET))
#endif
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto infoLayout = new QHBoxLayout{};
        mainLayout->addLayout(infoLayout);

        auto throbberMovie = new QMovie{":/images/loader.gif", QByteArray{}, this};

        QFont font;
        font.setPixelSize(16);

        auto throbber = new QLabel{this};
        infoLayout->addWidget(throbber);
        throbber->setMovie(throbberMovie);

        throbberMovie->start();

        auto throbberText = new QLabel{tr("Synchronizing..."), this};
        infoLayout->addWidget(throbberText, 1, Qt::AlignLeft);
        throbberText->setFont(font);

        auto cancelBtn = new QPushButton{tr("Cancel"), this};
        infoLayout->addWidget(cancelBtn);
        connect(cancelBtn, &QPushButton::clicked, this, &SyncDialog::reject);

        mTokenGroup = new QGroupBox{};
        mainLayout->addWidget(mTokenGroup);
        mTokenGroup->setVisible(false);

        auto tokenLayout = new QFormLayout{};
        mTokenGroup->setLayout(tokenLayout);

        mTokenEdit = new QLineEdit{this};
        tokenLayout->addRow(tr("Token:"), mTokenEdit);

        mTokenSecretEdit = new QLineEdit{this};
        tokenLayout->addRow(tr("Token secret:"), mTokenSecretEdit);

        auto tokenLabel = new QLabel{tr("Dropbox requires authenticating Evernus first. Please click on "
                "<a href='%1'>this link</a>, obtain token data, fill the form above and press 'Accept'.")
                .arg(mDb.authorizeLink().toString()), this};
        tokenLayout->addRow(tokenLabel);
        tokenLabel->setOpenExternalLinks(true);
        tokenLabel->setWordWrap(true);

        setWindowTitle(tr("Synchronization"));

        QMetaObject::invokeMethod(this, "startSync", Qt::QueuedConnection);
    }

    void SyncDialog::startSync()
    {
        QSettings settings;

        const auto token = settings.value(SyncSettings::dbTokenKey).toString();
        const auto tokenSecret = settings.value(SyncSettings::dbTokenSecretKey).toString();

        if (token.isEmpty() || tokenSecret.isEmpty())
        {
            mTokenGroup->show();
            adjustSize();
        }
    }
}
