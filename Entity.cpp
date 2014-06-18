namespace Evernus
{
    template<class Id>
    Entity<Id>::Entity(const IdType &id)
        : mId{id}
        , mOriginalId{id}
    {
    }

    template<class Id>
    Entity<Id>::Entity(IdType &&id)
        : mId{std::move(id)}
        , mOriginalId{std::move(id)}
    {
    }

    template<class Id>
    typename Entity<Id>::IdType Entity<Id>::getId() const
    {
        return mId;
    }

    template<class Id>
    void Entity<Id>::setId(IdType id)
    {
        mId = id;
    }

    template<class Id>
    typename Entity<Id>::IdType Entity<Id>::getOriginalId() const
    {
        return mOriginalId;
    }

    template<class Id>
    void Entity<Id>::updateOriginalId()
    {
        mOriginalId = mId;
    }

    template<class Id>
    bool Entity<Id>::isNew() const noexcept
    {
        return mIsNew;
    }

    template<class Id>
    void Entity<Id>::setNew(bool isNew) noexcept
    {
        mIsNew = isNew;
    }
}
