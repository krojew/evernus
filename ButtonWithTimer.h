#pragma once

#include <QPushButton>
#include <QDateTime>
#include <QTimer>

namespace Evernus
{
    class ButtonWithTimer
        : public QPushButton
    {
        Q_OBJECT

    public:
        explicit ButtonWithTimer(const QString &text, QWidget *parent = nullptr);
        virtual ~ButtonWithTimer() = default;

        void setTimer(const QDateTime &endTime);
        void stopTimer();

    private slots:
        void tick();

    private:
        QTimer mTimer;
        QString mOrigText;
        QDateTime mEndTime;
    };
}
