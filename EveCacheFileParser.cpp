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
#include <stdexcept>

#include "EveCacheFile.h"

#include "EveCacheFileParser.h"

namespace Evernus
{
    EveCacheFileParser::EveCacheFileParser(EveCacheFile &file)
        : mFile{file}
    {
    }

    void EveCacheFileParser::parse()
    {
        while (!mFile.atEnd())
        {
            const auto check = mFile.readChar();
            if (static_cast<EveCacheNode::Base::StreamCode>(check) != EveCacheNode::Base::StreamCode::StreamStart)
//                throw std::runtime_error{tr("Stream start not found!").toStdString()};
                continue;
        }
    }
}
