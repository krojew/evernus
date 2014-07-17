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
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>
#include <QEvent>

#include "UISettings.h"

#include "MarketOrderInfoWidget.h"

namespace Evernus
{
    MarketOrderInfoWidget::MarketOrderInfoWidget(QWidget *parent)
        : QFrame{parent, Qt::Tool | Qt::FramelessWindowHint}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);
        mainLayout->setContentsMargins(3, 3, 3, 3);

        auto btnLayout = new QHBoxLayout{};
        mainLayout->addLayout(btnLayout);

        auto closeBtn = new QPushButton{QIcon{":/images/cross.png"}, tr("Close"), this};
        btnLayout->addWidget(closeBtn);
        closeBtn->setFlat(true);
        connect(closeBtn, &QPushButton::clicked, this, &MarketOrderInfoWidget::deleteLater);

        auto copyBtn = new QPushButton{QIcon{":/images/page_copy.png"}, tr("Copy"), this};
        btnLayout->addWidget(copyBtn);
        copyBtn->setFlat(true);
        connect(copyBtn, &QPushButton::clicked, this, &MarketOrderInfoWidget::copyPrice);

        QSettings settings;

        auto copyCheck = new QCheckBox{tr("Copy new price on open"), this};
        btnLayout->addWidget(copyCheck);
        copyCheck->setChecked(settings.value(UISettings::autoCopyPriceFromInfoKey, true).toBool());
        connect(copyCheck, &QCheckBox::stateChanged, this, &MarketOrderInfoWidget::setAutoCopy);

        btnLayout->addStretch();

        setFrameStyle(QFrame::StyledPanel);
    }

    void MarketOrderInfoWidget::setAutoCopy(int state)
    {
        QSettings settings;
        settings.setValue(UISettings::autoCopyPriceFromInfoKey, state == Qt::Checked);
    }

    void MarketOrderInfoWidget::copyPrice()
    {

    }

    bool MarketOrderInfoWidget::event(QEvent *event)
    {
        if (event->type() == QEvent::WindowDeactivate)
        {
            event->accept();
            deleteLater();
            return true;
        }

        return QFrame::event(event);
    }
}
