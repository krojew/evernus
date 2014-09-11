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

#include <QSortFilterProxyModel>
#include <QWidget>

#include "LMeveTaskModel.h"
#include "Character.h"

namespace Evernus
{
    class CacheTimerProvider;
    class ButtonWithTimer;

    class LMeveWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        LMeveWidget(const CacheTimerProvider &cacheTimerProvider,
                    const EveDataProvider &dataProvider,
                    QWidget *parent = nullptr);
        virtual ~LMeveWidget() = default;

    signals:
        void syncLMeve(Character::IdType id);

        void openPreferences();

    public slots:
        void setCharacter(Character::IdType id);
        void updateData();

    private:
        const CacheTimerProvider &mCacheTimerProvider;

        ButtonWithTimer *mSyncBtn = nullptr;

        Character::IdType mCharacterId = Character::invalidId;

        LMeveTaskModel mTaskModel;
        QSortFilterProxyModel mTaskProxy;

        QWidget *createTaskTab();

        void refreshImportTimer();
    };
}
