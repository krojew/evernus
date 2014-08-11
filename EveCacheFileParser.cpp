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

#include "EveCacheNodes.h"
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
//                throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Stream start not found!")};
                continue;

            mStreams.emplace_back(std::make_unique<EveCacheNode::Base>(EveCacheNode::Base::StreamCode::StreamStart));
            initShare();
        }
    }

    void EveCacheFileParser::initShare()
    {
        uint shares = mFile.readInt();
        if (shares >= 16384)
            return;

        if (shares != 0)
        {
            mShareMap.resize(shares);
            mShareObjs.resize(shares);

            const auto shareSkip = 4 * shares;
            const auto pos = mFile.getPos();
            const auto size = mFile.getSize();

            mFile.seek(size - shareSkip);

            for (auto i = 0u; i < shares; ++i)
            {
                mShareMap[i] = mFile.readInt();
                if (mShareMap[i] > shares || mShareMap[i] < 1)
                    throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Invalid data in share map!")};
            }

            mFile.seek(pos);
            mFile.setSize(size - shareSkip);
        }
    }
}
