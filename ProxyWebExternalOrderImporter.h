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

#include "CRESTIndividualExternalOrderImporter.h"
#include "CRESTWholeExternalOrderImporter.h"
#include "EveCentralExternalOrderImporter.h"
#include "ImportSettings.h"

namespace Evernus
{
    class EveDataProvider;

    class ProxyWebExternalOrderImporter
        : public ExternalOrderImporter
    {
        Q_OBJECT

    public:
        explicit ProxyWebExternalOrderImporter(const EveDataProvider &dataProvider,
                                               QObject *parent = nullptr);
        virtual ~ProxyWebExternalOrderImporter() = default;

        virtual void fetchExternalOrders(const TypeLocationPairs &target) const override;

    public slots:
        void handleNewPreferences();

    private:
        std::unique_ptr<CRESTIndividualExternalOrderImporter> mCRESTIndividualImporter;
        std::unique_ptr<CRESTWholeExternalOrderImporter> mCRESTWholeImporter;
        std::unique_ptr<EveCentralExternalOrderImporter> mEveCentralImporter;

        ImportSettings::WebImporterType mCurrentImporter = ImportSettings::webImportTypeDefault;
        ImportSettings::MarketOrderImportType mCurrentOrderImportType = ImportSettings::marketOrderImportTypeDefault;

        template<class T>
        void connectImporter(T &importer);
        void setCurrentImporter();
    };
}
