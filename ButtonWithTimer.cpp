#include "ButtonWithTimer.h"

namespace Evernus
{
    ButtonWithTimer::ButtonWithTimer(const QString &text, QWidget *parent)
        : QPushButton{QIcon{":/images/arrow_refresh.png"}, text, parent}
        , mOrigText{text}
    {
        setFlat(true);

        connect(&mTimer, &QTimer::timeout, this, &ButtonWithTimer::tick);
    }

    void ButtonWithTimer::setTimer(const QDateTime &endTime)
    {
        mEndTime = endTime;
        mTimer.start(1000);
        tick();
    }

    void ButtonWithTimer::stopTimer()
    {
        mTimer.stop();
        setText(mOrigText);
    }

    void ButtonWithTimer::tick()
    {
        const auto curTime = QDateTime::currentDateTime();
        if (curTime >= mEndTime)
        {
            mTimer.stop();
        }
        else
        {
            const auto delta = curTime.secsTo(mEndTime);
            setText(QString{"%1 (%2:%3)"}.arg(mOrigText).arg(delta / 60, 2, 10, QLatin1Char{'0'}).arg(delta % 60, 2, 10, QLatin1Char{'0'}));
        }
    }
}
