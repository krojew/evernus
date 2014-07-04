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
#include "ButtonWithTimer.h"

#include "CharacterBoundWidget.h"

namespace Evernus
{
    CharacterBoundWidget::CharacterBoundWidget(const TimeGetter &timeGetter,
                                               QWidget *parent)
        : QWidget{parent}
        , mTimeGetter{timeGetter}
        , mImportBtn{new ButtonWithTimer{tr("API import"), this}}
    {
        connect(mImportBtn, &QPushButton::clicked, this, &CharacterBoundWidget::requestUpdate);
    }

    void CharacterBoundWidget::refreshImportTimer()
    {
        if (mCharacterId == Character::invalidId)
        {
            mImportBtn->setDisabled(true);
            mImportBtn->stopTimer();
        }
        else
        {
            mImportBtn->setEnabled(true);
            mImportBtn->setTimer(mTimeGetter(mCharacterId));
        }
    }

    void CharacterBoundWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        refreshImportTimer();
        handleNewCharacter(mCharacterId);
    }

    void CharacterBoundWidget::requestUpdate()
    {
        Q_ASSERT(mCharacterId != Character::invalidId);
        emit importFromAPI(mCharacterId);
    }

    ButtonWithTimer &CharacterBoundWidget::getAPIImportButton() const noexcept
    {
        return *mImportBtn;
    }

    Character::IdType CharacterBoundWidget::getCharacterId() const noexcept
    {
        return mCharacterId;
    }
}
