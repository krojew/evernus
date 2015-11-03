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

#include <chrono>

namespace Evernus
{
    class RateLimiter
    {
    public:
        RateLimiter() = default;
        explicit RateLimiter(float rate);
        RateLimiter(const RateLimiter &) = default;
        RateLimiter(RateLimiter &&) = default;
        ~RateLimiter() = default;

        float getRate() const noexcept;
        void setRate(float rate);

        std::chrono::microseconds acquire(size_t permits = 1);

        RateLimiter &operator =(const RateLimiter &) = default;
        RateLimiter &operator =(RateLimiter &&) = default;

    private:
        static constexpr auto scale = 1000000.f;

        float mInterval = 1.f;

        std::chrono::microseconds::rep mNextFree = 0;
    };
}
