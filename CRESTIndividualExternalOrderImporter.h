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

#include <QNetworkAccessManager>
#include <QStringList>

#include "CallbackExternalOrderImporter.h"
#include "CRESTManager.h"

namespace Evernus
{
    class CharacterRepository;
    class EveDataProvider;

    class CRESTIndividualExternalOrderImporter
        : public CallbackExternalOrderImporter
    {
        Q_OBJECT

    public:
        CRESTIndividualExternalOrderImporter(QByteArray clientId,
                                             QByteArray clientSecret,
                                             const EveDataProvider &dataProvider,
                                             const CharacterRepository &characterRepo,
                                             QObject *parent = nullptr);
        virtual ~CRESTIndividualExternalOrderImporter() = default;

        virtual void fetchExternalOrders(const TypeLocationPairs &target) const override;

    public slots:
        void handleNewPreferences();

    private:
        const EveDataProvider &mDataProvider;

        CRESTManager mManager;
    };
}
