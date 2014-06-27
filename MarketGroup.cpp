#include "MarketGroup.h"

namespace Evernus
{
    MarketGroup::ParentIdType MarketGroup::getParentId() const
    {
        return mParentId;
    }

    void MarketGroup::setParentId(const ParentIdType &id)
    {
        mParentId = id;
    }

    QString MarketGroup::getName() const &
    {
        return mName;
    }

    QString &&MarketGroup::getName() && noexcept
    {
        return std::move(mName);
    }

    void MarketGroup::setName(const QString &name)
    {
        mName = name;
    }

    void MarketGroup::setName(QString &&name)
    {
        mName = std::move(name);
    }

    MarketGroup::DescriptionType MarketGroup::getDescription() const &
    {
        return mDescription;
    }

    MarketGroup::DescriptionType &&MarketGroup::getDescription() && noexcept
    {
        return std::move(mDescription);
    }

    void MarketGroup::setDescription(const DescriptionType &name)
    {
        mDescription = name;
    }

    void MarketGroup::setDescription(DescriptionType &&name)
    {
        mDescription = std::move(name);
    }

    MarketGroup::IconIdType MarketGroup::getIconId() const
    {
        return mIconId;
    }

    void MarketGroup::setIconId(const IconIdType &id)
    {
        mIconId = id;
    }

    bool MarketGroup::hasTypes() const noexcept
    {
        return mHasTypes;
    }

    void MarketGroup::setHasTypes(bool has)
    {
        mHasTypes = has;
    }
}
