#pragma once

#include <unordered_map>
#include <functional>

#include <QNetworkAccessManager>

namespace Evernus
{
    class Key;

    class APIInterface
        : public QObject
    {
        Q_OBJECT

    public:
        typedef std::function<void (const QString &response)> Callback;

        using QObject::QObject;
        virtual ~APIInterface() = default;

        void fetchCharacterList(const Key &key, const Callback &callback);

    private slots:
        void processReply();

    private:
        QNetworkAccessManager mNetworkManager;

        std::unordered_map<QNetworkReply *, Callback> mPendingCallbacks;

        void makeRequest(const QString &endpoint, const Key &key, const Callback &callback);
    };
}
