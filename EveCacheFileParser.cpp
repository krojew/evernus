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

#include "EveCacheBuffer.h"
#include "EveCacheNodes.h"

#include "EveCacheFileParser.h"

namespace Evernus
{
    namespace
    {
        struct packer_opcap
        {
            uchar tlen : 3;
            bool tzero : 1;
            uchar blen : 3;
            bool bzero : 1;
        };

        void rleUnpack(const uchar *inBuf, int inLength, std::vector<uchar> &buffer)
        {
            buffer.clear();
            if (inLength == 0)
                return;

            const auto end = inBuf + inLength;
            while (inBuf < end)
            {
                const auto opcap = *(reinterpret_cast<const packer_opcap *>(inBuf++));
                if (opcap.tzero)
                {
                    for (auto count = opcap.tlen + 1; count > 0; count--)
                        buffer.push_back(0);
                }
                else
                {
                    for (auto count = 8 - opcap.tlen; count > 0 && inBuf < end; --count)
                        buffer.push_back(*inBuf++);
                }

                if (opcap.bzero)
                {
                    for (auto count = opcap.blen + 1; count > 0; count--)
                        buffer.push_back(0);
                }
                else
                {
                    for (auto count = 8 - opcap.blen; count > 0 && inBuf < end; --count)
                        buffer.push_back(*inBuf++);
                }
            }
        }
    }

    EveCacheFileParser::EveCacheFileParser(EveCacheReader &file)
        : mReader{file}
    {
    }

    void EveCacheFileParser::parse()
    {
        while (!mReader.atEnd())
        {
            const auto check = mReader.readChar();
            if (static_cast<EveCacheNode::StreamCode>(check) != EveCacheNode::StreamCode::StreamStart)
//                throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Stream start not found!")};
                continue;

            mStreams.emplace_back(std::make_unique<EveCacheNode::Base>());

            initShare();
            parse(*mStreams.back(), 1);
            skipShare();
        }
    }

    std::vector<EveCacheNode::NodePtr> &EveCacheFileParser::getStreams() &
    {
        return mStreams;
    }

    const std::vector<EveCacheNode::NodePtr> &EveCacheFileParser::getStreams() const &
    {
        return mStreams;
    }

    std::vector<EveCacheNode::NodePtr> &&EveCacheFileParser::getStreams() && noexcept
    {
        return std::move(mStreams);
    }

    void EveCacheFileParser::initShare()
    {
        uint shares = mReader.readInt();
        if (shares >= 16384)
            return;

        if (shares != 0)
        {
            mShareMap.resize(shares + 1);
            mShareObjs.resize(shares + 1);

            const auto shareSkip = 4 * shares;
            const auto pos = mReader.getPos();
            const auto size = mReader.getSize();

            mReader.seek(size - shareSkip);

            for (auto i = 0u; i < shares; ++i)
            {
                mShareMap[i] = mReader.readInt();
                if (mShareMap[i] > shares || mShareMap[i] < 1)
                    throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Invalid data in share map!")};
            }

            mReader.seek(pos);
            mReader.setSize(size - shareSkip);
        }
    }

    void EveCacheFileParser::skipShare()
    {
        mReader.advance(mShareMap.size() * 4);
    }

    void EveCacheFileParser::parse(EveCacheNode::Base &stream, uint limit)
    {
        while (!mReader.atEnd() && limit != 0)
        {
            auto node = parseNext();
            if (node)
                stream.addChild(std::move(node));

            --limit;
        }
    }

