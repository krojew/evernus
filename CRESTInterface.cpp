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
#include <QDesktopWidget>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonObject>
#include <QUrlQuery>
#include <QWebFrame>
#include <QDebug>

#include "CRESTInterface.h"

#define STR_VALUE(s) #s
#define EVERNUS_TEXT(s) STR_VALUE(s)
#define EVERNUS_CREST_CLIENT_ID_TEXT EVERNUS_TEXT(EVERNUS_CREST_CLIENT_ID)
#define EVERNUS_CREST_SECRET_TEXT EVERNUS_TEXT(EVERNUS_CREST_SECRET)

namespace Evernus
{
#ifdef EVERNUS_CREST_SISI
    const QString CRESTInterface::crestUrl = "http://public-crest-sisi.testeveonline.com";
    const QString CRESTInterface::loginUrl = "https://sisilogin.testeveonline.com";
#else
    const QString CRESTInterface::crestUrl = "http://public-crest.eveonline.com";
    const QString CRESTInterface::loginUrl = "https://login.eveonline.com";
#endif
    const QString CRESTInterface::redirectUrl = "evernus.com";

    const QString CRESTInterface::authUrlName = "authEndpoint";

    CRESTInterface::CRESTInterface(QObject *parent)
        : QObject{parent}
    {
        qDebug() << "Fetching CREST endpoints...";

        auto reply = mNetworkManager.get(QNetworkRequest{crestUrl});
        connect(reply, &QNetworkReply::finished, this, [reply, this] {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError)
            {
                qDebug() << "Error fetching CREST endpoints:" << reply->errorString();
                return;
            }

            const auto json = QJsonDocument::fromJson(reply->readAll());
            if (json.isNull())
            {
                qDebug() << "Null CREST endpoints.";
                return;
            }

            std::function<void (const QJsonObject &)> addEndpoints = [this, &addEndpoints](const QJsonObject &object) {
                for (auto it = std::begin(object); it != std::end(object); ++it)
                {
                    const auto value = it.value().toObject();
                    if (value.contains("href"))
                    {
                        qDebug() << "Endpoint:" << it.key() << "->" << it.value();
                        mEndpoints[it.key()] = value.value("href").toString();
                    }
                    else
                    {
                        addEndpoints(value);
                    }
                }
            };

            addEndpoints(json.object());
        });
    }

    bool CRESTInterface::eventFilter(QObject *watched, QEvent *event)
    {
        Q_ASSERT(event != nullptr);

        if (watched == mAuthView.get() && event->type() == QEvent::Close)
        {
            qDebug() << "Auth window closed.";
            for (const auto &request : mPendingRequests)
                request(tr("CREST authorization failed."));

            mPendingRequests.clear();
        }

        return QObject::eventFilter(watched, event);
    }

    void CRESTInterface::fetchMarketOrders(uint regionId, EveType::IdType typeId, const Callback &callback) const
    {
        qDebug() << "Fetching orders for" << regionId << "and" << typeId;

        auto fetcher = [=](const auto &error) {
            callback(QJsonDocument{}, error);
        };

        checkAuth(fetcher);
    }

    template<class T>
    void CRESTInterface::checkAuth(const T &continuation) const
    {
        if (mExpiry < QDateTime::currentDateTime())
        {
            if (!mEndpoints.contains(authUrlName))
            {
                continuation("Missing CREST auth url!");
                return;
            }

            mPendingRequests.emplace_back(continuation);
            if (mPendingRequests.size() == 1)
            {
                auto processPending = [=](const auto &error) {
                    for (const auto &request : mPendingRequests)
                        request(error);

                    mPendingRequests.clear();
                };

                fetchAccessToken(processPending);
            }
        }
        else
        {
            continuation(QString{});
        }
    }

    template<class T>
    void CRESTInterface::fetchAccessToken(const T &continuation) const
    {
        qDebug() << "Refreshing access token...";

        if (mRefreshToken.isEmpty())
        {
            qDebug() << "No refresh token - requesting access.";

            QUrl url{loginUrl + "/oauth/authorize"};

            QUrlQuery query;
            query.addQueryItem("response_type", "code");
            query.addQueryItem("redirect_uri", "http://" + redirectUrl);
            query.addQueryItem("client_id", EVERNUS_CREST_CLIENT_ID_TEXT);
            query.addQueryItem("scope", "publicData");

            url.setQuery(query);

            mAuthView = std::make_unique<QWebView>();
            mAuthView->setWindowModality(Qt::ApplicationModal);
            mAuthView->setWindowTitle(tr("CREST"));
            mAuthView->installEventFilter(const_cast<CRESTInterface *>(this));
            mAuthView->adjustSize();
            mAuthView->move(QApplication::desktop()->screenGeometry(QApplication::activeWindow()).center() -
                            mAuthView->rect().center());
            mAuthView->setUrl(url);
            mAuthView->show();

            connect(mAuthView->page()->mainFrame(), &QWebFrame::urlChanged, [=](const auto &url) {
                if (url.host() == redirectUrl)
                {
                    mAuthView->removeEventFilter(const_cast<CRESTInterface *>(this));
                    mAuthView->close();

                    qDebug() << "Requesting access token...";

                    QUrlQuery query{url};
                    QByteArray data = "grant_type=authorization_code&code=";
                    data.append(query.queryItemValue("code"));

                    QNetworkRequest request{mEndpoints[authUrlName]};
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
                    request.setRawHeader(
                        "Authorization", QByteArray{EVERNUS_CREST_CLIENT_ID_TEXT ":" EVERNUS_CREST_SECRET_TEXT}.toBase64());

                    auto reply = mNetworkManager.post(request, data);
                    connect(reply, &QNetworkReply::finished, this, [reply, continuation, this] {
                        reply->deleteLater();

                        if (reply->error() != QNetworkReply::NoError)
                        {
                            qDebug() << "Error requesting access token:" << reply->errorString();
                            continuation(reply->errorString());
                            return;
                        }

                        const auto doc = QJsonDocument::fromJson(reply->readAll());
                        const auto object = doc.object();

                        mRefreshToken = object.value("refresh_token").toString();
                        if (mRefreshToken.isEmpty())
                        {
                            qDebug() << "Empty refresh token!";
                            continuation(tr("Empty refresh token!"));
                            return;
                        }

                        mExpiry = QDateTime::currentDateTime().addSecs(object.value("expires_in").toInt() - 10);
                        mAccessToken = object.value("access_token").toString();

                        continuation(QString{});
                    });
                }
            });
        }
        else
        {
            qDebug() << "Refreshing token...";

            QByteArray data = "grant_type=refresh_token&refresh_token=";
            data.append(mRefreshToken);

            QNetworkRequest request{mEndpoints[authUrlName]};
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            request.setRawHeader(
                "Authorization", QByteArray{EVERNUS_CREST_CLIENT_ID_TEXT ":" EVERNUS_CREST_SECRET_TEXT}.toBase64());

            auto reply = mNetworkManager.post(request, data);
            connect(reply, &QNetworkReply::finished, this, [reply, continuation, this] {
                reply->deleteLater();

                if (reply->error() != QNetworkReply::NoError)
                {
                    qDebug() << "Error refreshing token:" << reply->errorString();
                    continuation(reply->errorString());
                    return;
                }

                const auto doc = QJsonDocument::fromJson(reply->readAll());
                const auto object = doc.object();

                mAccessToken = object.value("access_token").toString();
                if (mAccessToken.isEmpty())
                {
                    qDebug() << "Empty access token!";
                    continuation(tr("Empty access token!"));
                    return;
                }

                mExpiry = QDateTime::currentDateTime().addSecs(doc.object().value("expires_in").toInt() - 10);

                continuation(QString{});
            });
        }
    }
}
