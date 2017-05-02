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
#include <QLineSeries>
#include <QBarSeries>
#include <QSettings>
#include <QSqlQuery>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QBarSet>
#include <QLabel>
#include <QDebug>
#include <QHash>
#include <QFont>

#include "WalletTransactionRepository.h"
#include "EveDataProvider.h"
#include "UISettings.h"
#include "TextUtils.h"

#include "ItemHistoryWidget.h"

namespace Evernus
{
    ItemHistoryWidget::ItemHistoryWidget(const WalletTransactionRepository &walletRepo,
                                         const WalletTransactionRepository &corpWalletRepo,
                                         const EveDataProvider &dataProvider,
                                         QWidget *parent)
        : QWidget(parent)
        , mWalletRepo(walletRepo)
        , mCorpWalletRepo(corpWalletRepo)
        , mDataProvider(dataProvider)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto itemGroup = new QGroupBox{this};
        mainLayout->addWidget(itemGroup);

        auto itemLayout = new QHBoxLayout{itemGroup};

        itemLayout->addWidget(new QLabel{tr("Item type:"), this});

        mItemTypeCombo = new QComboBox{this};
        itemLayout->addWidget(mItemTypeCombo);
        mItemTypeCombo->setEditable(true);
        mItemTypeCombo->setInsertPolicy(QComboBox::NoInsert);
        mItemTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(mItemTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(computeValues()));

        mAllCharactersBtn = new QCheckBox{tr("Combine for all characters"), this};
        itemLayout->addWidget(mAllCharactersBtn);
        connect(mAllCharactersBtn, &QCheckBox::toggled, this, &ItemHistoryWidget::updateData);

        itemLayout->addStretch();

        mChart = new ZoomableChartView{this};
        mainLayout->addWidget(mChart);
        mChart->setMinimumHeight(300);

        auto locale = mChart->locale();
        locale.setNumberOptions(0);
        mChart->setLocale(locale);

        mDateAxis = new QBarCategoryAxis{this};
        mDateAxis->setLabelsAngle(60);
        mChart->chart()->addAxis(mDateAxis, Qt::AlignBottom);

        mBalanceAxis = new QValueAxis{this};
        mBalanceAxis->setTitleText(tr("ISK"));
        mChart->chart()->addAxis(mBalanceAxis, Qt::AlignLeft);

        mVolumeAxis = new QValueAxis{this};
        mVolumeAxis->setTitleText(tr("Volume"));
        mChart->chart()->addAxis(mVolumeAxis, Qt::AlignRight);

        auto totalGroup = new QGroupBox{this};
        mainLayout->addWidget(totalGroup);

        auto totalLayout = new QHBoxLayout{totalGroup};

        QFont font;
        font.setBold(true);

        totalLayout->addWidget(new QLabel{tr("Total income:"), this}, 0, Qt::AlignRight);

        mTotalIncomeLabel = new QLabel{"-", this};
        totalLayout->addWidget(mTotalIncomeLabel, 0, Qt::AlignLeft);
        mTotalIncomeLabel->setFont(font);

        totalLayout->addWidget(new QLabel{tr("Total cost:"), this}, 0, Qt::AlignRight);

        mTotalOutcomeLabel = new QLabel{"-", this};
        totalLayout->addWidget(mTotalOutcomeLabel, 0, Qt::AlignLeft);
        mTotalOutcomeLabel->setFont(font);

        totalLayout->addWidget(new QLabel{tr("Balance:"), this}, 0, Qt::AlignRight);

        mTotalBalanceLabel = new QLabel{"-", this};
        totalLayout->addWidget(mTotalBalanceLabel, 0, Qt::AlignLeft);
        mTotalBalanceLabel->setFont(font);

        totalLayout->addWidget(new QLabel{tr("Margin:"), this}, 0, Qt::AlignRight);

        mTotalMarginLabel = new QLabel{"-", this};
        totalLayout->addWidget(mTotalMarginLabel, 0, Qt::AlignLeft);
        mTotalMarginLabel->setFont(font);

        totalLayout->addWidget(new QLabel{tr("Total volume:"), this}, 0, Qt::AlignRight);

        mTotalVolumeLabel = new QLabel{"-", this};
        totalLayout->addWidget(mTotalVolumeLabel, 0, Qt::AlignLeft);
        mTotalVolumeLabel->setFont(font);

        mainLayout->addStretch();

