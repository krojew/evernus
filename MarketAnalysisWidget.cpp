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

#include "ExternalOrderRepository.h"
#include "RegionTypeSelectDialog.h"

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

        mIgnoreExistingOrdersBtn = new QCheckBox{tr("Keep already imported data"), this};
        toolBarLayout->addWidget(mIgnoreExistingOrdersBtn);
        mIgnoreExistingOrdersBtn->setChecked(true);

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
            ignored = mOrderRepo.fetchDistinctTypesAndRegions();

        for (const auto &pair : pairs)
        {
            if (ignored.find(pair) != std::end(ignored))
                continue;

            mManager.fetchMarketOrders(pair.second, pair.first, [this](auto &&orders, const auto &error) {
                processOrders(std::move(orders), error);
            });
        }
    }

    void MarketAnalysisWidget::processOrders(std::vector<ExternalOrder> &&orders, const QString &errorText)
    {

    }
}
