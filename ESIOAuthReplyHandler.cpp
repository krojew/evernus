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
#include <QtDebug>

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

#include "ESIOAuthReplyHandler.h"

namespace Evernus
{
    QString ESIOAuthReplyHandler::callback() const
    {
        return QStringLiteral("http://evernus.com/sso-authentication/");
    }

    void ESIOAuthReplyHandler::networkReplyFinished(QNetworkReply *reply)
    {
        Q_ASSERT(reply != nullptr);

        const auto replyError = reply->error();
        if (Q_UNLIKELY(replyError != QNetworkReply::NoError))
        {
            qWarning() << "OAuth error:" << replyError << reply->errorString();

            // part 2 of the hack for https://bugreports.qt.io/browse/QTBUG-65778
            // TODO: remove
            if (replyError == QNetworkReply::ProtocolUnknownError && reply->url().path() == QStringLiteral("dummy"))
                return;

            emit error(reply->errorString());
            return;
        }

        const auto data = reply->readAll();
        emit replyDataReceived(data);

        const auto doc = QJsonDocument::fromJson(data);
        emit tokensReceived(doc.object().toVariantMap());
    }
}
