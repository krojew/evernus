#pragma once

namespace Evernus
{
    template<class Id>
    class Entity
    {
    public:
        typedef Id IdType;

        Entity() = default;
        Entity(const IdType &id);
        Entity(IdType &&id);
        virtual ~Entity() = default;

        IdType getId() const;
        void setId(IdType id);

        IdType getOriginalId() const;
        void updateOriginalId();

        bool isNew() const noexcept;
        void setNew(bool isNew) noexcept;

    private:
        IdType mId = IdType{}, mOriginalId = IdType{};

        bool mIsNew = true;
    };
}

#include "Entity.cpp"
