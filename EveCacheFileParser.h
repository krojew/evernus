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

#include "EveCacheNodes.h"

namespace Evernus
{
    class EveCacheReader;

    class EveCacheFileParser
    {
    public:
        explicit EveCacheFileParser(EveCacheReader &file);
        ~EveCacheFileParser() = default;

        void parse();

        std::vector<EveCacheNode::NodePtr> &getStreams() &;
        const std::vector<EveCacheNode::NodePtr> &getStreams() const &;
        std::vector<EveCacheNode::NodePtr> &&getStreams() && noexcept;

    private:
        EveCacheReader &mReader;

        std::vector<EveCacheNode::NodePtr> mStreams;
        std::vector<uint> mShareMap;
        std::vector<EveCacheNode::Base *> mShareObjs;

        size_t mShareCursor = 0;

        void initShare();
        void skipShare();

        void parse(EveCacheNode::Base &stream, uint limit);
        EveCacheNode::NodePtr parseNext();
        EveCacheNode::NodePtr parseDBRow();

        EveCacheNode::NodePtr getShare(size_t index) const;
        void addShare(EveCacheNode::Base *node);

        uint parseLen();
    };
}
