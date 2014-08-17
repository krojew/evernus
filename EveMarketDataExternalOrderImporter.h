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

#include "ExternalOrderImporter.h"
#include "ExternalOrder.h"

namespace Evernus
{
    class EveMarketDataExternalOrderImporter
        : public ExternalOrderImporter
    {
        Q_OBJECT

    public:
        using ExternalOrderImporter::ExternalOrderImporter;
        virtual ~EveMarketDataExternalOrderImporter() = default;

        virtual void fetchExternalOrders(const TypeLocationPairs &target) const override;

    private slots:
        void processReply() const;

    private:
        mutable QNetworkAccessManager mNetworkManager;
        mutable uint mRequestCount = 0;

        mutable std::vector<ExternalOrder> mResult;
    };
}
