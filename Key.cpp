#include "Key.h"

namespace Evernus
{
    Key::IdType Key::getId() const noexcept
    {
        return mId;
    }

    void Key::setId(IdType id) noexcept
    {
        mId = id;
    }

    QString Key::getCode() const &
    {
        return mCode;
    }

    QString &&Key::getCode() &&
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
