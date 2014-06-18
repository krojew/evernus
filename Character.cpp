#include "Character.h"

namespace Evernus
{
    QString Character::getName() const &
    {
        return mName;
    }

    QString &&Character::getName() && noexcept
    {
        return std::move(mName);
    }

    void Character::setName(const QString &name)
    {
        mName = name;
    }

    void Character::setName(QString &&name)
    {
        mName = std::move(name);
    }

    Character::KeyIdType Character::getKeyId() const noexcept
    {
        return mKeyId;
    }

    void Character::setKeyId(Key::IdType id)
    {
        mKeyId = id;
    }

    bool Character::isEnabled() const noexcept
    {
        return mEnabled;
    }

    void Character::setEnabled(bool flag) noexcept
    {
        mEnabled = flag;
    }
}
