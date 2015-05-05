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
#include <future>

#include <QApplication>
#include <QProgressBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QSettings>
#include <QSaveFile>
#include <QFileInfo>
#include <QLabel>
#include <QMovie>
#include <QDebug>
#include <QFile>
#include <QFont>

#include "DatabaseUtils.h"
#include "SyncSettings.h"

#include "qdropboxfile.h"

#include "SyncDialog.h"

#define STR_VALUE(s) #s
#define EVERNUS_TEXT(s) STR_VALUE(s)
#define EVERNUS_DROPBOX_APP_KEY_TEXT EVERNUS_TEXT(EVERNUS_DROPBOX_APP_KEY)
#define EVERNUS_DROPBOX_APP_SECRET_TEXT EVERNUS_TEXT(EVERNUS_DROPBOX_APP_SECRET)

namespace Evernus
{
    const QString SyncDialog::mainDbPath = "/sandbox/main.db";
    QDateTime SyncDialog::mLastSyncTime;

    SyncDialog::SyncDialog(Mode mode, QWidget *parent)
        : QDialog(parent)
        , mMode(mode)
        , mCrypt(Q_UINT64_C(0x4630e0cc6a00124b))
#ifdef EVERNUS_DROPBOX_ENABLED
        , mDb(EVERNUS_DROPBOX_APP_KEY_TEXT, EVERNUS_DROPBOX_APP_SECRET_TEXT)
#endif
    {
        auto mainLayout = new QVBoxLayout{this};

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

        mCancelBtn = new QPushButton{tr("Cancel"), this};
        infoLayout->addWidget(mCancelBtn);
        connect(mCancelBtn, &QPushButton::clicked, this, &SyncDialog::reject);

        mProgress = new QProgressBar{this};
        mainLayout->addWidget(mProgress);
        mProgress->setRange(0, 100);

        mTokenGroup = new QGroupBox{};
        mainLayout->addWidget(mTokenGroup);
        mTokenGroup->setVisible(false);

        auto tokenLayout = new QVBoxLayout{mTokenGroup};

        mTokenLabel = new QLabel{this};
        tokenLayout->addWidget(mTokenLabel);
        mTokenLabel->setOpenExternalLinks(true);
        mTokenLabel->setWordWrap(true);

        auto acceptTokenBtn = new QPushButton{tr("Proceed"), this};
        tokenLayout->addWidget(acceptTokenBtn);
        connect(acceptTokenBtn, &QPushButton::clicked, this, &SyncDialog::acceptToken);

        setWindowTitle(tr("Synchronization"));

        connect(&mDb, &QDropbox::errorOccured, this, &SyncDialog::showError);
        connect(&mDb, &QDropbox::requestTokenFinished, this, &SyncDialog::showTokenLink);
        connect(&mDb, &QDropbox::tokenExpired, this, &SyncDialog::showTokenLink);
        connect(&mDb, &QDropbox::accessTokenFinished, this, &SyncDialog::setToken);
        connect(&mDb, &QDropbox::metadataReceived, this, &SyncDialog::processMetadata);
        connect(&mDb, &QDropbox::fileNotFound, this, &SyncDialog::handleNoFiles);

        QMetaObject::invokeMethod(this, "startSync", Qt::QueuedConnection);
    }

    bool SyncDialog::performedSync()
    {
        return mLastSyncTime.isValid();
    }

    void SyncDialog::startSync()
    {
        qDebug() << "Starting sync...";

        QSettings settings;

        const auto token = mCrypt.decryptToString(settings.value(SyncSettings::dbTokenKey).toString());
        const auto tokenSecret = mCrypt.decryptToString(settings.value(SyncSettings::dbTokenSecretKey).toString());

        if (token.isEmpty() || tokenSecret.isEmpty())
        {
            mDb.requestToken();
        }
        else
        {
            mDb.setToken(token);
            mDb.setTokenSecret(tokenSecret);

            requestMetadata();
        }
    }

    void SyncDialog::showTokenLink()
    {
        mTokenLabel->setText(tr("Dropbox requires authenticating Evernus first. Please click on "
            "<a href='%1'>this link</a>, authorize Evernus and press 'Proceed'.").arg(mDb.authorizeLink().toString()));
        mTokenGroup->show();
        adjustSize();
    }

    void SyncDialog::acceptToken()
    {
        qDebug() << "Requesting access token...";

        mTokenGroup->hide();
        adjustSize();

        mDb.requestAccessToken();
    }

    void SyncDialog::showError()
    {
        qDebug() << "Error:" << mDb.error() << mDb.errorString();

        QMessageBox::warning(this, tr("Synchronization"), tr("Error: %1 (%2)").arg(mDb.error()).arg(mDb.errorString()));
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
    }

