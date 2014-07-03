#include "ConquerableStation.h"

namespace Evernus
{
    ConquerableStation::ConquerableStation(IdType id, const QString &name)
        : Entity{id}
        , mName{name}
    {
    }

    ConquerableStation::ConquerableStation(IdType id, QString &&name)
        : Entity{id}
        , mName{std::move(name)}
    {
    }

    QString ConquerableStation::getName() const &
    {
        return mName;
    }

    QString &&ConquerableStation::getName() && noexcept
    {
        return std::move(mName);
    }

    void ConquerableStation::setName(const QString &name)
    {
        mName = name;
    }

    void ConquerableStation::setName(QString &&name)
    {
        mName = std::move(name);
    }
}
