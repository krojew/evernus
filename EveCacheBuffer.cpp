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
#include "EveCacheBuffer.h"

namespace Evernus
{
    bool EveCacheBuffer::seek(qint64 pos)
    {
        return mBuffer.seek(pos);
    }

    void EveCacheBuffer::advance(qint64 offset)
    {
        mBuffer.seek(mBuffer.pos() + offset);
    }

    void EveCacheBuffer::setSize(qint64 size)
    {
        mBuffer.buffer().resize(size);
    }

    bool EveCacheBuffer::atEnd() const
    {
        return mBuffer.atEnd();
    }

    qint64 EveCacheBuffer::getPos() const
    {
        return mBuffer.pos();
    }

    qint64 EveCacheBuffer::getSize() const
    {
        return mBuffer.size();
    }

    uchar EveCacheBuffer::readChar()
    {
        uchar out;

        if (mBuffer.read(reinterpret_cast<char *>(&out), sizeof(out)) < sizeof(out))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheBuffer", "Error reading cache.")};

        return out;
    }

    qint32 EveCacheBuffer::readInt()
    {
        return readChar() |
               (readChar() << 8) |
               (readChar() << 16) |
               (readChar() << 24);
    }

    double EveCacheBuffer::readDouble()
    {
        double out;

        if (mBuffer.read(reinterpret_cast<char *>(&out), sizeof(out)) < sizeof(out))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheBuffer", "Error reading cache.")};

        return out;
    }

    float EveCacheBuffer::readFloat()
    {
        float out;

        if (mBuffer.read(reinterpret_cast<char *>(&out), sizeof(out)) < sizeof(out))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheBuffer", "Error reading cache.")};

        return out;
    }

    qint64 EveCacheBuffer::readLongLong()
    {
        const qint64 a = readInt();
        const qint64 b = readInt();

        return a | (b << 32);
    }

    qint16 EveCacheBuffer::readShort()
    {
        return readChar() | (readChar() << 8);
    }

    std::string EveCacheBuffer::readString(uint len)
    {
        std::string out(len, 0);

        if (mBuffer.read(&out.front(), len) < len)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheBuffer", "Error reading cache.")};

        return out;
    }

    std::unique_ptr<EveCacheReader> EveCacheBuffer::extract(qint64 size)
    {
        const auto data = mBuffer.read(size);
        if (data.size() < size)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheBuffer", "Error reading cache.")};

        auto result = std::make_unique<EveCacheBuffer>();
        result->openWithData(data);

        return std::move(result);
    }

    void EveCacheBuffer::openWithData(const QByteArray &data)
    {
        mBuffer.setData(data);

        if (!mBuffer.open(QIODevice::ReadOnly))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheBuffer", "Cannot open buffer.")};
    }
}
