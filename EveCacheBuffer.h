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

#include <QBuffer>

#include "EveCacheReader.h"

namespace Evernus
{
    class EveCacheBuffer
        : public EveCacheReader
    {
    public:
        EveCacheBuffer() = default;
        virtual ~EveCacheBuffer() = default;

        virtual bool seek(qint64 pos) override;
        virtual void advance(qint64 offset) override;
        virtual void setSize(qint64 size) override;

        virtual bool atEnd() const override;
        virtual qint64 getPos() const override;
        virtual qint64 getSize() const override;

        virtual uchar readChar() override;
        virtual qint32 readInt() override;
        virtual double readDouble() override;
        virtual float readFloat() override;
        virtual qint64 readLongLong() override;
        virtual qint16 readShort() override;
        virtual QString readString(uint len) override;

        virtual std::unique_ptr<EveCacheReader> extract(qint64 size) override;

        void openWithData(const QByteArray &data);

    private:
        QBuffer mBuffer;
    };
}
