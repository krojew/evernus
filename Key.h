#pragma once

#include <QString>

namespace Evernus
{
    class Key
    {
    public:
        typedef uint IdType;

        Key() = default;

        template<class T>
        Key(IdType id, T &&code)
            : mId{id}
            , mCode{std::forward<T>(code)}
        {
        }

        ~Key() = default;

        IdType getId() const noexcept;
        void setId(IdType id) noexcept;

        QString getCode() const &;
        QString &&getCode() &&;
        void setCode(const QString &code);
        void setCode(QString &&code);

    private:
        IdType mId = 0;
        QString mCode;
    };
}
