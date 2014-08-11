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

namespace Evernus
{
    class EveCacheFile
        : private QBuffer
    {
    public:
        explicit EveCacheFile(const QString &fileName);
        explicit EveCacheFile(QString &&fileName);
        virtual ~EveCacheFile() = default;

        void open();
        bool seek(qint64 pos);
        void setSize(qint64 size);

        bool atEnd() const;
        qint64 getPos() const;
        qint64 getSize() const;

        unsigned char readChar();
        int readInt();

    private:
        QString mFileName;
    };
}
