#pragma once

#include <QFileSystemWatcher>
#include <QDialog>
#include <QSet>

#include "Character.h"

class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;

    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit MarginToolDialog(const Repository<Character> &characterRepository,
                                  QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void toggleAlwaysOnTop(int state);

        void refreshData(const QString &path);

    private:
        const Repository<Character> &mCharacterRepository;

        QFileSystemWatcher mWatcher;

        QLabel *mNameLabel = nullptr;
        QLabel *mBestBuyLabel = nullptr;
        QLabel *mBestSellLabel = nullptr;
        QLabel *mMarginLabel = nullptr;
        QLabel *mMarkupLabel = nullptr;

        QSet<QString> mKnownFiles;

        Character::IdType mCharacterId = Character::invalidId;

        void setNewWindowFlags(bool alwaysOnTop);

        static QSet<QString> getKnownFiles(const QString &path);
    };
}
