#pragma once

#include "APIXmlReceiver.h"
#include "Character.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<Character::IdType>::attribute(const QXmlName &name, const QStringRef &value);
}
