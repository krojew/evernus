#pragma once

#include <unordered_map>
#include <functional>

#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Evernus
{
    class Key;

    class APIInterface
        : public QObject
    {
        Q_OBJECT

    public:
        typedef std::function<void (const QString &response, const QString &error)> Callback;

        using QObject::QObject;
        virtual ~APIInterface() = default;

        void fetchCharacterList(const Key &key, const Callback &callback);

    signals:
        void generalError(const QString &info);

    private slots:
        void processReply();
        void processSslErrors(const QList<QSslError> &errors);

    private:
        QNetworkAccessManager mNetworkManager;

        std::unordered_map<QNetworkReply *, Callback> mPendingCallbacks;

        void makeRequest(const QString &endpoint, const Key &key, const Callback &callback);
    };
}
