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

#include <QOAuth2AuthorizationCodeFlow>

namespace Evernus
{
    // this whole class exists because of https://bugreports.qt.io/browse/QTBUG-66097
    // TODO: remove when fixed

    class ESIOAuth2AuthorizationCodeFlow
        : public QOAuth2AuthorizationCodeFlow
    {
    public:
        using QOAuth2AuthorizationCodeFlow::QOAuth2AuthorizationCodeFlow;
        ESIOAuth2AuthorizationCodeFlow(const ESIOAuth2AuthorizationCodeFlow &) = default;
        ESIOAuth2AuthorizationCodeFlow(ESIOAuth2AuthorizationCodeFlow &&) = default;
        virtual ~ESIOAuth2AuthorizationCodeFlow() = default;

        void resetStatus();

        ESIOAuth2AuthorizationCodeFlow &operator =(const ESIOAuth2AuthorizationCodeFlow &) = default;
        ESIOAuth2AuthorizationCodeFlow &operator =(ESIOAuth2AuthorizationCodeFlow &&) = default;
    };
}
