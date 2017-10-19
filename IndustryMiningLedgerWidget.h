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

#include "CharacterBoundWidget.h"

namespace Evernus
{
    class CacheTimerProvider;

    class IndustryMiningLedgerWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        explicit IndustryMiningLedgerWidget(const CacheTimerProvider &cacheTimerProvider,
                                            QWidget *parent = nullptr);
        IndustryMiningLedgerWidget(const IndustryMiningLedgerWidget &) = default;
        IndustryMiningLedgerWidget(IndustryMiningLedgerWidget &&) = default;
        virtual ~IndustryMiningLedgerWidget() = default;

        IndustryMiningLedgerWidget &operator =(const IndustryMiningLedgerWidget &) = default;
        IndustryMiningLedgerWidget &operator =(IndustryMiningLedgerWidget &&) = default;

    private:
        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
