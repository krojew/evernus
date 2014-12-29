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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>
#include <QDebug>

#include <boost/scope_exit.hpp>

#include "ExternalOrderRepository.h"
#include "RegionTypeSelectDialog.h"
#include "PriceSettings.h"
#include "TaskManager.h"
#include "UISettings.h"

#include "MarketAnalysisWidget.h"

namespace Evernus
{
    MarketAnalysisWidget::MarketAnalysisWidget(QByteArray crestClientId,
                                               QByteArray crestClientSecret,
                                               const EveDataProvider &dataProvider,
                                               TaskManager &taskManager,
                                               const ExternalOrderRepository &orderRepo,
                                               QWidget *parent)
        : QWidget(parent)
        , mDataProvider(dataProvider)
        , mTaskManager(taskManager)
        , mOrderRepo(orderRepo)
        , mManager(std::move(crestClientId), std::move(crestClientSecret), mDataProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import order"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &MarketAnalysisWidget::prepareOrderImport);

        QSettings settings;

        mIgnoreExistingOrdersBtn = new QCheckBox{tr("Don't refresh existing up-to-date data"), this};
        toolBarLayout->addWidget(mIgnoreExistingOrdersBtn);
        mIgnoreExistingOrdersBtn->setChecked(
            settings.value(UISettings::ignoreExistingOrdersKey, UISettings::ignoreExistingOrdersDefault).toBool());
        connect(mIgnoreExistingOrdersBtn, &QCheckBox::toggled, [](auto checked) {
            QSettings settings;
            settings.setValue(UISettings::ignoreExistingOrdersKey, checked);
        });

        mDontSaveBtn = new QCheckBox{tr("Don't save imported orders (huge performance gain)"), this};
        toolBarLayout->addWidget(mDontSaveBtn);
        mDontSaveBtn->setChecked(
            settings.value(UISettings::dontSaveLargeOrdersKey, UISettings::dontSaveLargeOrdersDefault).toBool());
        connect(mDontSaveBtn, &QCheckBox::toggled, [](auto checked) {
            QSettings settings;
            settings.setValue(UISettings::dontSaveLargeOrdersKey, checked);
        });

        toolBarLayout->addStretch();
    }

    void MarketAnalysisWidget::prepareOrderImport()
    {
        RegionTypeSelectDialog dlg{mDataProvider, this};
        connect(&dlg, &RegionTypeSelectDialog::selected, this, &MarketAnalysisWidget::importOrders);

        dlg.exec();
    }

    void MarketAnalysisWidget::importOrders(const ExternalOrderImporter::TypeLocationPairs &pairs)
    {
        if (pairs.empty())
            return;

        ExternalOrderImporter::TypeLocationPairs ignored;
        if (mIgnoreExistingOrdersBtn->isChecked())
        {
            QSettings settings;
            const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();

            ignored = mOrderRepo.fetchDistinctTypesAndRegions(QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge));
        }

        mPreparingRequests = true;
        BOOST_SCOPE_EXIT(this_) {
            this_->mPreparingRequests = false;
        } BOOST_SCOPE_EXIT_END

        const auto mainTask = mTaskManager.startTask(tr("Importing data for analysis..."));

        mOrderSubtask = mTaskManager.startTask(mainTask, tr("Making %1 CREST order requests...").arg(pairs.size()));

        for (const auto &pair : pairs)
        {
            if (ignored.find(pair) != std::end(ignored))
                continue;

            ++mRequestCount;
            mManager.fetchMarketOrders(pair.second, pair.first, [this](auto &&orders, const auto &error) {
                processOrders(std::move(orders), error);
            });
        }

        qDebug() << "Making" << mRequestCount << "CREST order requests...";
    }

    void MarketAnalysisWidget::storeOrders()
    {
        emit updateExternalOrders(mResult);

        mTaskManager.endTask(mOrderSubtask);
        mResult.clear();
    }

    void MarketAnalysisWidget::processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText)
    {
        --mRequestCount;

        qDebug() << mRequestCount << " remaining:" << errorText;

        mTaskManager.updateTask(mOrderSubtask, tr("Waiting for %1 order server replies...").arg(mRequestCount));

        if (!errorText.isEmpty())
        {
            if (mRequestCount == 0)
            {
                mResult.clear();
                mTaskManager.endTask(mOrderSubtask, mAggregatedErrors.join("\n"));
                mAggregatedErrors.clear();
            }
            else
            {
                mAggregatedErrors << errorText;
            }

            return;
        }

        mResult.reserve(mResult.size() + orders.size());
        mResult.insert(std::end(mResult),
                       std::make_move_iterator(std::begin(orders)),
                       std::make_move_iterator(std::end(orders)));

        if (mRequestCount == 0)
        {
            if (!mPreparingRequests)
            {
                if (mAggregatedErrors.isEmpty())
                {
                    if (!mDontSaveBtn->isChecked())
                    {
                        mTaskManager.updateTask(mOrderSubtask, tr("Saving %1 imported orders...").arg(mResult.size()));
                        QMetaObject::invokeMethod(this, "storeOrders", Qt::QueuedConnection);
                    }
                    else
                    {
                        mTaskManager.endTask(mOrderSubtask);
                        mResult.clear();
                    }
                }
                else
                {
                    mTaskManager.endTask(mOrderSubtask, mAggregatedErrors.join("\n"));
                    mAggregatedErrors.clear();
                    mResult.clear();
                }
            }
        }
    }
}
