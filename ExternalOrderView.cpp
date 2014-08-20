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
#include <QLabel>
#include <QFont>

#include "MarketOrderVolumeItemDelegate.h"
#include "ExternalOrderModel.h"
#include "ItemCostProvider.h"
#include "StyledTreeView.h"

#include "ExternalOrderView.h"

namespace Evernus
{
    ExternalOrderView::ExternalOrderView(const ItemCostProvider &costProvider, const EveDataProvider &dataProvider, QWidget *parent)
        : QWidget(parent)
        , mCostProvider(costProvider)
        , mProxy(dataProvider)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mProxy.setSortRole(Qt::UserRole);
        connect(&mProxy, &QSortFilterProxyModel::modelReset, this, &ExternalOrderView::handleModelReset, Qt::QueuedConnection);

        mView = new StyledTreeView{this};
        mainLayout->addWidget(mView, 1);
        mView->setModel(&mProxy);
        mView->setRootIsDecorated(false);

        auto infoLayout = new QHBoxLayout{};
        mainLayout->addLayout(infoLayout);

        QFont font;
        font.setBold(true);

        infoLayout->addWidget(new QLabel{tr("Total cost:"), this});

        mTotalPriceLabel = new QLabel{"-", this};
        infoLayout->addWidget(mTotalPriceLabel);
        mTotalPriceLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Total volume:"), this});

        mTotalVolumeLabel = new QLabel{"-", this};
        infoLayout->addWidget(mTotalVolumeLabel);
        mTotalVolumeLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Total size:"), this});

        mTotalSizeLabel = new QLabel{"-", this};
        infoLayout->addWidget(mTotalSizeLabel);
        mTotalSizeLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Min. price:"), this});

        mMinPriceLabel = new QLabel{"-", this};
        infoLayout->addWidget(mMinPriceLabel);
        mMinPriceLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Median price:"), this});

        mMedianPriceLabel = new QLabel{"-", this};
        infoLayout->addWidget(mMedianPriceLabel);
        mMedianPriceLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Max. price:"), this});

        mMaxPriceLabel = new QLabel{"-", this};
        infoLayout->addWidget(mMaxPriceLabel);
        mMaxPriceLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Custom cost:"), this});

        mItemCostLabel = new QLabel{"-", this};
        infoLayout->addWidget(mItemCostLabel);
        mItemCostLabel->setFont(font);

        infoLayout->addStretch();
    }

    void ExternalOrderView::setModel(ExternalOrderModel *model)
    {
        mSource = model;
        mProxy.setSourceModel(mSource);

        if (mSource != nullptr)
        {
            mView->sortByColumn(mSource->getPriceColumn(), mSource->getPriceSortOrder());
            mView->setItemDelegateForColumn(mSource->getVolumeColumn(), new MarketOrderVolumeItemDelegate{this});
        }
    }

    void ExternalOrderView::setCharacterId(Character::IdType id)
    {
        mCharacterId = id;
        setCustomCost();
    }

    void ExternalOrderView::setTypeId(EveType::IdType id)
    {
        mTypeId = id;
        setCustomCost();
    }

    void ExternalOrderView
    ::setFilter(double minPrice, double maxPrice, uint minVolume, uint maxVolume, ExternalOrderFilterProxyModel::SecurityStatuses security)
    {
        mProxy.setFilter(minPrice, maxPrice, minVolume, maxVolume, security);
    }

    void ExternalOrderView::handleModelReset()
    {
        const auto curLocale = locale();

        if (mSource != nullptr)
        {
            mTotalPriceLabel->setText(curLocale.toCurrencyString(mSource->getTotalPrice(), "ISK"));
            mTotalVolumeLabel->setText(curLocale.toString(mSource->getTotalVolume()));
            mTotalSizeLabel->setText(QString{"%1m³"}.arg(curLocale.toString(mSource->getTotalSize(), 'f', 2)));
            mMinPriceLabel->setText(curLocale.toCurrencyString(mSource->getMinPrice(), "ISK"));
            mMedianPriceLabel->setText(curLocale.toCurrencyString(mSource->getMedianPrice(), "ISK"));
            mMaxPriceLabel->setText(curLocale.toCurrencyString(mSource->getMaxPrice(), "ISK"));
        }

        mView->header()->resizeSections(QHeaderView::ResizeToContents);
    }

    void ExternalOrderView::setCustomCost()
    {
        if (mCharacterId == Character::invalidId || mTypeId == EveType::invalidId)
        {
            mItemCostLabel->setText(tr("N/A"));
        }
        else
        {
            const auto cost = mCostProvider.fetchForCharacterAndType(mCharacterId, mTypeId);
            if (cost->isNew())
                mItemCostLabel->setText(tr("N/A"));
            else
                mItemCostLabel->setText(locale().toCurrencyString(cost->getCost(), "ISK"));
        }
    }
}
