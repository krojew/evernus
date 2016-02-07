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
#include "ProxyWebExternalOrderImporter.h"

namespace Evernus
{
    ProxyWebExternalOrderImporter::ProxyWebExternalOrderImporter(const EveDataProvider &dataProvider,
                                                                 QObject *parent)
        : ExternalOrderImporter{parent}
        , mCRESTImporter{std::make_unique<CRESTExternalOrderImporter>(dataProvider, parent)}
    {
        connect(mCRESTImporter.get(), &CRESTExternalOrderImporter::externalOrdersChanged,
                this, &ProxyWebExternalOrderImporter::externalOrdersChanged);
        connect(mCRESTImporter.get(), &CRESTExternalOrderImporter::error,
                this, &ProxyWebExternalOrderImporter::error);
        connect(mCRESTImporter.get(), &CRESTExternalOrderImporter::statusChanged,
                this, &ProxyWebExternalOrderImporter::statusChanged);
    }

    void ProxyWebExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        mCRESTImporter->fetchExternalOrders(target);
    }

    void ProxyWebExternalOrderImporter::handleNewPreferences()
    {
        mCRESTImporter->handleNewPreferences();
    }
}
