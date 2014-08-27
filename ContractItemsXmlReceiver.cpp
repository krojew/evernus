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
#include "ContractItemsXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<ContractItem>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        auto strValue = value.toString();

        if (localName == "recordID")
            mCurrentElement->setId(convert<ContractItem::IdType>(strValue));
        else if (localName == "typeID")
            mCurrentElement->setTypeId(convert<EveType::IdType>(strValue));
        else if (localName == "quantity")
            mCurrentElement->setQuantity(convert<quint64>(strValue));
        else if (localName == "included")
            mCurrentElement->setIncluded(strValue != "0");
    }
}
