#include <functional>
#include <memory>
#include <thread>
#include <cmath>

#include <QDialogButtonBox>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QApplication>
#include <QRadioButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QClipboard>
#include <QDateTime>
#include <QSettings>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QDebug>
#include <QFont>
#include <QFile>
#include <QDir>

#ifdef Q_OS_WIN
// see setNewWindowFlags()
#   include <windows.h>
#endif

#include "MarginToolSettings.h"
#include "PriceSettings.h"
#include "PathSettings.h"
#include "NameProvider.h"
#include "Repository.h"

#include "MarginToolDialog.h"

namespace Evernus
{
    MarginToolDialog
    ::MarginToolDialog(const Repository<Character> &characterRepository, const NameProvider &nameProvider, QWidget *parent)
        : QDialog{parent}
        , mCharacterRepository{characterRepository}
        , mNameProvider{nameProvider}
    {
        QSettings settings;

        QFont bigFont;
        bigFont.setBold(true);
        bigFont.setPointSize(bigFont.pointSize() + 3);

        QFont boldFont;
        boldFont.setBold(true);

        const auto alwaysOnTop = settings.value(MarginToolSettings::alwaysOnTopKey, true).toBool();

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mNameLabel = new QLabel{tr("export market logs in game"), this};
        mainLayout->addWidget(mNameLabel);
        mNameLabel->setFont(bigFont);

        auto priceGroup = new QGroupBox{this};
        mainLayout->addWidget(priceGroup);

        auto priceLayout = new QGridLayout{};
        priceGroup->setLayout(priceLayout);

        priceLayout->addWidget(new QLabel{tr("Buy:")}, 0, 0);
        priceLayout->addWidget(new QLabel{tr("Sell:")}, 1, 0);
        priceLayout->addWidget(new QLabel{tr("Profit:")}, 2, 0);
        priceLayout->addWidget(new QLabel{tr("Revenue:")}, 0, 2);
        priceLayout->addWidget(new QLabel{tr("Cost of sales:")}, 1, 2);

        mBestBuyLabel = new QLabel{"-", this};
        priceLayout->addWidget(mBestBuyLabel, 0, 1);

        mBestSellLabel = new QLabel{"-", this};
        priceLayout->addWidget(mBestSellLabel, 1, 1);

        mProfitLabel = new QLabel{"-", this};
        priceLayout->addWidget(mProfitLabel, 2, 1);
        mProfitLabel->setFont(boldFont);

        mRevenueLabel = new QLabel{"-", this};
        priceLayout->addWidget(mRevenueLabel, 0, 3);

        mCostOfSalesLabel = new QLabel{"-", this};
        priceLayout->addWidget(mCostOfSalesLabel, 1, 3);

        auto orderGroup = new QGroupBox{this};
        mainLayout->addWidget(orderGroup);

        auto orderLayout = new QGridLayout{};
        orderGroup->setLayout(orderLayout);

        orderLayout->addWidget(new QLabel{tr("Buy orders:")}, 0, 0);

        mBuyOrdersLabel = new QLabel{"-", this};
        orderLayout->addWidget(mBuyOrdersLabel, 0, 1);

        orderLayout->addWidget(new QLabel{tr("Sell orders:")}, 1, 0);

        mSellOrdersLabel = new QLabel{"-", this};
        orderLayout->addWidget(mSellOrdersLabel, 1, 1);

        orderLayout->addWidget(new QLabel{tr("Buy volume/movement:")}, 0, 2);

        mBuyVolLabel = new QLabel{"-", this};
        orderLayout->addWidget(mBuyVolLabel, 0, 3);

        orderLayout->addWidget(new QLabel{tr("Sell volume/movement:")}, 1, 2);

        mSellVolLabel = new QLabel{"-", this};
        orderLayout->addWidget(mSellVolLabel, 1, 3);

        auto infoLayout = new QHBoxLayout{};
        mainLayout->addLayout(infoLayout);

        auto marginGroup = new QGroupBox{this};
        infoLayout->addWidget(marginGroup);

        auto marginLayout = new QGridLayout{};
        marginGroup->setLayout(marginLayout);

        marginLayout->addWidget(new QLabel{tr("Margin:")}, 0, 0);

        mMarginLabel = new QLabel{"-", this};
        marginLayout->addWidget(mMarginLabel, 0, 1);
        mMarginLabel->setFont(bigFont);

        marginLayout->addWidget(new QLabel{tr("Markup:")}, 1, 0);

        mMarkupLabel = new QLabel{"-", this};
        marginLayout->addWidget(mMarkupLabel, 1, 1);

        auto copyGroup = new QGroupBox{tr("Autocopy"), this};
        infoLayout->addWidget(copyGroup);

        auto copyLayout = new QVBoxLayout{};
        copyGroup->setLayout(copyLayout);

        const auto copyMode = static_cast<const PriceSettings::CopyMode>(
            settings.value(PriceSettings::copyModeKey, static_cast<int>(PriceSettings::CopyMode::DontCopy)).toInt());

        mDontCopyBtn = new QRadioButton{tr("Nothing"), this};
        copyLayout->addWidget(mDontCopyBtn);
        if (copyMode == PriceSettings::CopyMode::DontCopy)
            mDontCopyBtn->setChecked(true);

        mCopySellBtn = new QRadioButton{tr("Sell price"), this};
        copyLayout->addWidget(mCopySellBtn);
        if (copyMode == PriceSettings::CopyMode::CopySell)
            mCopySellBtn->setChecked(true);

        mCopyBuyBtn = new QRadioButton{tr("Buy price"), this};
        copyLayout->addWidget(mCopyBuyBtn);
        if (copyMode == PriceSettings::CopyMode::CopyBuy)
            mCopyBuyBtn->setChecked(true);

        connect(mDontCopyBtn, &QRadioButton::toggled, this, &MarginToolDialog::saveCopyMode);
        connect(mCopySellBtn, &QRadioButton::toggled, this, &MarginToolDialog::saveCopyMode);
        connect(mCopyBuyBtn, &QRadioButton::toggled, this, &MarginToolDialog::saveCopyMode);

        auto taxesGroup = new QGroupBox{this};
        mainLayout->addWidget(taxesGroup);

        auto taxesLayout = new QHBoxLayout{};
        taxesGroup->setLayout(taxesLayout);

        taxesLayout->addWidget(new QLabel{tr("Broker fee:"), this});

        mBrokerFeeLabel = new QLabel{"-", this};
        taxesLayout->addWidget(mBrokerFeeLabel);

        taxesLayout->addWidget(new QLabel{tr("Sales tax:"), this});

        mSalesTaxLabel = new QLabel{"-", this};
        taxesLayout->addWidget(mSalesTaxLabel);

        auto sampleGroup = new QGroupBox{tr("Sample data"), this};
        mainLayout->addWidget(sampleGroup);

        auto sampleLayout = new QHBoxLayout{};
        sampleGroup->setLayout(sampleLayout);

        m1SampleDataTable = createSampleTable();
        sampleLayout->addWidget(m1SampleDataTable);

        m5SampleDataTable = createSampleTable();
        sampleLayout->addWidget(m5SampleDataTable);

        auto alwaysOnTopBtn = new QCheckBox{tr("Always on top"), this};
        mainLayout->addWidget(alwaysOnTopBtn);
        alwaysOnTopBtn->setChecked(alwaysOnTop);
        connect(alwaysOnTopBtn, &QCheckBox::stateChanged, this, &MarginToolDialog::toggleAlwaysOnTop);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Close, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto logPath = settings.value(PathSettings::marketLogsPath).toString();
        if (logPath.isEmpty())
            logPath = QStandardPaths::locate(QStandardPaths::DocumentsLocation, "EVE/logs/Marketlogs", QStandardPaths::LocateDirectory);

        if (logPath.isEmpty())
        {
            QMessageBox::warning(this,
                                 tr("Margin tool error"),
                                 tr("Could not determine market log path. Please enter log path in settings."));
        }
        else if (!mWatcher.addPath(logPath))
        {
            QMessageBox::warning(this,
                                 tr("Margin tool error"),
                                 tr("Could not start watching market log path. Make sure the path exists (eg. export some logs) and try again."));
        }
        else
        {
            mKnownFiles = getKnownFiles(logPath);
        }

        connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this, &MarginToolDialog::refreshData);

