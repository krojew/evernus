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

#include <boost/optional.hpp>

#include <QDateTime>

#include "Character.h"

namespace Evernus
{
    struct WalletJournalData
    {
        typedef boost::optional<QString> ArgType;
        typedef boost::optional<QString> ReasonType;
        typedef boost::optional<quint64> TaxReceiverType;
        typedef boost::optional<double> TaxAmountType;

        Character::IdType mCharacterId = Character::invalidId;
        QDateTime mTimestamp;
        uint mRefTypeId = 0;
        QString mOwnerName1;
        quint64 mOwnerId1 = 0;
        QString mOwnerName2;
        quint64 mOwnerId2 = 0;
        ArgType mArgName;
        quint64 mArgId = 0;
        double mAmount = 0.;
        double mBalance = 0.;
        ReasonType mReason;
        TaxReceiverType mTaxReceiverId = 0;
        TaxAmountType mTaxAmount = 0.;
    };
}
