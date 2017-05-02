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

#include <QChartView>

QT_CHARTS_USE_NAMESPACE

namespace Evernus
{
    class ZoomableChartView
        : public QChartView
    {
    public:
        explicit ZoomableChartView(QWidget *parent = nullptr);
        ZoomableChartView(const ZoomableChartView &) = default;
        ZoomableChartView(ZoomableChartView &&) = default;
        virtual ~ZoomableChartView() = default;

        ZoomableChartView &operator =(const ZoomableChartView &) = default;
        ZoomableChartView &operator =(ZoomableChartView &&) = default;

    protected:
        virtual bool viewportEvent(QEvent *event) override;
        virtual void mousePressEvent(QMouseEvent *event) override;
        virtual void mouseMoveEvent(QMouseEvent *event) override;
        virtual void mouseReleaseEvent(QMouseEvent *event) override;
        virtual void keyPressEvent(QKeyEvent *event) override;

    private:
        bool mIsTouching = false;
    };
}
