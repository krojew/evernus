#pragma once

#include <QWidget>

class QDoubleSpinBox;

namespace Evernus
{
    class PricePreferencesWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit PricePreferencesWidget(QWidget *parent = nullptr);
        virtual ~PricePreferencesWidget() = default;

    public slots:
        void applySettings();

    private:
        QDoubleSpinBox *mMinMarginEdit = nullptr;
        QDoubleSpinBox *mPreferredMarginEdit = nullptr;
        QDoubleSpinBox *mPriceDeltaEdit = nullptr;
    };
}
