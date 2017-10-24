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
#include "ESIExternalOrderImporter.h"

namespace Evernus
{
    ESIExternalOrderImporter::ESIExternalOrderImporter(QByteArray clientId,
                                                       QByteArray clientSecret,
                                                       const EveDataProvider &dataProvider,
                                                       const CharacterRepository &characterRepo,
                                                       ESIInterfaceManager &interfaceManager,
                                                       QObject *parent)
        : CallbackExternalOrderImporter{parent}
        , mManager{std::move(clientId), std::move(clientSecret), dataProvider, characterRepo, interfaceManager}
    {
        connect(&mManager, &ESIManager::error, this, &ESIExternalOrderImporter::genericError);
        connect(&mManager, &ESIManager::ssoAuthRequested, this, &ESIExternalOrderImporter::ssoAuthRequested);
    }

    void ESIExternalOrderImporter::processAuthorizationCode(Character::IdType charId, const QByteArray &code)
    {
        mManager.processAuthorizationCode(charId, code);
    }

    void ESIExternalOrderImporter::cancelSSOAuth(Character::IdType charId)
    {
        mManager.cancelSSOAuth(charId);
    }
}
