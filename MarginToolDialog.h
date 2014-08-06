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

#include <unordered_set>
#include <vector>

#include <QFileSystemWatcher>
#include <QDateTime>
#include <QDialog>
#include <QHash>

#include "ExternalOrder.h"
#include "Character.h"

class QTableWidget;
class QRadioButton;
class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;
    class ItemCostProvider;
    class EveDataProvider;
    class ExternalOrder;

    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        MarginToolDialog(const Repository<Character> &characterRepository,
                         const ItemCostProvider &itemCostProvider,
                         const EveDataProvider &dataProvider,
                         QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;

    signals:
        void parsedData(const std::vector<ExternalOrder> &orders);

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void toggleAlwaysOnTop(int state);

        void refreshData(const QString &path);

        void saveCopyMode();

    protected:
        virtual void closeEvent(QCloseEvent *event) override;

    private:
        typedef QHash<QString, QDateTime> FileModificationMap;

        static const auto samples = 100000000;

        static const QString settingsPosKey;

        const Repository<Character> &mCharacterRepository;
        const ItemCostProvider &mItemCostProvider;
        const EveDataProvider &mDataProvider;

        QFileSystemWatcher mWatcher;

        QLabel *mNameLabel = nullptr;
        QLabel *mBestBuyLabel = nullptr;
        QLabel *mBestSellLabel = nullptr;
        QLabel *mProfitLabel = nullptr;
        QLabel *mRevenueLabel = nullptr;
        QLabel *mCostOfSalesLabel = nullptr;
        QLabel *mMarginLabel = nullptr;
        QLabel *mMarkupLabel = nullptr;
        QLabel *mBrokerFeeLabel = nullptr;
        QLabel *mSalesTaxLabel = nullptr;
        QLabel *mBuyVolLabel = nullptr;
        QLabel *mSellVolLabel = nullptr;
        QLabel *mBuyOrdersLabel = nullptr;
        QLabel *mSellOrdersLabel = nullptr;

        QRadioButton *mDontCopyBtn = nullptr;
        QRadioButton *mCopySellBtn = nullptr;
        QRadioButton *mCopyBuyBtn = nullptr;

        QTableWidget *m1SampleDataTable = nullptr;
        QTableWidget *m5SampleDataTable = nullptr;

        FileModificationMap mKnownFiles;

        Character::IdType mCharacterId = Character::invalidId;

        void setNewWindowFlags(bool alwaysOnTop);
        QTableWidget *createSampleTable();
        void savePosition() const;

        static void fillSampleData(QTableWidget &table, double revenue, double cos, int multiplier);

        static FileModificationMap getKnownFiles(const QString &path);
    };
}
