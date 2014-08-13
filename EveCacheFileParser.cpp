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
            if (static_cast<EveCacheNode::StreamCode>(check) != EveCacheNode::StreamCode::StreamStart)
//                throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Stream start not found!")};
                continue;

            mStreams.emplace_back(std::make_unique<EveCacheNode::Base>());

            initShare();
            parse(*mStreams.back(), 1);
            skipShare();
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

    void EveCacheFileParser::skipShare()
    {
        mFile.advance(mShareMap.size() * 4);
    }

    void EveCacheFileParser::parse(EveCacheNode::Base &stream, uint limit)
    {
        while (!mFile.atEnd() && limit != 0)
        {
            auto node = parseNext();
            if (node)
                stream.addChild(std::move(node));

            --limit;
        }
    }

    EveCacheNode::NodePtr EveCacheFileParser::parseNext()
    {
        auto type = mFile.readChar();
        if (mFile.atEnd())
            return EveCacheNode::NodePtr{};

        const auto isShared = type & 0x40;
        EveCacheNode::NodePtr result;

        const auto realType = static_cast<EveCacheNode::StreamCode>(type & 0x3f);
        switch (realType) {
        case EveCacheNode::StreamCode::None:
            result = std::make_unique<EveCacheNode::None>();
            break;
        case EveCacheNode::StreamCode::Real:
            result = std::make_unique<EveCacheNode::Real>(mFile.readDouble());
            break;
        case EveCacheNode::StreamCode::Real0:
            result = std::make_unique<EveCacheNode::Real>(0.);
            break;
        case EveCacheNode::StreamCode::BoolFalse:
            result = std::make_unique<EveCacheNode::Bool>(false);
            break;
        case EveCacheNode::StreamCode::BoolTrue:
            result = std::make_unique<EveCacheNode::Bool>(true);
            break;
        case EveCacheNode::StreamCode::Integer:
            result = std::make_unique<EveCacheNode::Int>(mFile.readInt());
            break;
        case EveCacheNode::StreamCode::Integer0:
            result = std::make_unique<EveCacheNode::Int>(0);
            break;
        case EveCacheNode::StreamCode::Integer1:
            result = std::make_unique<EveCacheNode::Int>(1);
            break;
        case EveCacheNode::StreamCode::Neg1Integer:
            result = std::make_unique<EveCacheNode::Int>(-1);
            break;
        case EveCacheNode::StreamCode::LongLong:
            result = std::make_unique<EveCacheNode::LongLong>(mFile.readLongLong());
            break;
        case EveCacheNode::StreamCode::Short:
            result = std::make_unique<EveCacheNode::Int>(mFile.readShort());
            break;
        case EveCacheNode::StreamCode::Byte:
            result = std::make_unique<EveCacheNode::Int>(mFile.readChar());
            break;
        case EveCacheNode::StreamCode::SizedInt:
            {
                const auto size = mFile.readChar();
                switch (size) {
                case 8:
                    result = std::make_unique<EveCacheNode::LongLong>(mFile.readLongLong());
                    break;
                case 4:
                    result = std::make_unique<EveCacheNode::Int>(mFile.readInt());
                    break;
                case 2:
                    result = std::make_unique<EveCacheNode::Int>(mFile.readShort());
                    break;
                case 3:
                    result = std::make_unique<EveCacheNode::Int>(
                        mFile.readChar() | (mFile.readChar() << 16) | (mFile.readChar() << 24));
                }
            }
            break;
        case EveCacheNode::StreamCode::Ident:
            result = std::make_unique<EveCacheNode::Ident>(mFile.readString(parseLen()));
            break;
        case EveCacheNode::StreamCode::EmptyString:
            result = std::make_unique<EveCacheNode::String>();
            break;
        case EveCacheNode::StreamCode::UnicodeString2:
            result = std::make_unique<EveCacheNode::String>(mFile.readString(2));
            break;
        case EveCacheNode::StreamCode::String3:
            result = std::make_unique<EveCacheNode::String>(mFile.readString(1));
            break;
        case EveCacheNode::StreamCode::String0:
            result = std::make_unique<EveCacheNode::String>();
            break;
        case EveCacheNode::StreamCode::UnicodeString:
        case EveCacheNode::StreamCode::String4:
        case EveCacheNode::StreamCode::String2:
        case EveCacheNode::StreamCode::String:
            result = std::make_unique<EveCacheNode::String>(mFile.readString(mFile.readChar()));
            break;
        }

        return result;
    }

    uint EveCacheFileParser::parseLen()
    {
        uint len = mFile.readChar();
        if ((len & 0xff) == 0xff)
            len = mFile.readInt();

        return len;
    }
}
