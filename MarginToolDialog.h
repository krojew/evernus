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

    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit MarginToolDialog(const Repository<Character> &characterRepository,
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

        FileModificationMap mKnownFiles;

        Character::IdType mCharacterId = Character::invalidId;

        void setNewWindowFlags(bool alwaysOnTop);

        Taxes calculateTaxes() const;

        static FileModificationMap getKnownFiles(const QString &path);
    };
}
