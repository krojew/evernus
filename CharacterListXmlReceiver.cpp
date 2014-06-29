#include <stdexcept>

#include <QCoreApplication>

#include "CharacterListXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<Character::IdType>::attribute(const QXmlName &name, const QStringRef &value)
    {
        if (name.localName(mNamePool) == "characterID")
            *mCurrentElement = convert<Character::IdType>(value.toString());
    }
}
