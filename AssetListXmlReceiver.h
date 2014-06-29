#pragma once

#include <stack>

#include "APIXmlReceiver.h"
#include "AssetList.h"

namespace Evernus
{
    template<>
    struct APIXmlReceiverAdditionalData<AssetList::value_type>
    {
        std::stack<std::unique_ptr<AssetList::value_type::element_type>> mElementStack;
        bool mParsingRowset = false;
    };

    template<>
    void APIXmlReceiver<AssetList::value_type, std::unique_ptr<AssetList::value_type::element_type>>::startElement(const QXmlName &name);
    template<>
    void APIXmlReceiver<AssetList::value_type, std::unique_ptr<AssetList::value_type::element_type>>::endElement();
    template<>
    void APIXmlReceiver<AssetList::value_type, std::unique_ptr<AssetList::value_type::element_type>>::attribute(const QXmlName &name, const QStringRef &value);
}
