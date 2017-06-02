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
#include <QSettings>

#include "ProxyWebExternalOrderImporter.h"
#include "SSOUtils.h"

namespace Evernus
{
    ProxyWebExternalOrderImporter::ProxyWebExternalOrderImporter(QByteArray clientId,
                                                                 QByteArray clientSecret,
                                                                 const EveDataProvider &dataProvider,
                                                                 const CharacterRepository &characterRepo,
                                                                 QObject *parent)
        : ExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
        , mESIIndividualImporter{std::make_unique<ESIIndividualExternalOrderImporter>(clientId, clientSecret, mDataProvider, characterRepo, parent)}
        , mESIWholeImporter{std::make_unique<ESIWholeExternalOrderImporter>(std::move(clientId), std::move(clientSecret), mDataProvider, characterRepo, parent)}
        , mEveCentralImporter{std::make_unique<EveCentralExternalOrderImporter>(mDataProvider, parent)}
    {
        setCurrentImporter();

        connectImporter(*mESIIndividualImporter);
        connectImporter(*mESIWholeImporter);
        connectImporter(*mEveCentralImporter);
    }

    void ProxyWebExternalOrderImporter::fetchExternalOrders(Character::IdType id, const TypeLocationPairs &target) const
    {
        if (mCurrentOrderImportType == ImportSettings::MarketOrderImportType::Auto)
        {
            if (SSOUtils::useWholeMarketImport(target, mDataProvider))
                mESIWholeImporter->fetchExternalOrders(id, target);
            else
                mESIIndividualImporter->fetchExternalOrders(id, target);
        }
        else if (mCurrentImporter == ImportSettings::WebImporterType::EveCentral)
        {
            mEveCentralImporter->fetchExternalOrders(id, target);
        }
        else if (mCurrentOrderImportType == ImportSettings::MarketOrderImportType::Individual)
        {
            mESIIndividualImporter->fetchExternalOrders(id, target);
        }
        else
        {
            mESIWholeImporter->fetchExternalOrders(id, target);
        }
    }

    void ProxyWebExternalOrderImporter::handleNewPreferences()
    {
        mESIIndividualImporter->handleNewPreferences();
        mESIWholeImporter->handleNewPreferences();

        setCurrentImporter();
    }

    template<class T>
    void ProxyWebExternalOrderImporter::connectImporter(T &importer)
    {
        connect(&importer, &T::externalOrdersChanged,
                this, &ProxyWebExternalOrderImporter::externalOrdersChanged);
        connect(&importer, &T::error,
                this, &ProxyWebExternalOrderImporter::error);
        connect(&importer, &T::statusChanged,
                this, &ProxyWebExternalOrderImporter::statusChanged);
    }

    void ProxyWebExternalOrderImporter::setCurrentImporter()
    {
        QSettings settings;

        mCurrentImporter
            = static_cast<ImportSettings::WebImporterType>(
                settings.value(ImportSettings::webImportTypeKey, static_cast<int>(ImportSettings::webImportTypeDefault)).toInt());
        mCurrentOrderImportType
            = static_cast<ImportSettings::MarketOrderImportType>(
                settings.value(ImportSettings::marketOrderImportTypeKey, static_cast<int>(ImportSettings::marketOrderImportTypeDefault)).toInt());
    }
}
