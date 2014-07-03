#pragma once

#include <QFileSystemWatcher>
#include <QDateTime>
#include <QDialog>
#include <QHash>

#include "Character.h"

class QTableWidget;
class QRadioButton;
class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;
    class EveDataProvider;

    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        MarginToolDialog(const Repository<Character> &characterRepository,
                         const EveDataProvider &dataProvider,
                         QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void toggleAlwaysOnTop(int state);

        void refreshData(const QString &path);

        void saveCopyMode();

    private:
        typedef QHash<QString, QDateTime> FileModificationMap;

        struct Taxes
        {
            double mBrokerFee;
            double mSalesTax;
        };

        static const auto samples = 100000000;

        const Repository<Character> &mCharacterRepository;
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

        Taxes calculateTaxes() const;

        QTableWidget *createSampleTable();

        static void fillSampleData(QTableWidget &table, double revenue, double cos, int multiplier);

        static FileModificationMap getKnownFiles(const QString &path);

        static double getCoS(double buyPrice, const Taxes &taxes);
        static double getRevenue(double sellPrice, const Taxes &taxes);
    };
}
