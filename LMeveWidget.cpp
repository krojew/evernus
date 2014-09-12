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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTabWidget>
#include <QLabel>

#include "CacheTimerProvider.h"
#include "LMeveDataProvider.h"
#include "ButtonWithTimer.h"
#include "StyledTreeView.h"

#include "LMeveWidget.h"

namespace Evernus
{
    LMeveWidget::LMeveWidget(const CacheTimerProvider &cacheTimerProvider,
                             const EveDataProvider &dataProvider,
                             const LMeveDataProvider &lMeveDataProvider,
                             QWidget *parent)
        : QWidget(parent)
        , mCacheTimerProvider(cacheTimerProvider)
        , mLMeveDataProvider(lMeveDataProvider)
        , mTaskModel(dataProvider)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolbarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolbarLayout);

        mSyncBtn = new ButtonWithTimer{tr("Synchronize"), this};
        toolbarLayout->addWidget(mSyncBtn);
        mSyncBtn->setEnabled(false);
        connect(mSyncBtn, &ButtonWithTimer::clicked, this, [this] {
            Q_ASSERT(mCharacterId != Character::invalidId);
            emit syncLMeve(mCharacterId);
        });

        auto infoLabel = new QLabel{tr("Before synchronizing, enter LMeve url and key in the <a href='#'>Preferences</a>."), this};
        toolbarLayout->addWidget(infoLabel);
        connect(infoLabel, &QLabel::linkActivated, this, &LMeveWidget::openPreferences);

        toolbarLayout->addStretch();

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs, 1);

        tabs->addTab(createTaskTab(), tr("Tasks"));

        mTaskProxy.setSortRole(Qt::UserRole);
    }

    void LMeveWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        updateData();
    }

    void LMeveWidget::updateData()
    {
        refreshImportTimer();
        mTaskModel.setTasks(mLMeveDataProvider.getTasks(mCharacterId));
        mTaskView->header()->resizeSections(QHeaderView::ResizeToContents);
    }

    QWidget *LMeveWidget::createTaskTab()
    {
        mTaskProxy.setSourceModel(&mTaskModel);

        mTaskView = new StyledTreeView{"lmeve-tasks", this};
        mTaskView->setModel(&mTaskProxy);
        mTaskView->setRootIsDecorated(false);

        return mTaskView;
    }

    void LMeveWidget::refreshImportTimer()
    {
        if (mCharacterId == Character::invalidId)
        {
            mSyncBtn->setDisabled(true);
            mSyncBtn->stopTimer();
        }
        else
        {
            mSyncBtn->setEnabled(true);
            mSyncBtn->setTimer(mCacheTimerProvider.getLocalCacheTimer(mCharacterId, TimerType::LMeveTasks));
        }
    }
}
