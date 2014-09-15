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
#pragma once

#include <QDateTime>
#include <QDialog>

#include "qdropbox.h"

#include "SimpleCrypt.h"

class QProgressBar;
class QPushButton;
class QGroupBox;
class QLabel;

namespace Evernus
{
    class SyncDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        enum class Mode
        {
            Download,
            Upload
        };

        explicit SyncDialog(Mode mode, QWidget *parent = nullptr);
        virtual ~SyncDialog() = default;

        static bool performedSync();

    private slots:
        void startSync();
        void showTokenLink();
        void acceptToken();
        void showError();
        void setToken(const QString &token, const QString &secret);
        void processMetadata(const QString &json);
        void handleNoFiles();

        void updateProgress(qint64 current, qint64 total);

    private:
        static const QString mainDbPath;

        static QDateTime mLastSyncTime;

        Mode mMode = Mode::Download;
        bool mStarted = false;

        SimpleCrypt mCrypt;
        QDropbox mDb;

        QPushButton *mCancelBtn = nullptr;
        QProgressBar *mProgress = nullptr;
        QGroupBox *mTokenGroup = nullptr;
        QLabel *mTokenLabel = nullptr;

        void requestMetadata();
        void downloadFiles();
        void uploadFiles();

        static QString getMainDbPath();

        template<class T>
        static void asyncExec(T &&func);
    };
}