        setNumberFormat();
    }

    void ItemHistoryWidget::setCharacter(Character::IdType id)
    {
        qDebug() << "Switching to character" << id;

        mCharacterId = id;
        if (!mAllCharactersBtn->isChecked())
            updateData();
    }

    void ItemHistoryWidget::updateData()
    {
        QSqlQuery query{mWalletRepo.getDatabase()};

        if (mAllCharactersBtn->isChecked())
        {
            query.prepare(QString{"SELECT DISTINCT ids.type_id FROM ("
                "SELECT type_id FROM %1 "
                "UNION "
                "SELECT type_id FROM %2"
            ") ids"}.arg(mWalletRepo.getTableName()).arg(mCorpWalletRepo.getTableName()));
        }
        else
        {
            query.prepare(QString{"SELECT DISTINCT ids.type_id FROM ("
                "SELECT type_id, character_id FROM %1 "
                "UNION "
                "SELECT type_id, character_id FROM %2"
            ") ids WHERE ids.character_id = ?"}.arg(mWalletRepo.getTableName()).arg(mCorpWalletRepo.getTableName()));
            query.bindValue(0, mCharacterId);
        }

        DatabaseUtils::execQuery(query);

        mItemTypeCombo->clear();
        while (query.next())
        {
            const auto id = query.value(0).value<EveType::IdType>();
            mItemTypeCombo->addItem(mDataProvider.getTypeName(id), id);
        }

        mItemTypeCombo->model()->sort(0);
    }

    void ItemHistoryWidget::handleNewPreferences()
    {
        setNumberFormat();
        setMarginColor();
    }

    void ItemHistoryWidget::computeValues()
    {
        const auto id = mItemTypeCombo->currentData().value<EveType::IdType>();

        QHash<QDate, std::pair<double, uint>> values;
        uint totalVolume = 0;

        mTotalIncome = mTotalOutcome = 0.;

        const auto inserter = [&](const auto &entries) {
            for (const auto &entry : entries)
            {
                if (entry->isIgnored())
                    continue;

                auto &value = values[entry->getTimestamp().toLocalTime().date()];

                const auto volume = entry->getQuantity();
                const auto amount = entry->getPrice() * volume;

                if (entry->getType() == Evernus::WalletTransaction::Type::Buy)
                {
                    value.first -= amount;
                    mTotalOutcome += amount;
                }
                else
                {
                    value.first += amount;
                    mTotalIncome += amount;
                }

                value.second += volume;
                totalVolume += volume;
            }
        };

        if (mAllCharactersBtn->isChecked())
        {
            inserter(mWalletRepo.fetchForTypeId(id));
            inserter(mCorpWalletRepo.fetchForTypeId(id));
        }
        else
        {
            inserter(mWalletRepo.fetchForTypeIdAndCharacter(id, mCharacterId));
            inserter(mCorpWalletRepo.fetchForTypeIdAndCharacter(id, mCharacterId));
        }

        const auto curLocale = locale();

        const auto chart = mChart->chart();
        Q_ASSERT(chart != nullptr);

        chart->removeAllSeries();

        QStringList dates;
        dates.reserve(values.size());

        const auto dateFormat = curLocale.dateFormat(QLocale::NarrowFormat);
        auto i = 0;

        const auto volumeSet = new QBarSet{tr("Volume"), this};
        volumeSet->setColor(Qt::cyan);

        const auto balanceSeries = new QLineSeries{this};
        balanceSeries->setName(tr("Balance"));

        auto minBalance = 0., maxBalance = 0., maxVolume = 0.;

        QHashIterator<QDate, std::pair<double, uint>> it{values};
        while (it.hasNext())
        {
            it.next();

            const auto balance = it.value().first;
            const auto volume = it.value().second;

            dates << it.key().toString(dateFormat);
            balanceSeries->append(i++, balance);
            volumeSet->append(volume);

            if (balance > maxBalance)
                maxBalance = balance;
            if (balance < minBalance)
                minBalance = balance;

            if (volume > maxVolume)
                maxVolume = volume;
        }

        const auto volumeSeries = new QBarSeries{this};
        volumeSeries->append(volumeSet);

        mBalanceAxis->setRange(minBalance, maxBalance);
        mVolumeAxis->setRange(0., maxVolume);
        mDateAxis->setCategories(dates);

        chart->addSeries(balanceSeries);
        chart->addSeries(volumeSeries);

        balanceSeries->attachAxis(mDateAxis);
        balanceSeries->attachAxis(mBalanceAxis);

        volumeSeries->attachAxis(mDateAxis);
        volumeSeries->attachAxis(mVolumeAxis);

        mTotalIncomeLabel->setText(TextUtils::currencyToString(mTotalIncome, curLocale));
        mTotalOutcomeLabel->setText(TextUtils::currencyToString(mTotalOutcome, curLocale));
        mTotalBalanceLabel->setText(TextUtils::currencyToString(mTotalIncome - mTotalOutcome, curLocale));
        mTotalMarginLabel->setText(QString{"%1%2"}.arg(curLocale.toString(getMargin(), 'f', 2)).arg(curLocale.percent()));
        mTotalVolumeLabel->setText(curLocale.toString(totalVolume));

        setMarginColor();
    }

    void ItemHistoryWidget::setNumberFormat()
    {
        QSettings settings;
        const auto format = settings.value(UISettings::plotNumberFormatKey, UISettings::plotNumberFormatDefault).toString();

        mBalanceAxis->setLabelFormat(format);
        mVolumeAxis->setLabelFormat(format);
    }

    void ItemHistoryWidget::setMarginColor()
    {
        mTotalMarginLabel->setStyleSheet(TextUtils::getMarginStyleSheet(getMargin()));
    }

    double ItemHistoryWidget::getMargin() const noexcept
    {
        return (qFuzzyIsNull(mTotalIncome)) ? (0.) : (100. * (mTotalIncome - mTotalOutcome) / mTotalIncome);
    }
}
