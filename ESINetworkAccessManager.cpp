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
#include "ESINetworkAccessManager.h"

namespace Evernus
{
    ESINetworkAccessManager::ESINetworkAccessManager(const QString &clientId, const QString &clientSecret, QObject *parent)
        : QNetworkAccessManager{parent}
        , mAutorization{QByteArrayLiteral("Basic ") + (clientId + ":" + clientSecret).toLatin1().toBase64()}
    {
    }

    QNetworkReply *ESINetworkAccessManager::createRequest(Operation op, const QNetworkRequest &originalReq, QIODevice *outgoingData)
    {
        auto request = originalReq;
        request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
        request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
        request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

        if (!request.hasRawHeader(QByteArrayLiteral("Authorization")))
            request.setRawHeader(QByteArrayLiteral("Authorization"), mAutorization);

        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }
}
