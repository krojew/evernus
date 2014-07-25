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
#include <QSettings>

#include "WarningBarWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"

#include "CharacterBoundWidget.h"

namespace Evernus
{
    CharacterBoundWidget::CharacterBoundWidget(const TimeGetter &apiTimeGetter,
                                               const TimeGetter &updateTimeGetter,
                                               const QString &updateAgeSettingsKey,
                                               QWidget *parent)
        : QWidget{parent}
        , mAPITimeGetter{apiTimeGetter}
        , mUpdateTimeGetter{updateTimeGetter}
        , mUpdateAgeSettingsKey{updateAgeSettingsKey}
        , mImportBtn{new ButtonWithTimer{tr("API import"), this}}
        , mWarningBarWidget{new WarningBarWidget{this}}
    {
        connect(&mUpdateTimer, &QTimer::timeout, this, &CharacterBoundWidget::updateTimerTick);
        connect(mImportBtn, &QPushButton::clicked, this, &CharacterBoundWidget::requestUpdate);

        mWarningBarWidget->setVisible(false);

        mUpdateTimer.start(1000 * 60);
        updateTimerTick();
    }

    void CharacterBoundWidget::refreshImportTimer()
    {
        if (mCharacterId == Character::invalidId)
        {
            mImportBtn->setDisabled(true);
            mImportBtn->stopTimer();
            mWarningBarWidget->hide();
        }
        else
        {
            mImportBtn->setEnabled(true);
            mImportBtn->setTimer(mAPITimeGetter(mCharacterId));
            updateTimerTick();
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

    void CharacterBoundWidget::updateTimerTick()
    {
        QSettings settings;

        const auto maxAge = settings.value(mUpdateAgeSettingsKey, ImportSettings::importTimerDefault).toInt();
        const auto curTime = QDateTime::currentDateTime();
        const auto lastUpdate = mUpdateTimeGetter(mCharacterId);

        if (!lastUpdate.isValid() || lastUpdate < curTime.addSecs(-60 * maxAge))
        {
            mWarningBarWidget->show();

            if (!lastUpdate.isValid())
            {
                mWarningBarWidget->setText(tr("<strong>Warning!</strong> No data has been imported."));
            }
            else
            {
                const auto secondsToString = [this](auto duration) {
                    QString res;

                    duration /= 60;

                    const auto minutes = duration % 60;
                    duration /= 60;

                    const auto hours = duration % 24;
                    const auto days = duration / 24;

                    if (hours == 0 && days == 0)
                        return res.sprintf(tr("%02dmin").toLatin1().data(), minutes);

                    if (days == 0)
                        return res.sprintf(tr("%02dh %02dmin").toLatin1().data(), hours, minutes);

                    return res.sprintf(tr("%dd %02dh").toLatin1().data(), days, hours);
                };

                mWarningBarWidget->setText(tr("<strong>Warning!</strong> This data is %1 old and may need an update.")
                    .arg(secondsToString((curTime.toMSecsSinceEpoch() - lastUpdate.toMSecsSinceEpoch()) / 1000)));
            }
        }
        else
        {
            mWarningBarWidget->hide();
        }
    }

    ButtonWithTimer &CharacterBoundWidget::getAPIImportButton() const noexcept
    {
        return *mImportBtn;
    }

    WarningBarWidget &CharacterBoundWidget::getWarningBarWidget() const noexcept
    {
        return *mWarningBarWidget;
    }

    Character::IdType CharacterBoundWidget::getCharacterId() const noexcept
    {
        return mCharacterId;
    }
}
