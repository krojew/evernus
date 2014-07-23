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
#include <QCursor>
#include <QLabel>
#include <QFont>

#include "MarketOrderVolumeItemDelegate.h"
#include "MarketOrderInfoWidget.h"
#include "MarketOrderModel.h"
#include "StyledTreeView.h"

#include "MarketOrderView.h"

namespace Evernus
{
    MarketOrderView::MarketOrderView(const EveDataProvider &dataProvider, QWidget *parent)
        : QWidget{parent}
        , mProxy{dataProvider}
    {
        QFont font;
        font.setBold(true);

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mProxy.setSortRole(Qt::UserRole);
        mProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
        connect(this, &MarketOrderView::statusFilterChanged, &mProxy, &MarketOrderFilterProxyModel::setStatusFilter);
        connect(this, &MarketOrderView::priceStatusFilterChanged, &mProxy, &MarketOrderFilterProxyModel::setPriceStatusFilter);
        connect(this, &MarketOrderView::wildcardChanged, &mProxy, &MarketOrderFilterProxyModel::setFilterWildcard);

        mView = new StyledTreeView{this};
        mainLayout->addWidget(mView, 1);
        mView->setModel(&mProxy);
        mView->setItemDelegateForColumn(6, new MarketOrderVolumeItemDelegate{this});
        connect(mView, &StyledTreeView::clicked, this, &MarketOrderView::showPriceInfo);

        auto infoLayout = new QHBoxLayout{};
        mainLayout->addLayout(infoLayout);

        infoLayout->addWidget(new QLabel{tr("Orders:"), this});

        mTotalOrdersLabel = new QLabel{this};
        infoLayout->addWidget(mTotalOrdersLabel);
        mTotalOrdersLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Total volume:"), this});

        mVolumeLabel = new QLabel{this};
        infoLayout->addWidget(mVolumeLabel);
        mVolumeLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Total ISK in orders:"), this});

        mTotalISKLabel = new QLabel{this};
        infoLayout->addWidget(mTotalISKLabel);
        mTotalISKLabel->setFont(font);

        infoLayout->addWidget(new QLabel{tr("Total size:"), this});

        mTotalSizeLabel = new QLabel{this};
        infoLayout->addWidget(mTotalSizeLabel);
        mTotalSizeLabel->setFont(font);

        infoLayout->addStretch();
    }

    QItemSelectionModel *MarketOrderView::getSelectionModel() const
    {
        return mView->selectionModel();
    }

    const QAbstractProxyModel &MarketOrderView::getProxyModel() const
    {
        return mProxy;
    }

    void MarketOrderView::setModel(MarketOrderModel *model)
    {
        mSource = model;

        auto curModel = mProxy.sourceModel();
        if (curModel != nullptr)
            curModel->disconnect(this, SLOT(updateInfo()));

        mProxy.setSourceModel(mSource);
        mProxy.sort(0);

        connect(mSource, &MarketOrderModel::modelReset, this, &MarketOrderView::updateInfo);

        updateInfo();
    }

    void MarketOrderView::expandAll()
    {
        mView->expandAll();
        mView->header()->resizeSections(QHeaderView::ResizeToContents);
    }

    void MarketOrderView::updateInfo()
    {
        const auto volRemaining = mSource->getVolumeRemaining();
        const auto volEntered = mSource->getVolumeEntered();

        const auto curLocale = locale();

        emit closeOrderInfo();

        mTotalOrdersLabel->setText(curLocale.toString(static_cast<qulonglong>(mSource->getOrderCount())));
        mVolumeLabel->setText(QString{"%1/%2 (%3%)"}
            .arg(curLocale.toString(volRemaining))
            .arg(curLocale.toString(volEntered))
            .arg(curLocale.toString(volRemaining * 100. / volEntered, 'f', 1)));
        mTotalISKLabel->setText(curLocale.toCurrencyString(mSource->getTotalISK(), "ISK"));
        mTotalSizeLabel->setText(QString{"%1m³"}.arg(curLocale.toString(mSource->getTotalSize(), 'f', 2)));

        mView->header()->resizeSections(QHeaderView::ResizeToContents);
    }

    void MarketOrderView::showPriceInfo(const QModelIndex &index)
    {
        emit closeOrderInfo();

        const auto source = mProxy.mapToSource(index);

        if (!mSource->shouldShowPriceInfo(source))
            return;

        auto infoWidget = new MarketOrderInfoWidget{mSource->getOrderInfo(source), this};
        infoWidget->move(QCursor::pos());
        infoWidget->show();
        connect(this, &MarketOrderView::closeOrderInfo, infoWidget, &MarketOrderInfoWidget::deleteLater);
    }
}
