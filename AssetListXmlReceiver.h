#pragma once

#include <stack>

#include "APIXmlReceiver.h"
#include "AssetList.h"

namespace Evernus
{
    template<>
    struct APIXmlReceiverAdditionalData<AssetList::ItemType>
    {
        std::stack<std::unique_ptr<AssetList::ItemType::element_type>> mElementStack;
        bool mParsingRowset = false;
    };

    template<>
    void APIXmlReceiver<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>::startElement(const QXmlName &name);
    template<>
    void APIXmlReceiver<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>::endElement();
    template<>
    void APIXmlReceiver<AssetList::ItemType, std::unique_ptr<AssetList::ItemType::element_type>>::attribute(const QXmlName &name, const QStringRef &value);
}
