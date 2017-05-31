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
#include <QDomElement>
#include <QSettings>

#include "CharacterDomParser.h"
#include "ImportSettings.h"

namespace Evernus
{
    namespace APIDomParser
    {
        template<>
        Character parse<Character>(const QDomElement &node)
        {
            Character result{node.firstChildElement("characterID").text().toULongLong()};
            result.setName(node.firstChildElement("name").text());
            result.setCorporationName(node.firstChildElement("corporationName").text());
            result.setCorporationId(node.firstChildElement("corporationID").text().toULongLong());
            result.setRace(node.firstChildElement("race").text());
            result.setBloodline(node.firstChildElement("bloodLine").text());
            result.setAncestry(node.firstChildElement("ancestry").text());
            result.setGender(node.firstChildElement("gender").text());
            result.setISK(node.firstChildElement("balance").text().toDouble());

            QSettings settings;
            if (settings.value(ImportSettings::importSkillsKey, ImportSettings::importSkillsDefault).toBool())
            {
                const auto rowsets = node.elementsByTagName("rowset");

                QDomElement skillsElement;
                for (auto i = 0; i < rowsets.count(); ++i)
                {
                    const auto element = rowsets.at(i).toElement();
                    if (element.attribute("name") == "skills")
                    {
                        skillsElement = element;
                        break;
                    }
                }

                CharacterData::OrderAmountSkills orderAmountSkills;
                CharacterData::TradeRangeSkills tradeRangeSkills;
                CharacterData::FeeSkills feeSkills;
                CharacterData::ContractSkills contractSkills;
                CharacterData::ReprocessingSkills reprocessingSkills;

                const auto skills = node.elementsByTagName("row");
                for (auto i = 0; i < skills.count(); ++i)
                {
                    const auto element = skills.at(i).toElement();
                    switch (element.attribute("typeID").toInt()) {
                    case 3443:
                        orderAmountSkills.mTrade = element.attribute("level").toInt();
                        break;
                    case 3444:
                        orderAmountSkills.mRetail = element.attribute("level").toInt();
                        break;
                    case 16596:
                        orderAmountSkills.mWholesale = element.attribute("level").toInt();
                        break;
                    case 18580:
                        orderAmountSkills.mTycoon = element.attribute("level").toInt();
                        break;
                    case 16598:
                        tradeRangeSkills.mMarketing = element.attribute("level").toInt();
                        break;
                    case 16594:
                        tradeRangeSkills.mProcurement = element.attribute("level").toInt();
                        break;
                    case 16595:
                        tradeRangeSkills.mDaytrading = element.attribute("level").toInt();
                        break;
                    case 3447:
                        tradeRangeSkills.mVisibility = element.attribute("level").toInt();
                        break;
                    case 16622:
                        feeSkills.mAccounting = element.attribute("level").toInt();
                        break;
                    case 3446:
                        feeSkills.mBrokerRelations = element.attribute("level").toInt();
                        break;
                    case 16597:
                        feeSkills.mMarginTrading = element.attribute("level").toInt();
                        break;
                    case 25235:
                        contractSkills.mContracting = element.attribute("level").toInt();
                        break;
                    case 12180:
                        reprocessingSkills.mArkonorProcessing = element.attribute("level").toInt();
                        break;
                    case 12181:
                        reprocessingSkills.mBistotProcessing = element.attribute("level").toInt();
                        break;
                    case 12182:
                        reprocessingSkills.mCrokiteProcessing = element.attribute("level").toInt();
                        break;
                    case 12183:
                        reprocessingSkills.mDarkOchreProcessing = element.attribute("level").toInt();
                        break;
                    case 12185:
                        reprocessingSkills.mHedbergiteProcessing = element.attribute("level").toInt();
                        break;
                    case 12186:
                        reprocessingSkills.mHemorphiteProcessing = element.attribute("level").toInt();
                        break;
                    case 18025:
                        reprocessingSkills.mIceProcessing = element.attribute("level").toInt();
                        break;
                    case 12187:
                        reprocessingSkills.mJaspetProcessing = element.attribute("level").toInt();
                        break;
                    case 12188:
                        reprocessingSkills.mKerniteProcessing = element.attribute("level").toInt();
                        break;
                    case 12189:
                        reprocessingSkills.mMercoxitProcessing = element.attribute("level").toInt();
                        break;
                    case 12190:
                        reprocessingSkills.mOmberProcessing = element.attribute("level").toInt();
                        break;
                    case 12191:
                        reprocessingSkills.mPlagioclaseProcessing = element.attribute("level").toInt();
                        break;
                    case 12192:
                        reprocessingSkills.mPyroxeresProcessing = element.attribute("level").toInt();
                        break;
                    case 3385:
                        reprocessingSkills.mReprocessing = element.attribute("level").toInt();
                        break;
                    case 3389:
                        reprocessingSkills.mReprocessingEfficiency = element.attribute("level").toInt();
                        break;
                    case 12193:
                        reprocessingSkills.mScorditeProcessing = element.attribute("level").toInt();
                        break;
                    case 12196:
                        reprocessingSkills.mScrapmetalProcessing = element.attribute("level").toInt();
                        break;
                    case 12194:
                        reprocessingSkills.mSpodumainProcessing = element.attribute("level").toInt();
                        break;
                    case 12195:
                        reprocessingSkills.mVeldsparProcessing = element.attribute("level").toInt();
                    }
                }

                result.setOrderAmountSkills(std::move(orderAmountSkills));
                result.setTradeRangeSkills(std::move(tradeRangeSkills));
                result.setFeeSkills(std::move(feeSkills));
                result.setContractSkills(std::move(contractSkills));
                result.setReprocessingSkills(std::move(reprocessingSkills));
            }

            return result;
        }
    }
}
