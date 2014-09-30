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

#include <vector>
#include <atomic>

#include <QThread>
#include <QTimer>
#include <QUrl>

namespace Evernus
{
    class ExternalOrderRepository;

    class UploaderThread
        : public QThread
    {
        Q_OBJECT

    public:
        enum class UploadMethod
        {
            Post,
            PostEntity,
            Put,
            Get
        };

        struct Endpoint
        {
            QString mName;
            QUrl mUrl;
            QString mKey;
            UploadMethod mMethod = UploadMethod::Post;
            bool mEnabled = true;
        };

        explicit UploaderThread(const ExternalOrderRepository &orderRepo, QObject *parent = nullptr);
        virtual ~UploaderThread() = default;

        void setEnabled(bool flag);

        template<class T>
        void emplaceEndpoint(T &&endpoint)
        {
            mEndpoints.emplace_back(std::forward<T>(endpoint));
        }

    signals:
        void statusChanged(const QString &status);

    public slots:
        void handleChangedData();

    private slots:
        void scheduleUpload();
        void finishUpload();

    protected:
        virtual void run() override;

    private:
        const ExternalOrderRepository &mOrderRepo;

        QTimer mWaitTimer;

        std::vector<Endpoint> mEndpoints;

        std::atomic_bool mEnabled;
        bool mDoUpload = false, mDataChanged = true;
        uint mRequestCount = 0;
    };
}
