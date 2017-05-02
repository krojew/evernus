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
#include <QFileDialog>
#include <QPushButton>
#include <QMessageBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QPainter>
#include <QLocale>
#include <QLabel>
#include <QImage>

#include "DateFilteredPlotWidget.h"

namespace Evernus
{
    DateFilteredPlotWidget::DateFilteredPlotWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto filterLayout = new QHBoxLayout{};
        mainLayout->addLayout(filterLayout);

        filterLayout->addWidget(new QLabel{tr("From:"), this});

        mFromEdit = new QDateEdit{this};
        filterLayout->addWidget(mFromEdit);
        mFromEdit->setCalendarPopup(true);
        connect(mFromEdit, &QDateEdit::dateChanged, this, &DateFilteredPlotWidget::fromChanged);

        filterLayout->addWidget(new QLabel{tr("To:"), this});

        mToEdit = new QDateEdit{this};
        filterLayout->addWidget(mToEdit);
        mToEdit->setCalendarPopup(true);
        connect(mToEdit, &QDateEdit::dateChanged, this, &DateFilteredPlotWidget::toChanged);

        auto legendBtn = new QCheckBox{tr("Show legend"), this};
        filterLayout->addWidget(legendBtn);
        legendBtn->setChecked(true);

        auto saveBtn = new QPushButton{QIcon{":/images/image.png"}, tr("Save..."), this};
        filterLayout->addWidget(saveBtn);
        saveBtn->setFlat(true);
        connect(saveBtn, &QPushButton::clicked, this, &DateFilteredPlotWidget::savePlot);

        filterLayout->addStretch();

        mChart = new ZoomableChartView{this};
        mainLayout->addWidget(mChart);
        mChart->setMinimumHeight(300);
        connect(legendBtn, &QCheckBox::stateChanged, this, [this](bool checked) {
            mChart->chart()->legend()->setVisible(checked);
        });

        auto locale = mChart->locale();
        locale.setNumberOptions(0);
        mChart->setLocale(locale);
    }

    QDate DateFilteredPlotWidget::getFrom() const
    {
        return mFromEdit->date();
    }

    void DateFilteredPlotWidget::setFrom(const QDate &date)
    {
        mFromEdit->setDate(date);
    }

    QDate DateFilteredPlotWidget::getTo() const
    {
        return mToEdit->date();
    }

    void DateFilteredPlotWidget::setTo(const QDate &date)
    {
        mToEdit->setDate(date);
    }

    QChart &DateFilteredPlotWidget::getChart() const
    {
        return *mChart->chart();
    }

    void DateFilteredPlotWidget::savePlot()
    {
        const auto file = QFileDialog::getSaveFileName(this, tr("Save plot"), QString{}, tr("Images (*.png *.jpg *.jpeg *.bmp *.ppm *.xbm *.xpm)"));
        if (file.isEmpty())
            return;

        QImage image{mChart->sceneRect().size().toSize(), QImage::Format_RGB32};

        QPainter painter{&image};
        painter.setRenderHint(QPainter::Antialiasing);

        mChart->render(&painter);

        if (!image.save(file))
            QMessageBox::warning(this, tr("Error"), tr("Error saving image."));
    }

    void DateFilteredPlotWidget::fromChanged(const QDate &date)
    {
        if (date > mToEdit->date())
            mToEdit->setDate(date.addDays(1));
        else
            emit filterChanged();
    }

    void DateFilteredPlotWidget::toChanged(const QDate &date)
    {
        if (date < mFromEdit->date())
            mFromEdit->setDate(date.addDays(-1));
        else
            emit filterChanged();
    }
}
