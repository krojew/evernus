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
        ESIInterfaceManager(const ESIInterfaceManager &) = default;
        ESIInterfaceManager(ESIInterfaceManager &&) = default;
        virtual ~ESIInterfaceManager() = default;

        void handleNewPreferences();

        ESIInterfaceManager &operator =(const ESIInterfaceManager &) = default;
        ESIInterfaceManager &operator =(ESIInterfaceManager &&) = default;

        static const ESIInterface &selectNextInterface();

    signals:
        void tokenRequested(Character::IdType charId) const;

        void tokenError(Character::IdType charId, const QString &error);
        void acquiredToken(Character::IdType charId, const QString &accessToken, const QDateTime &expiry);

    private:
        static std::vector<std::unique_ptr<ESIInterface, QObjectDeleteLaterDeleter>> mInterfaces;
        static std::size_t mCurrentInterface;

        void connectInterfaces();

        static void createInterfaces();
    };
}
