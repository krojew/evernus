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

#include <algorithm>
#include <cmath>

#include <QtGlobal>

namespace Evernus
{
    namespace MathUtils
    {
        template<class T>
        double calcPercentile(const T &orders,
                              quint64 maxVolume,
                              double avgPrice,
                              bool discardBogusOrders,
                              double bogusOrderThreshold)
        {
            if (orders.empty())
                return (std::isnan(avgPrice)) ? (0.) : (avgPrice);

            if (maxVolume == 0)
                maxVolume = 1;

            const auto nullAvg = qFuzzyIsNull(avgPrice);

            auto it = std::begin(orders);
            quint64 volume = 0u;
            auto result = 0.;

            while (volume < maxVolume && it != std::end(orders))
            {
                const auto price = it->get().getPrice();
                const quint64 orderVolume = it->get().getVolumeRemaining();
                const auto add = std::min(orderVolume, maxVolume - volume);

                if (!discardBogusOrders || nullAvg || fabs((price - avgPrice) / avgPrice) < bogusOrderThreshold)
                {
                    volume += add;
                    result += price * add;
                }
                else if (!nullAvg)
                {
                    maxVolume -= add;
                }

                ++it;
            }

            if (maxVolume == 0) // all bogus orders?
                return std::begin(orders)->get().getPrice();

            return result / maxVolume;
        }

        template<class T>
        size_t batchSize(T value) noexcept
        {
            return std::max(static_cast<size_t>(1u), std::min(static_cast<size_t>(exp(log(value) / 1.38629436112 - 1.)), static_cast<size_t>(300u)));
        }
    }
}
