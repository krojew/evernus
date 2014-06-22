#pragma once

class QDomElement;

namespace Evernus
{
    namespace APIDomParser
    {
        template<class T>
        T parse(const QDomElement &node);
    }
}
