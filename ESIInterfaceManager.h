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
#pragma once

#include <atomic>
#include <memory>

#include <QDateTime>
#include <QObject>
#include <QString>

#include "QObjectDeleteLaterDeleter.h"
#include "Character.h"

namespace Evernus
{
    class ESIInterface;

    class ESIInterfaceManager final
        : public QObject
    {
        Q_OBJECT

    public:
        explicit ESIInterfaceManager(QObject *parent = nullptr);
        ESIInterfaceManager(const ESIInterfaceManager &) = delete;
        ESIInterfaceManager(ESIInterfaceManager &&) = default;
        virtual ~ESIInterfaceManager() = default;

        void handleNewPreferences();

        const ESIInterface &selectNextInterface();

        ESIInterfaceManager &operator =(const ESIInterfaceManager &) = delete;
        ESIInterfaceManager &operator =(ESIInterfaceManager &&) = default;

    signals:
        void tokenRequested(Character::IdType charId) const;

        void tokenError(Character::IdType charId, const QString &error);
        void acquiredToken(Character::IdType charId, const QString &accessToken, const QDateTime &expiry);

    private:
        std::vector<std::unique_ptr<ESIInterface, QObjectDeleteLaterDeleter>> mInterfaces;
        std::atomic_size_t mCurrentInterface{0};

        void connectInterfaces();
        void createInterfaces();
    };
}
