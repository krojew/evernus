#pragma once

#include <unordered_map>
#include <memory>

#include <QApplication>
#include <QSqlDatabase>

#include "ConquerableStationRepository.h"
#include "CharacterRepository.h"
#include "AssetListRepository.h"
#include "EveTypeRepository.h"
#include "ItemRepository.h"
#include "TaskConstants.h"
#include "KeyRepository.h"
#include "NameProvider.h"
#include "APIManager.h"

class QSplashScreen;

namespace Evernus
{
    class Key;

    class EvernusApplication
        : public QApplication
        , public NameProvider
    {
        Q_OBJECT

    public:
        EvernusApplication(int &argc, char *argv[]);
        virtual ~EvernusApplication() = default;

        virtual QString getTypeName(EveType::IdType id) const override;
        virtual QString getLocationName(quint64 id) const override;

        const KeyRepository &getKeyRepository() const noexcept;
        const CharacterRepository &getCharacterRepository() const noexcept;
        const AssetListRepository &getAssetListRepository() const noexcept;

        APIManager &getAPIManager() noexcept;

    signals:
        void taskStarted(quint32 taskId, const QString &description);
        void taskStarted(quint32 taskId, quint32 parentTask, const QString &description);
        void taskStatusChanged(quint32 taskId, const QString &error);

        void apiError(const QString &info);

        void conquerableStationsChanged();
        void charactersChanged();
        void assetsChanged();
        void iskChanged();

    public slots:
        void refreshCharacters();
        void refreshCharacter(Character::IdType id, quint32 parentTask = TaskConstants::invalidTask);
        void refreshAssets(Character::IdType id, quint32 parentTask = TaskConstants::invalidTask);
        void refreshConquerableStations();

    private slots:
        void scheduleCharacterUpdate();
        void updateCharacters();

    private:
        static const QString versionKey;

        QSqlDatabase mMainDb, mEveDb;

        std::unique_ptr<KeyRepository> mKeyRepository;
        std::unique_ptr<CharacterRepository> mCharacterRepository;
        std::unique_ptr<ItemRepository> mItemRepository;
        std::unique_ptr<AssetListRepository> mAssetListRepository;
        std::unique_ptr<ConquerableStationRepository> mConquerableStationRepository;
        std::unique_ptr<EveTypeRepository> mEveTypeRepository;

        APIManager mAPIManager;

        quint32 mTaskId = TaskConstants::invalidTask + 1;

        bool mCharacterUpdateScheduled = false;

        mutable std::unordered_map<EveType::IdType, QString> mTypeNameCache;
        mutable std::unordered_map<quint64, QString> mLocationNameCache;

        void createDb();
        void createDbSchema();

        quint32 startTask(const QString &description);
        quint32 startTask(quint32 parentTask, const QString &description);

        void importCharacter(Character::IdType id, quint32 parentTask, const Key &key);

        Key getCharacterKey(Character::IdType id) const;

        static void showSplashMessage(const QString &message, QSplashScreen &splash);
    };
}