        setWindowTitle(tr("Margin tool"));
        setAttribute(Qt::WA_DeleteOnClose);
        setNewWindowFlags(alwaysOnTop);
    }

    void MarginToolDialog::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        try
        {
            const auto taxes = calculateTaxes();

            mBrokerFeeLabel->setText(QString{"%1%"}.arg(taxes.mBrokerFee * 100., 0, 'f', 2));
            mSalesTaxLabel->setText(QString{"%1%"}.arg(taxes.mSalesTax * 100., 0, 'f', 2));
        }
        catch (const Repository<Character>::NotFoundException &)
        {
            mBrokerFeeLabel->setText("-");
            mSalesTaxLabel->setText("-");
            mRevenueLabel->setText("-");
            mCostOfSalesLabel->setText("-");
            mMarginLabel->setText("-");
            mMarkupLabel->setText("-");
        }
    }

    void MarginToolDialog::toggleAlwaysOnTop(int state)
    {
        const auto alwaysOnTop = state == Qt::Checked;

        QSettings settings;
        settings.setValue(MarginToolSettings::alwaysOnTopKey, alwaysOnTop);

        setNewWindowFlags(alwaysOnTop);
    }

    void MarginToolDialog::refreshData(const QString &path)
    {
        QString targetFile;
        QLocale locale;

        auto newFiles = getKnownFiles(path);

        QHashIterator<QString, QDateTime> it{newFiles};
        while (it.hasNext())
        {
            it.next();
            if (!mKnownFiles.contains(it.key()))
            {
                targetFile = it.key();
                break;
            }
        }

        if (targetFile.isEmpty())
        {
            QHashIterator<QString, QDateTime> it{newFiles};
            while (it.hasNext())
            {
                it.next();
                if (it.value() != mKnownFiles[it.key()])
                {
                    targetFile = it.key();
                    break;
                }
            }

            if (targetFile.isEmpty())
            {
                mKnownFiles = std::move(newFiles);
                return;
            }
        }

        mKnownFiles = std::move(newFiles);

        if (!targetFile.isEmpty())
        {
            const QString logFile = path % QDir::separator() % targetFile;

            qDebug() << "Calculating margin from file: " << logFile;

            QFile file{logFile};

#ifdef Q_OS_WIN
            while (!file.open(QIODevice::ReadWrite))
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
#else
            const auto modTimeDelay = 1000;

            // wait for Eve to finish dumping data
            QFileInfo info{file};
            forever
            {
                const auto modTime = info.lastModified();
                if (modTime.msecsTo(QDateTime::currentDateTime()) >= modTimeDelay)
                    break;
            }
#endif

            auto buy = -1.;
            auto sell = -1.;

            uint buyVol = 0, buyInit = 0;
            uint sellVol = 0, sellInit = 0;

            auto buyCount = 0u;
            auto sellCount = 0u;

            QString name;

            while (!file.atEnd())
            {
                const QString line = file.readLine();
                const auto values = line.split(',');

                if (values.count() >= 14)
                {
                    if (name.isNull())
                    {
                        auto ok = false;
                        const auto id = values[2].toULong(&ok);

                        if (ok)
                            name = mNameProvider.getName(id);
                    }

                    if (values[13] != "0")
                        continue;

                    const auto curValue = values[0].toDouble();

                    if (values[7] == "True")
                    {
                        if (curValue > buy)
                            buy = curValue;

                        buyVol += static_cast<uint>(values[1].toDouble());
                        buyInit += static_cast<uint>(values[5].toDouble());

                        ++buyCount;
                    }
                    else if (values[7] == "False")
                    {
                        if (curValue < sell || sell < 0.)
                            sell = curValue;

                        sellVol += static_cast<uint>(values[1].toDouble());
                        sellInit += static_cast<uint>(values[5].toDouble());

                        ++sellCount;
                    }
                }
            }

            mNameLabel->setText(name);
            mBuyOrdersLabel->setText(QString::number(buyCount));
            mSellOrdersLabel->setText(QString::number(sellCount));
            mBuyVolLabel->setText(QString{"%1/%2"}.arg(locale.toString(buyVol)).arg(locale.toString(buyInit - buyVol)));
            mSellVolLabel->setText(QString{"%1/%2"}.arg(locale.toString(sellVol)).arg(locale.toString(sellInit - sellVol)));

            try
            {
                const auto taxes = calculateTaxes();

                if (buy < 0. || sell < 0.)
                {
                    if (buy < 0.)
                    {
                        mBestBuyLabel->setText("-");
                        mCostOfSalesLabel->setText("-");
                    }
                    else
                    {
                        const auto buyPrice = buy + 0.01;

                        mBestBuyLabel->setText(locale.toCurrencyString(buyPrice, "ISK"));
                        mCostOfSalesLabel->setText(locale.toCurrencyString(getCoS(buyPrice, taxes), "ISK"));
                    }

                    if (sell < 0.)
                    {
                        mBestSellLabel->setText("-");
                        mRevenueLabel->setText("-");
                    }
                    else
                    {
                        const auto sellPrice = sell + 0.01;

                        mBestSellLabel->setText(locale.toCurrencyString(sellPrice, "ISK"));
                        mRevenueLabel->setText(locale.toCurrencyString(getRevenue(sellPrice, taxes), "ISK"));
                    }

                    mProfitLabel->setText("-");
                    mMarginLabel->setStyleSheet("color: palette(text);");
                    m1SampleDataTable->clearContents();
                    m5SampleDataTable->clearContents();
                }
                else
                {
                        const auto bestBuy = buy;
                        const auto bestSell = sell;
                        const auto sellPrice = bestSell - 0.01;
                        const auto buyPrice = bestBuy + 0.01;
                        const auto revenue = getRevenue(sellPrice, taxes);
                        const auto cos = getCoS(buyPrice, taxes);
                        const auto margin = 100. * (revenue - cos) / revenue;
                        const auto markup = 100. * (revenue - cos) / cos;

                        mMarginLabel->setText(QString{"%1%"}.arg(margin, 0, 'f', 2));
                        mMarkupLabel->setText(QString{"%1%"}.arg(markup, 0, 'f', 2));

                        mBestBuyLabel->setText(locale.toCurrencyString(buyPrice, "ISK"));
                        mBestSellLabel->setText(locale.toCurrencyString(sellPrice, "ISK"));

                        mProfitLabel->setText(locale.toCurrencyString(revenue - cos, "ISK"));
                        mRevenueLabel->setText(locale.toCurrencyString(revenue, "ISK"));
                        mCostOfSalesLabel->setText(locale.toCurrencyString(cos, "ISK"));

                        QSettings settings;

                        if (margin < settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble())
                            mMarginLabel->setStyleSheet("color: red;");
                        else if (margin < settings.value(PriceSettings::preferredMarginKey, PriceSettings::preferredMarginDefault).toDouble())
                            mMarginLabel->setStyleSheet("color: orange;");
                        else
                            mMarginLabel->setStyleSheet("color: green;");

                        auto clipboard = QApplication::clipboard();

                        if (mCopySellBtn->isChecked())
                            clipboard->setText(QString::number(sellPrice, 'f', 2));
                        else if (mCopyBuyBtn->isChecked())
                            clipboard->setText(QString::number(buyPrice, 'f', 2));

                        fillSampleData(*m1SampleDataTable, revenue, cos, 1);
                        fillSampleData(*m5SampleDataTable, revenue, cos, 5);
                }
            }
            catch (const Repository<Character>::NotFoundException &)
            {
            }
        }
    }

    void MarginToolDialog::saveCopyMode()
    {
        QSettings settings;

        if (mDontCopyBtn->isChecked())
            settings.setValue(PriceSettings::copyModeKey, static_cast<int>(PriceSettings::CopyMode::DontCopy));
        else if (mCopySellBtn->isChecked())
            settings.setValue(PriceSettings::copyModeKey, static_cast<int>(PriceSettings::CopyMode::CopySell));
        else if (mCopyBuyBtn->isChecked())
            settings.setValue(PriceSettings::copyModeKey, static_cast<int>(PriceSettings::CopyMode::CopyBuy));
    }

    void MarginToolDialog::setNewWindowFlags(bool alwaysOnTop)
    {
#ifdef Q_OS_WIN
        // https://bugreports.qt-project.org/browse/QTBUG-30359
        if (alwaysOnTop)
            SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        else
            SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
        auto flags = windowFlags();
        if (alwaysOnTop)
            flags |= Qt::WindowStaysOnTopHint;
        else
            flags &= ~Qt::WindowStaysOnTopHint;

        setWindowFlags(flags);
        show();
#endif
    }

    MarginToolDialog::Taxes MarginToolDialog::calculateTaxes() const
    {
        const auto character = mCharacterRepository.find(mCharacterId);
        const auto feeSkills = character.getFeeSkills();
        const auto brokerFee = (0.01 - 0.0005 * feeSkills.mBrokerRelations) /
                               std::exp(0.1 * character.getFactionStanding() + 0.04 * character.getCorpStanding());
        const auto salesTax = 0.015 * (1. - feeSkills.mAccounting * 0.1);

        return Taxes{brokerFee, salesTax};
    }

    QTableWidget *MarginToolDialog::createSampleTable()
    {
        auto table = new QTableWidget{this};
        table->setColumnCount(3);
        table->setRowCount(static_cast<int>(std::log10(samples)));
        table->setHorizontalHeaderLabels(QStringList{} << "Volume" << "Cost" << "Profit");
        table->verticalHeader()->hide();

        return table;
    }

    void MarginToolDialog::fillSampleData(QTableWidget &table, double revenue, double cos, int multiplier)
    {
        const auto inserter = [&table](auto &&text, auto row, auto column) {
            std::unique_ptr<QTableWidgetItem> item{new QTableWidgetItem{std::forward<decltype(text)>(text)}};
            table.setItem(row, column, item.get());
            item.release();
        };

        const auto realMultiplier = multiplier / 1000000.;
        const auto profit = revenue - cos;

        QLocale locale;

        for (auto i = 1, row = 0; i < samples; i *= 10, ++row)
        {
            inserter(locale.toString(i * multiplier), row, 0);
            inserter(locale.toString(cos * i * realMultiplier, 'f', 2) + "M", row, 1);
            inserter(locale.toString(profit * i * realMultiplier, 'f', 2) + "M", row, 2);
        }

        table.resizeColumnsToContents();
    }

    MarginToolDialog::FileModificationMap MarginToolDialog::getKnownFiles(const QString &path)
    {
        const QDir basePath{path};
        const auto files = basePath.entryList(QStringList{"*.txt"}, QDir::Files | QDir::Readable);

        FileModificationMap out;
        for (const auto &file : files)
        {
            if (file.startsWith("My Orders"))
                continue;

            QFileInfo info{basePath.filePath(file)};
            out[file] = info.lastModified();
        }

        return out;
    }

    double MarginToolDialog::getCoS(double buyPrice, const Taxes &taxes)
    {
        return buyPrice + buyPrice * taxes.mBrokerFee;
    }

    double MarginToolDialog::getRevenue(double sellPrice, const Taxes &taxes)
    {
        return sellPrice - sellPrice * taxes.mSalesTax - sellPrice * taxes.mBrokerFee;
    }
}
