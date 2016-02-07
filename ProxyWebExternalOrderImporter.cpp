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

namespace Evernus
{
    ProxyWebExternalOrderImporter::ProxyWebExternalOrderImporter(const EveDataProvider &dataProvider,
                                                                 QObject *parent)
        : ExternalOrderImporter{parent}
        , mCRESTImporter{std::make_unique<CRESTExternalOrderImporter>(dataProvider, parent)}
        , mEveCentralImporter{std::make_unique<EveCentralExternalOrderImporter>(parent)}
    {
        setCurrentImporter();

        connectImporter(*mCRESTImporter);
        connectImporter(*mEveCentralImporter);
    }

    void ProxyWebExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (mCurrentImporter == ImportSettings::WebImporterType::EveCentral)
            mEveCentralImporter->fetchExternalOrders(target);
        else
            mCRESTImporter->fetchExternalOrders(target);
    }

    void ProxyWebExternalOrderImporter::handleNewPreferences()
    {
        setCurrentImporter();

        mCRESTImporter->handleNewPreferences();
        mEveCentralImporter->handleNewPreferences();
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
    }
}
