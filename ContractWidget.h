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

    class ContractWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        ContractWidget(const CacheTimerProvider &cacheTimerProvider, bool corp, QWidget *parent = nullptr);
        virtual ~ContractWidget() = default;

    public slots:
        void updateData();

    private:
        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
