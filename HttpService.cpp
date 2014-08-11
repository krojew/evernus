/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Http Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Http Public License for more details.
 *
 *  You should have received a copy of the GNU Http Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QSettings>

#include "HttpSettings.h"

#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"

#include "HttpService.h"

namespace Evernus
{
    HttpService::HttpService(const MarketOrderProvider &orderProvider,
                             const MarketOrderProvider &corpOrderProvider,
                             const EveDataProvider &dataProvider,
                             QxtHttpSessionManager *sm,
                             QObject *parent)
        : QxtWebSlotService(sm, parent)
        , mOrderProvider(orderProvider)
        , mCorpOrderProvider(corpOrderProvider)
        , mDataProvider(dataProvider)
        , mCrypt(HttpSettings::cryptKey)
    {
        mMainTemplate.open(":/html/http_template.html");
    }

    void HttpService::index(QxtWebRequestEvent *event)
    {
        renderContent(event, tr("Please select a category from the menu."));
    }

    void HttpService::pageRequestedEvent(QxtWebRequestEvent *event)
    {
        auto authHeader = event->headers.value("Authorization");
        if (!authHeader.startsWith("Basic "))
        {
            postUnauthorized(event);
            return;
        }

        authHeader.remove(0, 6);

        const auto auth = QByteArray::fromBase64(authHeader.toLatin1());
        if (!auth.isEmpty())
        {
            const auto sep = auth.indexOf(':');
            const auto user = auth.left(sep);
            const auto password = auth.mid(sep + 1);

            QSettings settings;
            if (user == settings.value(HttpSettings::userKey).toByteArray() &&
                password == mCrypt.decryptToByteArray(settings.value(HttpSettings::passwordKey).toString()))
            {
                QxtWebSlotService::pageRequestedEvent(event);
            }
            else
            {
                postUnauthorized(event);
            }
        }
        else
        {
            postUnauthorized(event);
        }
    }

    void HttpService::renderContent(QxtWebRequestEvent *event, const QString &content)
    {
        mMainTemplate["content"] = content;
        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, mMainTemplate.render().toUtf8()));
    }

    void HttpService::postUnauthorized(QxtWebRequestEvent *event)
    {
        auto pageEvent = new QxtWebErrorEvent(event->sessionID, event->requestID, 401, "Not Authorized");
        pageEvent->headers.insert("WWW-Authenticate", "Basic realm=\"Evernus\"");

        postEvent(pageEvent);
    }
}
