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
#include <QUrlQuery>

#include "CharacterRepository.h"
#include "HttpSettings.h"

#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"

#include "HttpService.h"

namespace Evernus
{
    const QString HttpService::characterIdName = "characterId";

    HttpService::HttpService(const MarketOrderProvider &orderProvider,
                             const MarketOrderProvider &corpOrderProvider,
                             const EveDataProvider &dataProvider,
                             const CharacterRepository &characterRepo,
                             QxtHttpSessionManager *sm,
                             QObject *parent)
        : QxtWebSlotService(sm, parent)
        , mOrderProvider(orderProvider)
        , mCorpOrderProvider(corpOrderProvider)
        , mDataProvider(dataProvider)
        , mCharacterRepo(characterRepo)
        , mCrypt(HttpSettings::cryptKey)
    {
        mMainTemplate.open(":/html/http_template.html");
        mIndexTemplate.open(":/html/http_index_template.html");

        mMainTemplate["index-link-text"] = tr("Select Character");

        mIndexTemplate["select-character-text"] = tr("Select character:");
        mIndexTemplate["ok-text"] = tr("OK");
        mIndexTemplate["character-id-name"] = characterIdName;
    }

    void HttpService::index(QxtWebRequestEvent *event)
    {
        const auto characters = mCharacterRepo.fetchAll();

        QStringList options;
        for (const auto &character : characters)
            options << QString{"<option value='%1'>%2</option>"}.arg(character->getId()).arg(character->getName());

        mIndexTemplate["characters"] = options.join("\n");
        renderContent(event, mIndexTemplate.render());
    }

    void HttpService::characterOrders(QxtWebRequestEvent *event)
    {

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
                QUrlQuery query{event->url.query()};
                if (!query.hasQueryItem(characterIdName) && !isIndexAction(event))
                    postEvent(new QxtWebRedirectEvent{event->sessionID, event->requestID, "/"});
                else
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
        postEvent(new QxtWebPageEvent{event->sessionID, event->requestID, mMainTemplate.render().toUtf8()});
    }

    void HttpService::postUnauthorized(QxtWebRequestEvent *event)
    {
        auto pageEvent = new QxtWebErrorEvent{event->sessionID, event->requestID, 401, "Not Authorized"};
        pageEvent->headers.insert("WWW-Authenticate", "Basic realm=\"Evernus\"");

        postEvent(pageEvent);
    }

    bool HttpService::isIndexAction(QxtWebRequestEvent *event)
    {
        auto args = event->url.path().split('/');
        args.removeFirst();
        if (args.at(args.count() - 1).isEmpty())
            args.removeLast();

        if (args.count() == 0)
            return true;

        const auto action = args.at(0).toUtf8();
        return action.trimmed().isEmpty() || action == "index";
    }
}