    void SyncDialog::setToken(const QString &token, const QString &secret)
    {
        QSettings settings;
        settings.setValue(SyncSettings::dbTokenKey, mCrypt.encryptToString(token));
        settings.setValue(SyncSettings::dbTokenSecretKey, mCrypt.encryptToString(secret));

        requestMetadata();
    }

    void SyncDialog::processMetadata(const QString &json)
    {
        if (mStarted)
            return;

        QDropboxFileInfo info{json};

        if (mMode == Mode::Download)
        {
            QFileInfo currentInfo{getMainDbPath()};
            if (currentInfo.lastModified() > info.modified())
            {
                QSettings settings;
                if (settings.value(SyncSettings::firstSyncKey, true).toBool())
                {
                    const auto ret = QMessageBox::question(this,
                                                           tr("Synchronization"),
                                                           tr("Your local database is newer than cloud one. Do you wish to replace your local copy?"));
                    if (ret == QMessageBox::Yes)
                    {
                        downloadFiles();
                        return;
                    }
                }

                mLastSyncTime = QDateTime::currentDateTimeUtc();
                QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
            }
            else
            {
                downloadFiles();
            }
        }
        else
        {
            if (mLastSyncTime.isValid() && info.modified() > mLastSyncTime)
            {
                const auto ret = QMessageBox::question(this,
                                                       tr("Synchronization"),
                                                       tr("Something modified cloud data since last synchronization. Do you wish to overwrite it?"));
                if (ret == QMessageBox::No)
                {
                    QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
                    return;
                }
            }

            uploadFiles();
        }
    }

    void SyncDialog::handleNoFiles()
    {
        if (mMode == Mode::Download)
            reject();
        else if (!mStarted)
            uploadFiles();
    }

    void SyncDialog::updateProgress(qint64 current, qint64 total)
    {
        if (total > 0)
            mProgress->setValue(current * 100 / total);
    }

    void SyncDialog::requestMetadata()
    {
        qDebug() << "Requesting metadata...";
        mDb.requestMetadata(mainDbPath);
    }

    void SyncDialog::downloadFiles()
    {
        qDebug() << "Downloading files...";

        mStarted = true;

        QDropboxFile mainDb{mainDbPath, &mDb};
        connect(&mainDb, &QDropboxFile::downloadProgress, this, &SyncDialog::updateProgress);
        connect(mCancelBtn, &QPushButton::clicked, &mainDb, &QDropboxFile::abort, Qt::QueuedConnection);

        if (mainDb.open(QIODevice::ReadOnly))
        {
            qDebug() << "Saving main db...";

            mCancelBtn->setEnabled(false);
            DatabaseUtils::backupDatabase(getMainDbPath());

            QSaveFile file{getMainDbPath()};
            if (!file.open(QIODevice::WriteOnly))
            {
                QMessageBox::warning(this, tr("Synchronization"), tr("Couldn't open file for writing! Synchronization failed."));
                QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
                return;
            }

            asyncExec([&, this] {
                file.write(qUncompress(mainDb.readAll()));
            });
            mCancelBtn->setEnabled(true);

            if (file.commit())
            {
                mLastSyncTime = mainDb.metadata().modified();
            }
            else
            {
                QMessageBox::warning(this, tr("Synchronization"), tr("Couldn't write destination file! Synchronization failed."));
                QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            }
        }

        QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
    }

    void SyncDialog::uploadFiles()
    {
        qDebug() << "Uploading files...";

        mStarted = true;
        mCancelBtn->setEnabled(false);

        QDropboxFile mainDb{mainDbPath, &mDb};
        mainDb.setOverwrite(true);
        connect(&mainDb, &QDropboxFile::uploadProgress, this, &SyncDialog::updateProgress);
        connect(mCancelBtn, &QPushButton::clicked, &mainDb, &QDropboxFile::abort);

        if (!mainDb.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(this, tr("Synchronization"), tr("Couldn't open remote file! Synchronization failed."));
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }

        QFile localMainDb{getMainDbPath()};
        if (!localMainDb.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, tr("Synchronization"), tr("Couldn't open local file! Synchronization failed."));
            QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
            return;
        }

        asyncExec([&] {
            const auto data = qCompress(localMainDb.readAll(), 9);

            mainDb.setFlushThreshold(data.size() + 1);
            mainDb.write(data);
        });
        mCancelBtn->setEnabled(true);

        mainDb.close();

        QSettings settings;
        settings.setValue(SyncSettings::firstSyncKey, false);

        mLastSyncTime = mainDb.metadata().modified();

        QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
    }

    QString SyncDialog::getMainDbPath()
    {
        return DatabaseUtils::getDbPath() + "main.db";
    }

    template<class T>
    void SyncDialog::asyncExec(T &&func)
    {
        auto future = std::async(std::launch::async, std::forward<T>(func));
        while (future.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}
