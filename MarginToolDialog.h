#pragma once

#include <QFileSystemWatcher>
#include <QDateTime>
#include <QDialog>
#include <QHash>

#include "Character.h"

class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;
    class NameProvider;

    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        MarginToolDialog(const Repository<Character> &characterRepository,
                         const NameProvider &nameProvider,
                         QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void toggleAlwaysOnTop(int state);

        void refreshData(const QString &path);

    private:
        typedef QHash<QString, QDateTime> FileModificationMap;

        struct Taxes
        {
            double mBrokerFee;
            double mSalesTax;
        };

        const Repository<Character> &mCharacterRepository;
        const NameProvider &mNameProvider;

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

        FileModificationMap mKnownFiles;

        Character::IdType mCharacterId = Character::invalidId;

        void setNewWindowFlags(bool alwaysOnTop);

        Taxes calculateTaxes() const;

        static FileModificationMap getKnownFiles(const QString &path);

        static double getCoS(double buyPrice, const Taxes &taxes);
        static double getRevenue(double sellPrice, const Taxes &taxes);
    };
}
