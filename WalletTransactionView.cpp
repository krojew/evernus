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
#include <unordered_map>

#include <QSortFilterProxyModel>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QAction>

#include "WalletTransactionsModel.h"
#include "ItemCostProvider.h"
#include "PriceSettings.h"
#include "TextUtils.h"

#include "WalletTransactionView.h"

namespace Evernus
{
    WalletTransactionView::WalletTransactionView(ItemCostProvider &itemCostProvider, QWidget *parent)
        : StyledTreeView{parent}
        , mItemCostProvider{itemCostProvider}
    {
        initialize();
    }

    WalletTransactionView::WalletTransactionView(const QString &objectName, ItemCostProvider &itemCostProvider, QWidget *parent)
        : StyledTreeView{objectName, parent}
        , mItemCostProvider{itemCostProvider}
    {
        initialize();
    }

    void WalletTransactionView::setModel(QAbstractItemModel *model)
    {
        StyledTreeView::setModel(model);

        mModel = dynamic_cast<WalletTransactionsModel *>(model);
        mProxy = nullptr;

        auto selection = selectionModel();
        if (selection != nullptr && mModel != nullptr)
        {
            connect(selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &WalletTransactionView::selectTransaction);
        }
    }

    void WalletTransactionView::setModels(QAbstractProxyModel *proxy, WalletTransactionsModel *model)
    {
        StyledTreeView::setModel(proxy);

        mModel = model;
        mProxy = proxy;

        auto selection = selectionModel();
        if (selection != nullptr && mModel != nullptr)
        {
            connect(selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &WalletTransactionView::selectTransaction);
        }
    }

    void WalletTransactionView::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
    }

    void WalletTransactionView::selectTransaction(const QItemSelection &selected)
    {
        if (selected.isEmpty())
        {
            mCurrentTransaction = QModelIndex{};

            mCopySuggestedPriceAct->setText(getDefaultCopySuggestedPriceText());
            mCopySuggestedPriceAct->setEnabled(false);
        }
        else
        {
            mCurrentTransaction = (mProxy != nullptr) ?
                                  (mProxy->mapToSource(selected.indexes().first())) :
                                  (selected.indexes().first());

            const auto price = getSuggestedPrice(mModel->getPrice(mCurrentTransaction.row()));
            mCopySuggestedPriceAct->setText(tr("Copy suggested price: %1").arg(TextUtils::currencyToString(price, locale())));

            mCopySuggestedPriceAct->setEnabled(true);
        }
    }

    void WalletTransactionView::addItemCost()
    {
        const auto selection = selectionModel()->selectedIndexes();

        struct ItemData
        {
            uint mQuantity;
            double mPrice;
        };

        std::unordered_map<EveType::IdType, ItemData> aggrData;
        for (const auto &index : selection)
        {
            if (index.column() != 0)
                continue;

            const auto mappedIndex = (mProxy != nullptr) ? (mProxy->mapToSource(index)) : (index);
            const auto row = mappedIndex.row();

            auto &data = aggrData[mModel->getTypeId(row)];

            const auto quantity = mModel->getQuantity(row);

            data.mQuantity += quantity;
            data.mPrice += quantity * mModel->getPrice(row);
        }

        for (const auto &data : aggrData)
            mItemCostProvider.setForCharacterAndType(mCharacterId, data.first, data.second.mPrice / data.second.mQuantity);
    }

    void WalletTransactionView::copySuggestedPrice() const
    {
        const auto price = getSuggestedPrice(mModel->getPrice(mCurrentTransaction.row()));
        QApplication::clipboard()->setText(QString::number(price, 'f', 2));
    }

    double WalletTransactionView::getSuggestedPrice(double price) const
    {
        QSettings settings;
        const auto priceDelta = settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble();

        if (mModel->getType(mCurrentTransaction.row()) == WalletTransaction::Type::Buy)
            price += priceDelta;
        else
            price -= priceDelta;

        return price;
    }

    void WalletTransactionView::initialize()
    {
        auto separator = new QAction{this};
        separator->setSeparator(true);

        addAction(separator);

        auto addCostAct = new QAction{tr("Add to item costs"), this};
        connect(addCostAct, &QAction::triggered, this, &WalletTransactionView::addItemCost);

        addAction(addCostAct);

        mCopySuggestedPriceAct = new QAction{getDefaultCopySuggestedPriceText(), this};
        mCopySuggestedPriceAct->setEnabled(false);
        connect(mCopySuggestedPriceAct, &QAction::triggered, this, &WalletTransactionView::copySuggestedPrice);

        addAction(mCopySuggestedPriceAct);
    }

    QString WalletTransactionView::getDefaultCopySuggestedPriceText()
    {
        return tr("Copy suggested price");
    }
}
