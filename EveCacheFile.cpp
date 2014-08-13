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

#include <QFile>

#include "EveCacheFile.h"

namespace Evernus
{
    EveCacheFile::EveCacheFile(const QString &fileName)
        : mFileName{fileName}
    {
    }

    EveCacheFile::EveCacheFile(QString &&fileName)
        : mFileName{std::move(fileName)}
    {
    }

    void EveCacheFile::open()
    {
        QFile file{mFileName};
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFile", "Cannot open file.")};

        mBuffer.setData(file.readAll());

        if (!mBuffer.open(QIODevice::ReadOnly))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFile", "Cannot open file.")};
    }

    bool EveCacheFile::seek(qint64 pos)
    {
        return mBuffer.seek(pos);
    }

    void EveCacheFile::advance(qint64 offset)
    {
        mBuffer.seek(mBuffer.pos() + offset);
    }

    void EveCacheFile::setSize(qint64 size)
    {
        mBuffer.buffer().resize(size);
    }

    bool EveCacheFile::atEnd() const
    {
        return mBuffer.atEnd();
    }

    qint64 EveCacheFile::getPos() const
    {
        return mBuffer.pos();
    }

    qint64 EveCacheFile::getSize() const
    {
        return mBuffer.size();
    }

    unsigned char EveCacheFile::readChar()
    {
        unsigned char out;

        if (mBuffer.read(reinterpret_cast<char *>(&out), sizeof(out)) < sizeof(out))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFile", "Error reading file.")};

        return out;
    }

    int32_t EveCacheFile::readInt()
    {
        return readChar() |
               (readChar() << 8) |
               (readChar() << 16) |
               (readChar() << 24);
    }

    double EveCacheFile::readDouble()
    {
        double out;

        if (mBuffer.read(reinterpret_cast<char *>(&out), sizeof(out)) < sizeof(out))
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFile", "Error reading file.")};

        return out;
    }

    int64_t EveCacheFile::readLongLong()
    {
        const int64_t a = readInt();
        const int64_t b = readInt();

        return a | (b << 32);
    }

    int16_t EveCacheFile::readShort()
    {
        return readChar() | (readChar() << 8);
    }

    std::string EveCacheFile::readString(uint len)
    {
        std::string out(len, 0);

        if (mBuffer.read(&out.front(), len) < len)
            throw std::runtime_error{QT_TRANSLATE_NOOP("EveCacheFile", "Error reading file.")};

        return out;
    }
}
