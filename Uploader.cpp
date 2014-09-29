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
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QTimer>
#include <QDebug>

#include "UploaderSettings.h"

#include "Uploader.h"

namespace Evernus
{
    Uploader::Uploader(const ExternalOrderRepository &orderRepo, QObject *parent)
        : QObject{parent}
        , mWorkerThread{orderRepo}
    {
        qDebug() << "Initializing uploader...";

        QSettings settings;
        mWorkerThread.setEnabled(settings.value(UploaderSettings::enabledKey, UploaderSettings::enabledDefault).toBool());

        requestEndpoints();

        connect(this, &Uploader::dataChanged, &mWorkerThread, &UploaderThread::handleChangedData);
    }

    Uploader::~Uploader()
    {
        try
        {
            mWorkerThread.requestInterruption();
            mWorkerThread.wait();
        }
        catch (...)
        {
        }
    }

    void Uploader::setEnabled(bool flag)
    {
        mWorkerThread.setEnabled(flag);
    }

    void Uploader::requestEndpoints()
    {
        qDebug() << "Requesting endpoints...";

        // TODO: use own domain
        auto reply = mAccessManager.get(
            QNetworkRequest{QUrl{"https://bitbucket.org/BattleClinic/evemon/wiki/emuu/endpoints/endpoints.json"}});
        connect(reply, &QNetworkReply::finished, this, &Uploader::handleEndpoints);
    }

    void Uploader::handleEndpoints()
    {
        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        const auto error = reply->error();

        qDebug() << "Got MUU endpoints:" << error;

        if (error != QNetworkReply::NoError)
        {
            QTimer::singleShot(60 * 1000, this, SLOT(requestEndpoints()));
            return;
        }

        const auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirectUrl.isEmpty())
        {
            auto reply = mAccessManager.get(QNetworkRequest{redirectUrl});
            connect(reply, &QNetworkReply::finished, this, &Uploader::handleEndpoints);
            return;
        }

        QJsonParseError jsonError;
        const auto doc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

        if (jsonError.error != QJsonParseError::NoError)
        {
            qDebug() << jsonError.errorString();
            return;
        }

        const auto jsonObj = doc.object();
        const auto jsonArray = jsonObj.value("endpoints").toArray();

        for (const auto &jsonEndpoint : jsonArray)
        {
            const auto endpointObj =  jsonEndpoint.toObject();

            UploaderThread::Endpoint endpoint;
            endpoint.mName = endpointObj.value("name").toString();
            endpoint.mUrl = endpointObj.value("url").toString();
            endpoint.mKey = endpointObj.value("key").toString();
            endpoint.mEnabled = endpointObj.value("enabled").toString() == "True";

            const auto method = endpointObj.value("method").toString();
            if (method == "post")
                endpoint.mMethod = UploaderThread::UploadMethod::Post;
            else if (method == "postentity")
                endpoint.mMethod = UploaderThread::UploadMethod::PostEntity;
            else if (method == "put")
                endpoint.mMethod = UploaderThread::UploadMethod::Put;
            else if (method == "get")
                endpoint.mMethod = UploaderThread::UploadMethod::Get;
            else
                continue;

            mWorkerThread.emplaceEndpoint(std::move(endpoint));
        }

        mWorkerThread.start();
    }
}
