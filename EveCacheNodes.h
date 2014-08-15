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

#include <vector>
#include <memory>

#include <QString>
#include <QHash>

namespace Evernus
{
    namespace EveCacheNode
    {
        class Base;

        typedef std::unique_ptr<Base> NodePtr;

        enum class StreamCode
        {
            Invalid = 0,
            StreamStart = 0x7e,
            None = 0x01, // Python None type
            String = 0x2, // Another type of string, also ids
            LongLong = 0x3, // 64 bit value?
            Integer = 0x04, // 4 byte, little endian?
            Short = 0x05, // 2 byte
            Byte = 0x6, // i think
            Neg1Integer = 0x07, // int == -1
            Integer0 = 0x08, // int == 0
            Integer1 = 0x09, // int == 1
            Real = 0x0a, // floating point, 64 bits, assume matches machine double type
            Real0 = 0x0b,
            String0 = 0xe, // 0 length string
            String3 = 0xf, // really? another? single character string
            String4 = 0x10,
            Marker = 0x11, // A one byte identifier code
            UnicodeString = 0x12,
            Ident = 0x13, // an identifier string
            Tuple = 0x14, // a tuple of N objects
            Tuple2 = 0x15, // a tuple of N objects, variant 2
            Dict = 0x16, // N objects, consisting of key object and value object
            Object = 0x17, // Object type ?
            Blue = 0x18, // blue object
            Callback = 0x19, // callback
            SharedObj = 0x1b, // shared object reference
            Checksum = 0x1c,
            BoolTrue = 0x1f,
            BoolFalse = 0x20,
            Object22 = 0x22, // a database header field of some variety
            Object23 = 0x23, // another datatype containing ECompressedRows/DBRows
            Tuple0 = 0x24, // a tuple of 0 objects
            Tuple1 = 0x25, // a tuple of 1 objects
            Tuple20 = 0x26,
            Tuple21 = 0x27, // a tuple of 1 objects, variant 2
            EmptyString = 0x28, // empty
            UnicodeString2 = 0x29,
            CompressedRow = 0x2a, // the datatype from hell, a RLEish compressed row
            Substream = 0x2b, // substream - len bytes followed by 0x7e
            TuplePair = 0x2c, // a tuple of 2 objects
            Mark = 0x2d, // marker (for the NEWOBJ/REDUCE iterators that follow them)
            String2 = 0x2e, // stringtastic
            SizedInt = 0x2f, // when you can't decide ahead of time how long to make the integer...
        };

        class Base
        {
        public:
            Base() = default;
            Base(const Base &other);
            Base(Base &&) = default;
            virtual ~Base() = default;

            void addChild(NodePtr &&child);

            std::vector<NodePtr> &getChildren() noexcept;
            const std::vector<NodePtr> &getChildren() const noexcept;

            virtual NodePtr clone() const;

            Base &operator =(const Base &other) = delete;
            Base &operator =(Base &&) = delete;

        private:
            std::vector<NodePtr> mChildren;
        };

        class None
            : public Base
        {
        public:
            None() = default;
            None(const None &) = default;
            None(None &&) = default;
            virtual ~None() = default;

            virtual NodePtr clone() const override;
        };

        class Real
            : public Base
        {
        public:
            Real(double value);
            Real(const Real &) = default;
            Real(Real &&) = default;
            virtual ~Real() = default;

            virtual NodePtr clone() const override;

            double getValue() const noexcept;

        private:
            double mValue = 0.;
        };

        class Int
            : public Base
        {
        public:
            Int(qint32 value);
            Int(const Int &) = default;
            Int(Int &&) = default;
            virtual ~Int() = default;

            virtual NodePtr clone() const override;

            qint32 getValue() const noexcept;

        private:
            qint32 mValue = 0;
        };

        class Bool
            : public Base
        {
        public:
            Bool(bool value);
            Bool(const Bool &) = default;
            Bool(Bool &&) = default;
            virtual ~Bool() = default;

            virtual NodePtr clone() const override;

            bool getValue() const noexcept;

        private:
            bool mValue = false;
        };

        class LongLong
            : public Base
        {
        public:
            LongLong(qint64 value);
            LongLong(const LongLong &) = default;
            LongLong(LongLong &&) = default;
            virtual ~LongLong() = default;

            virtual NodePtr clone() const override;

            qint64 getValue() const noexcept;

        private:
            qint64 mValue = false;
        };

        class Ident
            : public Base
        {
        public:
            Ident(const QString &name);
            Ident(QString &&name);
            Ident(const Ident &) = default;
            Ident(Ident &&) = default;
            virtual ~Ident() = default;

            virtual NodePtr clone() const override;

            QString getName() const;

        private:
            QString mName;
        };

        class String
            : public Base
        {
        public:
            String() = default;
            String(const QString &value);
            String(QString &&value);
            String(const String &) = default;
            String(String &&) = default;
            virtual ~String() = default;

            virtual NodePtr clone() const override;

            QString getValue() const;

        private:
            QString mValue;
        };

        class Dictionary
            : public Base
        {
        public:
            Dictionary() = default;
            Dictionary(const Dictionary &) = default;
            Dictionary(Dictionary &&) = default;
            virtual ~Dictionary() = default;

            virtual NodePtr clone() const override;

            Base *getByName(const QString &name) const;

        private:
            mutable QHash<QString, Base *> mNodeMap;
        };

        class Tuple
            : public Base
        {
        public:
            Tuple() = default;
            Tuple(const Tuple &) = default;
            Tuple(Tuple &&) = default;
            virtual ~Tuple() = default;

            virtual NodePtr clone() const override;
        };

        class Substream
            : public Base
        {
        public:
            Substream() = default;
            Substream(const Substream &) = default;
            Substream(Substream &&) = default;
            virtual ~Substream() = default;

            virtual NodePtr clone() const override;
        };

        class Object
            : public Base
        {
        public:
            Object() = default;
            Object(const Object &) = default;
            Object(Object &&) = default;
            virtual ~Object() = default;

            virtual NodePtr clone() const override;

            QString getName() const;
        };

        class Marker
            : public Base
        {
        public:
            explicit Marker(uchar id);
            Marker(const Marker &) = default;
            Marker(Marker &&) = default;
            virtual ~Marker() = default;

            virtual NodePtr clone() const override;

            uchar getId() const noexcept;

        private:
            uchar mId = 0;
        };

        class DBRow
            : public Base
        {
        public:
            DBRow(int id, const std::vector<uchar> &data);
            DBRow(int id, std::vector<uchar> &&data);
            DBRow(const DBRow &) = default;
            DBRow(DBRow &&) = default;
            virtual ~DBRow() = default;

            virtual NodePtr clone() const override;

        private:
            int mId = 0;
            bool mLast = false;
            std::vector<uchar> mData;
        };
    }
}
