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

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace Evernus
{
    namespace EveCacheNode
    {
        class Base;

        typedef std::unique_ptr<Base> NodePtr;

        enum class StreamCode
        {
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
            String2 = 0x2e, // stringtastic
            SizedInt = 0x2f, // when you can't decide ahead of time how long to make the integer...
        };

        class Base
        {
        public:
            Base() = default;
            Base(const Base &) = delete;
            Base(Base &&) = delete;
            virtual ~Base() = default;

            void addChild(NodePtr &&child);

            Base &operator =(const Base &) = delete;
            Base &operator =(Base &&) = delete;

        private:
            std::vector<NodePtr> mChildren;
        };

        class None
            : public Base
        {
        public:
            None() = default;
            virtual ~None() = default;
        };

        class Real
            : public Base
        {
        public:
            Real(double value);
            virtual ~Real() = default;

            double getValue() const noexcept;

        private:
            double mValue = 0.;
        };

        class Int
            : public Base
        {
        public:
            Int(int32_t value);
            virtual ~Int() = default;

            int32_t getValue() const noexcept;

        private:
            int32_t mValue = 0;
        };

        class Bool
            : public Base
        {
        public:
            Bool(bool value);
            virtual ~Bool() = default;

            bool getValue() const noexcept;

        private:
            bool mValue = false;
        };

        class LongLong
            : public Base
        {
        public:
            LongLong(int64_t value);
            virtual ~LongLong() = default;

            int64_t getValue() const noexcept;

        private:
            int64_t mValue = false;
        };

        class Ident
            : public Base
        {
        public:
            Ident(const std::string &name);
            Ident(std::string &&name);
            virtual ~Ident() = default;

            std::string getName() const;

        private:
            std::string mName;
        };

        class String
            : public Base
        {
        public:
            String() = default;
            String(const std::string &value);
            String(std::string &&value);
            virtual ~String() = default;

            std::string getValue() const;

        private:
            std::string mValue;
        };
    }
}
