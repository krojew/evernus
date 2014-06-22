#pragma once

#include "APIDomParser.h"
#include "Character.h"

namespace Evernus
{
    namespace APIDomParser
    {
        template<>
        Character parse<Character>(const QDomElement &node);
    }
}

#include "CharacterDomParser.cpp"