    EveCacheNode::NodePtr EveCacheFileParser::parseNext()
    {
        auto type = mReader.readChar();
        if (mReader.atEnd())
            return EveCacheNode::NodePtr{};

        const auto isShared = type & 0x40;
        EveCacheNode::NodePtr result;

        const auto realType = static_cast<EveCacheNode::StreamCode>(type & 0x3f);
        switch (realType) {
        case EveCacheNode::StreamCode::None:
            result = std::make_unique<EveCacheNode::None>();
            break;
        case EveCacheNode::StreamCode::Real:
            result = std::make_unique<EveCacheNode::Real>(mReader.readDouble());
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
            result = std::make_unique<EveCacheNode::Int>(mReader.readInt());
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
            result = std::make_unique<EveCacheNode::LongLong>(mReader.readLongLong());
            break;
        case EveCacheNode::StreamCode::Short:
            result = std::make_unique<EveCacheNode::Int>(mReader.readShort());
            break;
        case EveCacheNode::StreamCode::Byte:
            result = std::make_unique<EveCacheNode::Int>(mReader.readChar());
            break;
        case EveCacheNode::StreamCode::SizedInt:
            {
                const auto size = mReader.readChar();
                switch (size) {
                case 8:
                    result = std::make_unique<EveCacheNode::LongLong>(mReader.readLongLong());
                    break;
                case 4:
                    result = std::make_unique<EveCacheNode::Int>(mReader.readInt());
                    break;
                case 2:
                    result = std::make_unique<EveCacheNode::Int>(mReader.readShort());
                    break;
                case 3:
                    result = std::make_unique<EveCacheNode::Int>(
                        mReader.readChar() | (mReader.readChar() << 16) | (mReader.readChar() << 24));
                }
            }
            break;
        case EveCacheNode::StreamCode::Ident:
            result = std::make_unique<EveCacheNode::Ident>(mReader.readString(parseLen()));
            break;
        case EveCacheNode::StreamCode::EmptyString:
            result = std::make_unique<EveCacheNode::String>();
            break;
        case EveCacheNode::StreamCode::UnicodeString2:
            result = std::make_unique<EveCacheNode::String>(mReader.readString(2));
            break;
        case EveCacheNode::StreamCode::String3:
            result = std::make_unique<EveCacheNode::String>(mReader.readString(1));
            break;
        case EveCacheNode::StreamCode::String0:
            result = std::make_unique<EveCacheNode::String>();
            break;
        case EveCacheNode::StreamCode::UnicodeString:
        case EveCacheNode::StreamCode::String4:
        case EveCacheNode::StreamCode::String2:
        case EveCacheNode::StreamCode::String:
            result = std::make_unique<EveCacheNode::String>(mReader.readString(mReader.readChar()));
            break;
        case EveCacheNode::StreamCode::Dict:
            {
                const auto len = parseLen();
                result = std::make_unique<EveCacheNode::Dictionary>();

                parse(*result, len * 2);
            }
            break;
        case EveCacheNode::StreamCode::Tuple:
        case EveCacheNode::StreamCode::Tuple2:
            {
                const auto len = parseLen();
                result = std::make_unique<EveCacheNode::Tuple>();

                parse(*result, len);
            }
            break;
        case EveCacheNode::StreamCode::TuplePair:
            result = std::make_unique<EveCacheNode::Tuple>();
            parse(*result, 2);
            break;
        case EveCacheNode::StreamCode::Tuple21:
        case EveCacheNode::StreamCode::Tuple1:
            result = std::make_unique<EveCacheNode::Tuple>();
            parse(*result, 1);
            break;
        case EveCacheNode::StreamCode::Tuple20:
        case EveCacheNode::StreamCode::Tuple0:
            result = std::make_unique<EveCacheNode::Tuple>();
            break;
        case EveCacheNode::StreamCode::Marker:
            result = std::make_unique<EveCacheNode::Marker>(parseLen());
            break;
        case EveCacheNode::StreamCode::Object:
            result = std::make_unique<EveCacheNode::Object>();
            parse(*result, 2);
            break;
        case EveCacheNode::StreamCode::Object22:
        case EveCacheNode::StreamCode::Object23:
            {
                result = std::make_unique<EveCacheNode::Object>();
                parse(*result, 1);

                const auto name = static_cast<const EveCacheNode::Object *>(result.get())->getName();
                if (name.compare("dbutil.RowList") == 0 || name.compare("eve.common.script.sys.rowset.RowList") == 0)
                {
                    while (auto row = parseNext())
                        result->addChild(std::move(row));
                }
            }
            break;
        case EveCacheNode::StreamCode::Substream:
            {
                result = std::make_unique<EveCacheNode::Substream>();
                auto reader = mReader.extract(parseLen());

                EveCacheFileParser parser{*reader};
                parser.parse();

                auto streams = std::move(parser).getStreams();
                for (auto &stream : streams)
                    result->addChild(std::move(stream));
            }
            break;
        case EveCacheNode::StreamCode::CompressedRow:
            result = parseDBRow();
            break;
        case EveCacheNode::StreamCode::SharedObj:
            result = getShare(parseLen());
            break;
        case EveCacheNode::StreamCode::Checksum:
            result = std::make_unique<EveCacheNode::String>("checksum");
            mReader.readInt();
            break;
        case EveCacheNode::StreamCode::Mark:
            if (mReader.readChar() != 0x2d)
                throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Didn't encounter a double 0x2d where should be one!")};

            result = std::make_unique<EveCacheNode::Base>();
            break;
        case EveCacheNode::StreamCode::Invalid:
            break;
        default:
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Unknown stream type!")};
        }

        if (!result)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Couldn't parse stream!")};

        if (isShared)
            addShare(result.get());

        return result;
    }

