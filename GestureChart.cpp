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
#include "GestureChart.h"

namespace Evernus
{
    GestureChart::GestureChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
        : QChart{parent, wFlags}
    {
        grabGesture(Qt::PanGesture);
        grabGesture(Qt::PinchGesture);
    }

    bool GestureChart::sceneEvent(QEvent *event)
    {
        if (event->type() == QEvent::Gesture)
            return gestureEvent(static_cast<QGestureEvent *>(event));

        return QChart::event(event);
    }

    bool GestureChart::gestureEvent(QGestureEvent *event)
    {
        if (const auto gesture = event->gesture(Qt::PanGesture))
        {
            const auto pan = static_cast<QPanGesture *>(gesture);
            QChart::scroll(-(pan->delta().x()), pan->delta().y());
        }

        if (const auto gesture = event->gesture(Qt::PinchGesture))
        {
            const auto pinch = static_cast<QPinchGesture *>(gesture);
            if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
                QChart::zoom(pinch->scaleFactor());
        }

        return true;
    }
}
