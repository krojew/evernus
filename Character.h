#pragma once

#include <boost/optional.hpp>

#include <QString>

#include "Key.h"

namespace Evernus
{
    class Character
        : public Entity<quint64>
    {
    public:
        typedef boost::optional<Key::IdType> KeyIdType;

        using Entity::Entity;
        virtual ~Character() = default;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

        KeyIdType getKeyId() const noexcept;
        void setKeyId(Key::IdType id);

        bool isEnabled() const noexcept;
        void setEnabled(bool flag) noexcept;

    private:
        QString mName;
        KeyIdType mKeyId;
        bool mEnabled = true;
    };
}
