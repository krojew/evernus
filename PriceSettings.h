#pragma once

namespace Evernus
{
    namespace PriceSettings
    {
        enum class CopyMode
        {
            DontCopy,
            CopySell,
            CopyBuy
        };

        const auto minMarginDefault = 10.;
        const auto preferredMarginDefault = 30.;
        const auto priceDeltaDefault = 0.01;

        const auto minMarginKey = "prices/margin/min";
        const auto preferredMarginKey = "prices/margin/preferred";
        const auto copyModeKey = "prices/copyMode";
        const auto priceDeltaKey = "prices/delta";
    }
}
