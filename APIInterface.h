#pragma once

#include <unordered_map>
#include <functional>
#include <vector>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "Character.h"

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

        void fetchCharacterList(const Key &key, const Callback &callback) const;
        void fetchCharacter(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchAssets(const Key &key, Character::IdType characterId, const Callback &callback) const;
        void fetchConquerableStationList(const Callback &callback) const;

    signals:
        void generalError(const QString &info);

    private slots:
        void processReply();
        void processSslErrors(const QList<QSslError> &errors);

    private:
        typedef std::vector<std::pair<QString, QString>> QueryParams;

        mutable QNetworkAccessManager mNetworkManager;

        mutable std::unordered_map<QNetworkReply *, Callback> mPendingCallbacks;

        void makeRequest(const QString &endpoint,
                         const Key &key,
                         const Callback &callback,
                         const QueryParams &additionalParams = QueryParams{}) const;
    };
}
