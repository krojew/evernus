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

#include "ZoomableChartView.h"

namespace Evernus
{
    ZoomableChartView::ZoomableChartView(QWidget *parent)
        : QChartView{new GestureChart{}, parent}
    {
        setRubberBand(QChartView::RectangleRubberBand);
    }

    bool ZoomableChartView::viewportEvent(QEvent *event)
    {
        if (event->type() == QEvent::TouchBegin)
        {
            mIsTouching = true;
            chart()->setAnimationOptions(QChart::NoAnimation);
        }

        return QChartView::viewportEvent(event);
    }

    void ZoomableChartView::mousePressEvent(QMouseEvent *event)
    {
        if (mIsTouching)
            return;

        QChartView::mousePressEvent(event);
    }

    void ZoomableChartView::mouseMoveEvent(QMouseEvent *event)
    {
        if (mIsTouching)
            return;

        QChartView::mouseMoveEvent(event);
    }

    void ZoomableChartView::mouseReleaseEvent(QMouseEvent *event)
    {
        if (mIsTouching)
            mIsTouching = false;

        chart()->setAnimationOptions(QChart::SeriesAnimations);
        QChartView::mouseReleaseEvent(event);
    }

    void ZoomableChartView::keyPressEvent(QKeyEvent *event)
    {
        switch (event->key()) {
        case Qt::Key_Plus:
            chart()->zoomIn();
            break;
        case Qt::Key_Minus:
            chart()->zoomOut();
            break;
        case Qt::Key_Left:
            chart()->scroll(-10, 0);
            break;
        case Qt::Key_Right:
            chart()->scroll(10, 0);
            break;
        case Qt::Key_Up:
            chart()->scroll(0, 10);
            break;
        case Qt::Key_Down:
            chart()->scroll(0, -10);
            break;
        default:
            QGraphicsView::keyPressEvent(event);
        }
    }
}
