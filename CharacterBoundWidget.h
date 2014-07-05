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

#include <functional>

#include <QWidget>

#include "Character.h"

namespace Evernus
{
    class ButtonWithTimer;

    class CharacterBoundWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        typedef std::function<QDateTime (Character::IdType)> TimeGetter;

        explicit CharacterBoundWidget(const TimeGetter &timeGetter,
                                      QWidget *parent = nullptr);
        virtual ~CharacterBoundWidget() = default;

    signals:
        void importFromAPI(Character::IdType id);

    public slots:
        void refreshImportTimer();

        void setCharacter(Character::IdType id);

    private slots:
        void requestUpdate();

    protected:
        ButtonWithTimer &getAPIImportButton() const noexcept;
        Character::IdType getCharacterId() const noexcept;

    private:
        TimeGetter mTimeGetter;

        ButtonWithTimer *mImportBtn = nullptr;

        Character::IdType mCharacterId = Character::invalidId;

        virtual void handleNewCharacter(Character::IdType id) = 0;
    };
}
