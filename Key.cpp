#include "Key.h"

namespace Evernus
{
    QString Key::getCode() const &
    {
        return mCode;
    }

    QString &&Key::getCode() && noexcept
    {
        return std::move(mCode);
    }

    void Key::setCode(const QString &code)
    {
        mCode = code;
    }

    void Key::setCode(QString &&code)
    {
        mCode = std::move(code);
    }
}