    EveCacheNode::NodePtr EveCacheFileParser::parseDBRow()
    {
        auto baseHead = parseNext();
        auto head = dynamic_cast<EveCacheNode::Object *>(baseHead.get());

        if (head == nullptr)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "DB row not found!")};

        if (head->getName().compare("blue.DBRowDescriptor") != 0)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Bad descriptor name!")};

        if (head->getChildren().empty() ||
            head->getChildren().front()->getChildren().size() < 2 ||
            head->getChildren().front()->getChildren()[1]->getChildren().empty())
        {
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Invalid row size!")};
        }

        auto fields = dynamic_cast<EveCacheNode::Tuple *>(head->getChildren().front()->getChildren()[1]->getChildren().front().get());

        const auto len = parseLen();
        const auto compData = mReader.readString(len);

        std::vector<uchar> newData;
        rleUnpack(reinterpret_cast<const uchar *>(compData.c_str()), len, newData);

        EveCacheBuffer blob;
        blob.openWithData(QByteArray{reinterpret_cast<const char *>(newData.data()), static_cast<int>(newData.size())});

        auto body = std::make_unique<EveCacheNode::DBRow>(17, std::move(newData));
        auto dict = std::make_unique<EveCacheNode::Dictionary>();
        auto step = 1;

        while (step < 6)
        {
            const auto &children = fields->getChildren();
            for (const auto &child : children)
            {
                if (child->getChildren().size() < 2)
                    throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Invalid row fields!")};

                auto fn = child->getChildren().front()->clone();
                const EveCacheNode::Int *ft = dynamic_cast<const EveCacheNode::Int *>(child->getChildren()[1].get());

                if (ft == nullptr)
                    throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Invalid row field type!")};

                const auto fti = ft->getValue();

                auto boolCount = 0;
                auto boolBuf = 0;

                EveCacheNode::NodePtr obj;

                switch (fti) {
                case 21:
                case 20:
                case 6:
                case 64:
                    if (step == 1)
                        obj = std::make_unique<EveCacheNode::LongLong>(blob.readLongLong());
                    break;
                case 5:
                    if (step == 1)
                        obj = std::make_unique<EveCacheNode::Real>(blob.readDouble());
                    break;
                case 4:
                    obj = std::make_unique<EveCacheNode::Real>(blob.readFloat());
                    break;
                case 19:
                case 3:
                    if (step == 2)
                        obj = std::make_unique<EveCacheNode::Int>(blob.readInt());
                    break;
                case 18:
                case 2:
                    if (step == 3)
                        obj = std::make_unique<EveCacheNode::Int>(blob.readShort());
                    break;
                case 17:
                case 16:
                    obj = std::make_unique<EveCacheNode::Int>(blob.readChar());
                    break;
                case 11:
                    if (step == 5)
                    {
                        if (boolCount == 0)
                        {
                            boolBuf = blob.readChar();
                            boolCount = 1;
                        }

                        obj = std::make_unique<EveCacheNode::Bool>(boolCount & boolBuf);

                        boolCount <<= 1;
                    }
                    break;
                case 128:
                case 129:
                case 130:
                    obj = std::make_unique<EveCacheNode::String>("DB strings are not supported yet.");
                    break;
                default:
                    throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Unknown ADO type!")};
                }

                if (obj)
                {
                    dict->addChild(std::move(obj));
                    dict->addChild(std::move(fn));
                }
            }

            ++step;
        }

        EveCacheNode::NodePtr fakeRow = std::make_unique<EveCacheNode::Tuple>();
        fakeRow->addChild(std::move(baseHead));
        fakeRow->addChild(std::move(body));
        fakeRow->addChild(std::move(dict));

        return fakeRow;
    }

    EveCacheNode::NodePtr EveCacheFileParser::getShare(size_t index) const
    {
        if (index >= mShareObjs.size())
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Share index out of range!")};

        if (mShareObjs[index] == nullptr)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "No share at specified index found!")};

        return mShareObjs[index]->clone();
    }

    void EveCacheFileParser::addShare(EveCacheNode::Base *node)
    {
        if (mShareCursor >= mShareMap.size())
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Share cursor out of range!")};

        const auto id = mShareMap[mShareCursor];
        if (id > mShareObjs.size())
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFileParser", "Share ID out of range!")};

        mShareObjs[id] = node;
        ++mShareCursor;
    }

    uint EveCacheFileParser::parseLen()
    {
        uint len = mReader.readChar();
        if ((len & 0xff) == 0xff)
            len = mReader.readInt();

        return len;
    }
}
