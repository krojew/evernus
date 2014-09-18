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
#include <QColumnView>
#include <QTabWidget>
#include <QGroupBox>
#include <QSettings>
#include <QLabel>
#include <QDebug>
#include <QMenu>

#include "CacheTimerProvider.h"
#include "LMeveDataProvider.h"
#include "ButtonWithTimer.h"
#include "StyledTreeView.h"
#include "LMeveSettings.h"

#include "LMeveWidget.h"

namespace Evernus
{
    LMeveWidget::LMeveWidget(const CacheTimerProvider &cacheTimerProvider,
                             const EveDataProvider &dataProvider,
                             const LMeveDataProvider &lMeveDataProvider,
                             const ItemCostProvider &costProvider,
                             const CharacterRepository &characterRepository,
                             QWidget *parent)
        : QWidget(parent)
        , mCacheTimerProvider(cacheTimerProvider)
        , mLMeveDataProvider(lMeveDataProvider)
        , mTaskModel(dataProvider, costProvider, characterRepository)
        , mStationModel(dataProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

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
        qDebug() << "Switching LMeve to" << id;

        mCharacterId = id;
        mTaskModel.setCharacterId(mCharacterId);
        updateData();
    }

    void LMeveWidget::updateData()
    {
        refreshImportTimer();
        mTaskModel.setTasks(mLMeveDataProvider.getTasks(mCharacterId));
        mTaskView->header()->resizeSections(QHeaderView::ResizeToContents);
    }

    void LMeveWidget::prepareItemImportFromWeb()
    {
        emit importPricesFromWeb(getImportTarget());
    }

    void LMeveWidget::prepareItemImportFromFile()
    {
        emit importPricesFromFile(getImportTarget());
    }

    void LMeveWidget::prepareItemImportFromCache()
    {
        emit importPricesFromCache(getImportTarget());
    }

    void LMeveWidget::setStationId(const QModelIndex &index)
    {
        if (index.isValid())
        {
            const auto id = mStationModel.getStationId(index);
            mImportBtn->setEnabled(id != 0);
            mTaskModel.setStationId(id);

            QVariantList path;

            auto current = index;
            while (current.isValid())
            {
                path.prepend(mStationModel.getGenericId(current));
                current = mStationModel.parent(current);
            }

            QSettings settings;
            settings.setValue(LMeveSettings::sellStationKey, path);
        }
        else
        {
            mImportBtn->setEnabled(false);
        }
    }

    QWidget *LMeveWidget::createTaskTab()
    {
        auto container = new QWidget{this};
        auto containerLayout = new QHBoxLayout{container};

        auto stationGroup = new QGroupBox{tr("Sell station"), this};
        containerLayout->addWidget(stationGroup);

        auto stationLayout = new QVBoxLayout{stationGroup};

        auto stationView = new QColumnView{this};
        stationLayout->addWidget(stationView, 1);
        stationView->setModel(&mStationModel);
        stationView->setMaximumWidth(260);
        connect(stationView->selectionModel(), &QItemSelectionModel::currentChanged, this, &LMeveWidget::setStationId);

        auto importMenu = new QMenu{this};

        importMenu->addAction(QIcon{":/images/world.png"}, tr("Import prices from Web"), this, SLOT(prepareItemImportFromWeb()));
        importMenu->addAction(QIcon{":/images/page_refresh.png"}, tr("Import prices from logs"), this, SLOT(prepareItemImportFromFile()));
        importMenu->addAction(QIcon{":/images/disk_multiple.png"}, tr("Import prices from cache"), this, SLOT(prepareItemImportFromCache()));

        mImportBtn = new QPushButton{QIcon{":/images/arrow_refresh_small.png"}, tr("Import prices  "), this};
        stationLayout->addWidget(mImportBtn);
        mImportBtn->setFlat(true);
        mImportBtn->setMenu(importMenu);
        mImportBtn->setEnabled(false);

        mTaskProxy.setSourceModel(&mTaskModel);

        mTaskView = new StyledTreeView{"lmeve-tasks", this};
        containerLayout->addWidget(mTaskView, 1);
        mTaskView->setModel(&mTaskProxy);
        mTaskView->setRootIsDecorated(false);

        QSettings settings;

        const auto path = settings.value(LMeveSettings::sellStationKey).toList();
        if (path.size() == 4)
        {
            QModelIndex index;
            for (const auto &element : path)
                index = mStationModel.index(element.value<quint64>(), index);

            stationView->setCurrentIndex(index);
        }

        return container;
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

    ExternalOrderImporter::TypeLocationPairs LMeveWidget::getImportTarget() const
    {
        ExternalOrderImporter::TypeLocationPairs result;

        const auto stationId = mTaskModel.getStationId();
        if (stationId != 0)
        {
            const auto &tasks = mTaskModel.getTasks();
            for (const auto &task : tasks)
                result.emplace(std::make_pair(task->getTypeId(), stationId));
        }

        return result;
    }
}
