#pragma once

#include <QString>

#include "Entity.h"

namespace Evernus
{
    class Key
        : public Entity<uint>
    {
    public:
        using Entity::Entity;

        Key() = default;

        template<class T>
        Key(IdType id, T &&code)
            : Entity{id}
            , mCode{std::forward<T>(code)}
        {
        }

        ~Key() = default;

        QString getCode() const &;
        QString &&getCode() &&;
        void setCode(const QString &code);
        void setCode(QString &&code);

    private:
        QString mCode;
    };
}
