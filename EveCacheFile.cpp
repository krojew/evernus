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
        : QBuffer{}
        , mFileName{fileName}
    {
    }

    EveCacheFile::EveCacheFile(QString &&fileName)
        : QBuffer{}
        , mFileName{std::move(fileName)}
    {
    }

    void EveCacheFile::open()
    {
        QFile file{mFileName};
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error{tr("Cannot open file.").toStdString()};

        setData(file.readAll());

        if (!QBuffer::open(QIODevice::ReadOnly))
            throw std::runtime_error{tr("Cannot open file.").toStdString()};
    }

    bool EveCacheFile::seek(qint64 pos)
    {
        return QBuffer::seek(pos);
    }

    void EveCacheFile::setSize(qint64 size)
    {
        buffer().resize(size);
    }

    bool EveCacheFile::atEnd() const
    {
        return QBuffer::atEnd();
    }

    qint64 EveCacheFile::getPos() const
    {
        return QBuffer::pos();
    }

    qint64 EveCacheFile::getSize() const
    {
        return QBuffer::size();
    }

    unsigned char EveCacheFile::readChar()
    {
        unsigned char out;

        if (read(reinterpret_cast<char *>(&out), sizeof(out)) < sizeof(out))
            throw std::runtime_error{tr("Error reading file.").toStdString()};

        return out;
    }

    int EveCacheFile::readInt()
    {
        return readChar() |
               (readChar() << 8) |
               (readChar() << 16) |
               (readChar() << 24);
    }
}
