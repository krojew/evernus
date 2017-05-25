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
#include <random>
#include <cmath>

#include <QSettings>

#include "PriceSettings.h"
#include "Character.h"

#include "PriceUtils.h"

namespace Evernus
{
    namespace PriceUtils
    {
        Taxes calculateTaxes(const Character &character)
        {
            const auto feeSkills = character.getFeeSkills();
            const auto salesTax = 0.02 - feeSkills.mAccounting * 0.002;

            auto calcBrokersFee = [&](auto customBrokersFee) {
                return (customBrokersFee) ? (*customBrokersFee) : (0.03 - (feeSkills.mBrokerRelations * 0.001 +
                    0.0003 * character.getFactionStanding() + 0.0002 * character.getCorpStanding()));
            };

            return Taxes{calcBrokersFee(character.getBuyBrokersFee()), calcBrokersFee(character.getSellBrokersFee()), salesTax};
        }

        double getCoS(double buyPrice, const Taxes &taxes, bool limitOrder)
        {
            return (limitOrder) ? (buyPrice + buyPrice * taxes.mBuyBrokerFee) : (buyPrice);
        }

        double getRevenue(double sellPrice, const Taxes &taxes, bool limitOrder)
        {
            return sellPrice - sellPrice * taxes.mSalesTax - ((limitOrder) ? (sellPrice * taxes.mSellBrokerFee) : (0.));
        }

        double getMargin(double cost, double price, const Taxes &taxes, bool limitOrder)
        {
            const auto realCost = getCoS(cost, taxes, limitOrder);
            const auto realPrice = getRevenue(price, taxes, limitOrder);
            return 100. * (realPrice - realCost) / realPrice;
        }

        double getPriceDelta()
        {
            QSettings settings;

            const auto priceDeltaRandom
                = settings.value(PriceSettings::priceDeltaRandomKey, PriceSettings::priceDeltaRandomDefault).toDouble();
            auto priceDelta = settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble();

            if (!qFuzzyIsNull(priceDeltaRandom))
            {
                std::random_device rd;
                std::mt19937 gen{rd()};
                std::uniform_real_distribution<> dis(0., priceDeltaRandom);

                priceDelta += dis(gen);
            }

            return priceDelta;
        }
    }
}
