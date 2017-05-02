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
#pragma once

#include <QGestureEvent>
#include <QChart>

QT_CHARTS_USE_NAMESPACE

namespace Evernus
{
    class GestureChart
        : public QChart
    {
    public:
        GestureChart(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = Qt::WindowFlags{});
        GestureChart(const GestureChart &) = default;
        GestureChart(GestureChart &&) = default;
        virtual ~GestureChart() = default;

        GestureChart &operator =(const GestureChart &) = default;
        GestureChart &operator =(GestureChart &&) = default;

    protected:
        virtual bool sceneEvent(QEvent *event) override;

    private:
        bool gestureEvent(QGestureEvent *event);
    };
}
