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
#include <QDesktopServices>
#include <QActionGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSettings>
#include <QCursor>
#include <QLabel>
#include <QFont>
#include <QUrl>

#include "MarketOrderVolumeItemDelegate.h"
#include "MarketOrderInfoWidget.h"
#include "MarketOrderModel.h"
#include "StyledTreeView.h"
#include "UISettings.h"

#include "MarketOrderView.h"

namespace Evernus
{
    MarketOrderView::MarketOrderView(const EveDataProvider &dataProvider, QWidget *parent)
        : QWidget(parent)
        , mProxy(dataProvider)
    {
        QFont font;
        font.setBold(true);

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mProxy.setSortRole(Qt::UserRole);
        mProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
        connect(this, &MarketOrderView::statusFilterChanged, &mProxy, &MarketOrderFilterProxyModel::setStatusFilter);
        connect(this, &MarketOrderView::priceStatusFilterChanged, &mProxy, &MarketOrderFilterProxyModel::setPriceStatusFilter);
        connect(this, &MarketOrderView::textFilterChanged, &mProxy, &MarketOrderFilterProxyModel::setTextFilter);
        connect(&mProxy, &MarketOrderFilterProxyModel::scriptError, this, &MarketOrderView::scriptError);

        mView = new StyledTreeView{this};
        mainLayout->addWidget(mView, 1);
        mView->setModel(&mProxy);
        connect(mView, &StyledTreeView::clicked, this, &MarketOrderView::showPriceInfo);
        connect(mView->header(), &QHeaderView::sectionMoved, this, &MarketOrderView::saveHeaderState);
        connect(mView->header(), &QHeaderView::sectionResized, this, &MarketOrderView::saveHeaderState);

        mInfoWidget = new QWidget{this};
        mainLayout->addWidget(mInfoWidget);

        auto infoLayout = new QHBoxLayout{};
        mInfoWidget->setLayout(infoLayout);
        infoLayout->setContentsMargins(QMargins{});

        infoLayout->addWidget(new QLabel{tr("Active orders:"), this});

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

        auto actionGroup = new QActionGroup{this};

        auto action = actionGroup->addAction(tr("Lookup item on eve-marketdata.com"));
        connect(action, &QAction::triggered, this, &MarketOrderView::lookupOnEveMarketdata);

        action = actionGroup->addAction(tr("Lookup item on eve-central.com"));
        connect(action, &QAction::triggered, this, &MarketOrderView::lookupOnEveCentral);

        action = new QAction{this};
        action->setSeparator(true);

        mView->addAction(action);
        mView->addActions(actionGroup->actions());
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
        if (mSource == model)
            return;

        mSource = model;

        auto curModel = mProxy.sourceModel();
        if (curModel != nullptr)
            curModel->disconnect(this, SLOT(updateInfo()));

        mProxy.setSourceModel(mSource);

        mView->setItemDelegateForColumn(model->getVolumeColumn(), new MarketOrderVolumeItemDelegate{this});

        if (mSource != nullptr)
        {
            QSettings settings;
            mView->header()->restoreState(
                settings.value(QString{UISettings::orderViewHeaderStateKey}.arg(static_cast<int>(mSource->getType()))).toByteArray());

            connect(mSource, &MarketOrderModel::modelReset, this, &MarketOrderView::updateInfo);
        }

        updateInfo();
    }

    void MarketOrderView::setShowInfo(bool flag)
    {
        mInfoWidget->setVisible(flag);
    }

    void MarketOrderView::expandAll()
    {
        mView->expandAll();
        mView->header()->resizeSections(QHeaderView::ResizeToContents);
    }

    void MarketOrderView::sortByColumn(int column, Qt::SortOrder order)
    {
        mView->sortByColumn(column, order);
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
            .arg(curLocale.toString((volEntered > 0.) ? (volRemaining * 100. / volEntered) : (0.), 'f', 1)));
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
        infoWidget->activateWindow();
        connect(this, &MarketOrderView::closeOrderInfo, infoWidget, &MarketOrderInfoWidget::deleteLater);
    }

    void MarketOrderView::lookupOnEveMarketdata()
    {
        lookupOnWeb("http://eve-marketdata.com/price_check.php?type_id=%1");
    }

    void MarketOrderView::lookupOnEveCentral()
    {
        lookupOnWeb("https://eve-central.com/home/quicklook.html?typeid=%1");
    }

    void MarketOrderView::saveHeaderState()
    {
        if (mSource != nullptr)
        {
            QSettings settings;
            settings.setValue(QString{UISettings::orderViewHeaderStateKey}.arg(static_cast<int>(mSource->getType())),
                              mView->header()->saveState());
        }
    }

    void MarketOrderView::lookupOnWeb(const QString &baseUrl) const
    {
        if (mSource == nullptr)
            return;

        const auto typeId = mSource->getOrderTypeId(mProxy.mapToSource(mView->currentIndex()));
        if (typeId != EveType::invalidId)
            QDesktopServices::openUrl(baseUrl.arg(typeId));
    }
}
