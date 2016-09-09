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
#include <QDebug>

#include <boost/scope_exit.hpp>

#include "CRESTIndividualExternalOrderImporter.h"
#include "EveDataProvider.h"


namespace Evernus
{
    CRESTIndividualExternalOrderImporter
    ::CRESTIndividualExternalOrderImporter(QByteArray clientId,
                                           QByteArray clientSecret,
                                           const EveDataProvider &dataProvider,
                                           const CharacterRepository &characterRepo,
                                           QObject *parent)
        : CallbackExternalOrderImporter{parent}
        , mDataProvider{dataProvider}
        , mManager{std::move(clientId), std::move(clientSecret), mDataProvider, characterRepo}
    {
        connect(&mManager, &CRESTManager::error, this, &CRESTIndividualExternalOrderImporter::genericError);
    }

    void CRESTIndividualExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        mCounter.resetBatchIfEmpty();

        for (const auto &pair : target)
        {
            const auto regionId = mDataProvider.getStationRegionId(pair.second);
            if (regionId != 0)
            {
                mCounter.incCount();
                mManager.fetchMarketOrders(regionId, pair.first, [this](auto &&orders, const auto &error) {
                    processResult(std::move(orders), error);
                });
            }
        }

        qDebug() << "Making" << mCounter.getCount() << "CREST requests...";

        if (mCounter.isEmpty())
        {
            emit externalOrdersChanged(mResult);
            mResult.clear();
        }
    }

    void CRESTIndividualExternalOrderImporter::handleNewPreferences()
    {
        mManager.handleNewPreferences();
    }
}
