/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
