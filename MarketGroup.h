#pragma once

#include <boost/optional.hpp>

#include <QString>

#include "Entity.h"

namespace Evernus
{
    class MarketGroup
        : public Entity<uint>
    {
    public:
        typedef boost::optional<IdType> ParentIdType;
        typedef boost::optional<QString> DescriptionType;
        typedef boost::optional<uint> IconIdType;

        using Entity::Entity;
        virtual ~MarketGroup() = default;

        ParentIdType getParentId() const;
        void setParentId(const ParentIdType &id);

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

        DescriptionType getDescription() const &;
        DescriptionType &&getDescription() && noexcept;
        void setDescription(const DescriptionType &name);
        void setDescription(DescriptionType &&name);

        IconIdType getIconId() const;
        void setIconId(const IconIdType &id);

        bool hasTypes() const noexcept;
        void setHasTypes(bool has);

    private:
        ParentIdType mParentId;
        QString mName;
        DescriptionType mDescription;
        IconIdType mIconId;
        bool mHasTypes;
    };
}
