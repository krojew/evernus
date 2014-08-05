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
#include <cmath>

#include "Character.h"

#include "PriceUtils.h"

namespace Evernus
{
    namespace PriceUtils
    {
        Taxes calculateTaxes(const Character &character)
        {
            const auto feeSkills = character.getFeeSkills();
            const auto brokerFee = (0.01 - 0.0005 * feeSkills.mBrokerRelations) /
                                   std::exp(0.1 * character.getFactionStanding() + 0.04 * character.getCorpStanding());
            const auto salesTax = 0.015 * (1. - feeSkills.mAccounting * 0.1);

            return Taxes{brokerFee, salesTax};
        }

        double getCoS(double buyPrice, const Taxes &taxes)
        {
            return buyPrice + buyPrice * taxes.mBrokerFee;
        }

        double getRevenue(double sellPrice, const Taxes &taxes)
        {
            return sellPrice - sellPrice * taxes.mSalesTax - sellPrice * taxes.mBrokerFee;
        }
    }
}
