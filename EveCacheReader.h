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

#include <memory>

#include <QString>

namespace Evernus
{
    class EveCacheReader
    {
    public:
        EveCacheReader() = default;
        virtual ~EveCacheReader() = default;

        virtual bool seek(qint64 pos) = 0;
        virtual void advance(qint64 offset) = 0;
        virtual void setSize(qint64 size) = 0;

        virtual bool atEnd() const = 0;
        virtual qint64 getPos() const = 0;
        virtual qint64 getSize() const = 0;

        virtual uchar readChar() = 0;
        virtual qint32 readInt() = 0;
        virtual double readDouble() = 0;
        virtual float readFloat() = 0;
        virtual qint64 readLongLong() = 0;
        virtual qint16 readShort() = 0;
        virtual QString readString(uint len) = 0;

        virtual std::unique_ptr<EveCacheReader> extract(qint64 size) = 0;
    };
}
