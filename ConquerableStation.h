#pragma once

#include <QString>

#include "Entity.h"

namespace Evernus
{
    class ConquerableStation
        : public Entity<uint>
    {
    public:
        using Entity::Entity;

        ConquerableStation() = default;
        ConquerableStation(IdType id, const QString &name);
        ConquerableStation(IdType id, QString &&name);
        virtual ~ConquerableStation() = default;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

    private:
        QString mName;
    };
}
