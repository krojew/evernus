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
#include <type_traits>
#include <stdexcept>

#include <QCoreApplication>

#include "RateLimiter.h"

namespace Evernus
{
    const float RateLimiter::scale = 1000000.f;

    RateLimiter::RateLimiter(float rate)
    {
        setRate(rate);
    }

    float RateLimiter::getRate() const noexcept
    {
        return scale / mInterval;
    }

    void RateLimiter::setRate(float rate)
    {
        if (rate <= 0.f)
            throw std::runtime_error{QCoreApplication::translate("RateLimiter", "Rate must be greater than 0!").toStdString()};

        mInterval = scale / rate;
    }

    std::chrono::microseconds RateLimiter::acquire(size_t permits)
    {
        using Clock = std::conditional<std::chrono::high_resolution_clock::is_steady,
                                       std::chrono::high_resolution_clock,
                                       std::chrono::steady_clock>::type;

        const auto now = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count();
        if (now > mNextFree)
            mNextFree = now;

        const auto wait = mNextFree - now;
        const auto adjustNext = permits * mInterval;

        mNextFree += adjustNext;

        return std::chrono::microseconds{wait};
    }
}
