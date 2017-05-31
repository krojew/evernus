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
#include <QSqlQuery>

#include "KeyRepository.h"

#include "CharacterRepository.h"

namespace Evernus
{
    QString CharacterRepository::getTableName() const
    {
        return "characters";
    }

    QString CharacterRepository::getIdColumn() const
    {
        return "id";
    }

    CharacterRepository::EntityPtr CharacterRepository::populate(const QSqlRecord &record) const
    {
        const auto keyId = record.value("key_id");

        auto character = std::make_shared<Character>(record.value("id").value<Character::IdType>());
        character->setKeyId((keyId.isNull()) ? (Character::KeyIdType{}) : (keyId.value<Key::IdType>()));
        character->setName(record.value("name").toString());
        character->setCorporationName(record.value("corporation_name").toString());
        character->setCorporationId(record.value("corporation_id").toULongLong());
        character->setRace(record.value("race").toString());
        character->setBloodline(record.value("bloodline").toString());
        character->setAncestry(record.value("ancestry").toString());
        character->setGender(record.value("gender").toString());
        character->setISK(record.value("isk").value<CharacterData::ISKType>());
        character->setCorpStanding(record.value("corp_standing").toFloat());
        character->setFactionStanding(record.value("faction_standing").toFloat());
        character->setEnabled(record.value("enabled").toBool());
        character->setReprocessingImplantBonus(record.value("reprocessing_implant_bonus").toFloat());

        if (!record.value("brokers_fee").isNull())
            character->setBuyBrokersFee(record.value("brokers_fee").toDouble());
        if (!record.value("sell_brokers_fee").isNull())
            character->setSellBrokersFee(record.value("sell_brokers_fee").toDouble());

        CharacterData::OrderAmountSkills orderAmountSkills;
        orderAmountSkills.mTrade = record.value("trade_skill").toInt();
        orderAmountSkills.mRetail = record.value("retail_skill").toInt();
        orderAmountSkills.mWholesale = record.value("wholesale_skill").toInt();
        orderAmountSkills.mTycoon = record.value("tycoon_skill").toInt();

        CharacterData::TradeRangeSkills tradeRangeSkills;
        tradeRangeSkills.mMarketing = record.value("marketing_skill").toInt();
        tradeRangeSkills.mProcurement = record.value("procurement_skill").toInt();
        tradeRangeSkills.mDaytrading = record.value("daytrading_skill").toInt();
        tradeRangeSkills.mVisibility = record.value("visibility_skill").toInt();

        CharacterData::FeeSkills feeSkills;
        feeSkills.mAccounting = record.value("accounting_skill").toInt();
        feeSkills.mBrokerRelations = record.value("broker_relations_skill").toInt();
        feeSkills.mMarginTrading = record.value("margin_trading_skill").toInt();

        CharacterData::ContractSkills contractSkills;
        contractSkills.mContracting = record.value("contracting_skill").toInt();
        contractSkills.mCorporationContracting = record.value("corporation_contracting_skill").toInt();

        CharacterData::ReprocessingSkills reprocessingSkills;
        reprocessingSkills.mArkonorProcessing = record.value("arkonor_processing").toInt();
        reprocessingSkills.mBistotProcessing = record.value("bistot_processing").toInt();
        reprocessingSkills.mCrokiteProcessing = record.value("crokite_processing").toInt();
        reprocessingSkills.mDarkOchreProcessing = record.value("dark_ochre_processing").toInt();
        reprocessingSkills.mGneissProcessing = record.value("gneiss_processing").toInt();
        reprocessingSkills.mHedbergiteProcessing = record.value("hedbergite_processing").toInt();
        reprocessingSkills.mHemorphiteProcessing = record.value("hemorphite_processing").toInt();
        reprocessingSkills.mIceProcessing = record.value("ice_processing").toInt();
        reprocessingSkills.mJaspetProcessing = record.value("jaspet_processing").toInt();
        reprocessingSkills.mKerniteProcessing = record.value("kernite_processing").toInt();
        reprocessingSkills.mMercoxitProcessing = record.value("mercoxit_processing").toInt();
        reprocessingSkills.mOmberProcessing = record.value("omber_processing").toInt();
        reprocessingSkills.mPlagioclaseProcessing = record.value("plagioclase_processing").toInt();
        reprocessingSkills.mPyroxeresProcessing = record.value("pyroxeres_processing").toInt();
        reprocessingSkills.mReprocessing = record.value("reprocessing").toInt();
        reprocessingSkills.mReprocessingEfficiency = record.value("reprocessing_efficiency").toInt();
        reprocessingSkills.mScorditeProcessing = record.value("scordite_processing").toInt();
        reprocessingSkills.mScrapmetalProcessing = record.value("scrapmetal_processing").toInt();
        reprocessingSkills.mSpodumainProcessing = record.value("spodumain_processing").toInt();
        reprocessingSkills.mVeldsparProcessing = record.value("veldspar_processing").toInt();

        character->setOrderAmountSkills(std::move(orderAmountSkills));
        character->setTradeRangeSkills(std::move(tradeRangeSkills));
        character->setFeeSkills(std::move(feeSkills));
        character->setContractSkills(std::move(contractSkills));
        character->setReprocessingSkills(std::move(reprocessingSkills));
        character->setNew(false);

        return character;
    }

    void CharacterRepository::create(const KeyRepository &keyRepository) const
    {
        exec(getCreateQuery(keyRepository));
        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(key_id)"}.arg(getTableName()).arg(keyRepository.getTableName()));
    }

    void CharacterRepository::updateSkill(Character::IdType id, const QString &skill, int level) const
    {
        auto query = prepare(QString{"UPDATE %1 SET %2 = ? WHERE %3 = ?"}
            .arg(getTableName())
            .arg(skill)
            .arg(getIdColumn()));
        query.bindValue(0, level);
        query.bindValue(1, id);

        DatabaseUtils::execQuery(query);
    }

    void CharacterRepository::updateStanding(Character::IdType id, const QString &type, double value) const
    {
        auto query = prepare(QString{"UPDATE %1 SET %2 = ? WHERE %3 = ?"}
            .arg(getTableName())
            .arg(type)
            .arg(getIdColumn()));
        query.bindValue(0, value);
        query.bindValue(1, id);

        DatabaseUtils::execQuery(query);
    }

    void CharacterRepository::updateBrokersFee(Character::IdType id, const boost::optional<double> &buy, const boost::optional<double> &sell) const
    {
        auto query = prepare(QString{"UPDATE %1 SET brokers_fee = ?, sell_brokers_fee = ? WHERE %2 = ?"}
            .arg(getTableName())
            .arg(getIdColumn()));
        query.bindValue(0, (buy) ? (*buy) : (QVariant{QVariant::Double}));
        query.bindValue(1, (sell) ? (*sell) : (QVariant{QVariant::Double}));
        query.bindValue(2, id);

        DatabaseUtils::execQuery(query);
    }

    void CharacterRepository::updateReprocessingImplantBonus(Character::IdType id, double value) const
    {
        auto query = prepare(QString{"UPDATE %1 SET reprocessing_implant_bonus = ? WHERE %2 = ?"}
            .arg(getTableName())
            .arg(getIdColumn()));
        query.bindValue(0, value);
        query.bindValue(1, id);

        DatabaseUtils::execQuery(query);
    }

    void CharacterRepository::disableByKey(Key::IdType id) const
    {
        auto query = prepare(QString{"UPDATE %1 SET key_id = NULL, enabled = 0 WHERE key_id = ?"}.arg(getTableName()));
        query.bindValue(0, id);

        DatabaseUtils::execQuery(query);
    }

    void CharacterRepository::disableByKey(Key::IdType id, const std::vector<Character::IdType> &excluded) const
    {
        QStringList ids;
        for (auto i = 0u; i < excluded.size(); ++i)
            ids << "?";

        auto query = prepare(QString{"UPDATE %1 SET key_id = NULL, enabled = 0 WHERE key_id = ? AND %2 NOT IN (%3)"}
            .arg(getTableName())
            .arg(getIdColumn())
            .arg(ids.join(", ")));

        query.addBindValue(id);

        for (const auto &character : excluded)
            query.addBindValue(character);

        DatabaseUtils::execQuery(query);
    }

    bool CharacterRepository::hasCharacters() const
    {
        auto query = exec(QString{"SELECT COUNT(*) FROM %1"}.arg(getTableName()));
        query.next();

        return query.value(0).toUInt() != 0;
    }

    std::unordered_set<Character::IdType> CharacterRepository::fetchAllIds() const
    {
        std::unordered_set<Character::IdType> result;

        auto query = exec(QString{"SELECT id FROM %1"}.arg(getTableName()));
        while (query.next())
            result.emplace(query.value(0).value<Character::IdType>());

        return result;
    }

    QString CharacterRepository::getNameColumn() const
    {
        return "name";
    }

    quint64 CharacterRepository::getCorporationId(Character::IdType id) const
    {
        auto query = prepare(QString{"SELECT corporation_id FROM %1 WHERE %2 = ?"}.arg(getTableName()).arg(getIdColumn()));
        query.bindValue(0, id);

        DatabaseUtils::execQuery(query);

        if (!query.next())
            throw NotFoundException{};

        return query.value(0).toULongLong();
    }

    QString CharacterRepository::getName(Character::IdType id) const
    {
        auto query = prepare(QString{"SELECT name FROM %1 WHERE %2 = ?"}.arg(getTableName()).arg(getIdColumn()));
        query.bindValue(0, id);

        DatabaseUtils::execQuery(query);

        if (!query.next())
            throw NotFoundException{};

        return query.value(0).toString();
    }

    QSqlQuery CharacterRepository::getEnabledQuery() const
    {
        return exec(QString{"SELECT %2, name FROM %1 WHERE enabled != 0 AND key_id IS NOT NULL ORDER BY name"}
            .arg(getTableName())
            .arg(getIdColumn()));
    }

    QString CharacterRepository::getCreateQuery(const KeyRepository &keyRepository) const
    {
        return QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
            "id BIGINT PRIMARY KEY,"
            "key_id INTEGER NULL REFERENCES %2(id) ON UPDATE SET NULL ON DELETE SET NULL,"
            "name TEXT NOT NULL,"
            "corporation_name TEXT NOT NULL,"
            "corporation_id BIGINT NOT NULL,"
            "race TEXT NOT NULL,"
            "bloodline TEXT NOT NULL,"
            "ancestry TEXT NOT NULL,"
            "gender TEXT NOT NULL,"
            "isk DOUBLE NOT NULL,"
            "corp_standing FLOAT NOT NULL,"
            "faction_standing FLOAT NOT NULL,"
            "trade_skill TINYINT NOT NULL,"
            "retail_skill TINYINT NOT NULL,"
            "wholesale_skill TINYINT NOT NULL,"
            "tycoon_skill TINYINT NOT NULL,"
            "marketing_skill TINYINT NOT NULL,"
            "procurement_skill TINYINT NOT NULL,"
            "daytrading_skill TINYINT NOT NULL,"
            "visibility_skill TINYINT NOT NULL,"
            "accounting_skill TINYINT NOT NULL,"
            "broker_relations_skill TINYINT NOT NULL,"
            "margin_trading_skill TINYINT NOT NULL,"
            "contracting_skill TINYINT NOT NULL,"
            "corporation_contracting_skill TINYINT NOT NULL,"
            "enabled TINYINT NOT NULL,"
            "brokers_fee FLOAT NULL,"
            "sell_brokers_fee FLOAT NULL,"
            "reprocessing_implant_bonus FLOAT NULL,"
            "arkonor_processing TINYINT NOT NULL,"
            "bistot_processing TINYINT NOT NULL,"
            "crokite_processing TINYINT NOT NULL,"
            "dark_ochre_processing TINYINT NOT NULL,"
            "gneiss_processing TINYINT NOT NULL,"
            "hedbergite_processing TINYINT NOT NULL,"
            "hemorphite_processing TINYINT NOT NULL,"
            "ice_processing TINYINT NOT NULL,"
            "jaspet_processing TINYINT NOT NULL,"
            "kernite_processing TINYINT NOT NULL,"
            "mercoxit_processing TINYINT NOT NULL,"
            "omber_processing TINYINT NOT NULL,"
            "plagioclase_processing TINYINT NOT NULL,"
            "pyroxeres_processing TINYINT NOT NULL,"
            "reprocessing TINYINT NOT NULL,"
            "reprocessing_efficiency TINYINT NOT NULL,"
            "scordite_processing TINYINT NOT NULL,"
            "scrapmetal_processing TINYINT NOT NULL,"
            "spodumain_processing TINYINT NOT NULL,"
            "veldspar_processing TINYINT NOT NULL"
        ")").arg(getTableName()).arg(keyRepository.getTableName());
    }

    QStringList CharacterRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "key_id"
            << "name"
            << "corporation_name"
            << "corporation_id"
            << "race"
            << "bloodline"
            << "ancestry"
            << "gender"
            << "isk"
            << "corp_standing"
            << "faction_standing"
            << "trade_skill"
            << "retail_skill"
            << "wholesale_skill"
            << "tycoon_skill"
            << "marketing_skill"
            << "procurement_skill"
            << "daytrading_skill"
            << "visibility_skill"
            << "accounting_skill"
            << "broker_relations_skill"
            << "margin_trading_skill"
            << "contracting_skill"
            << "corporation_contracting_skill"
            << "enabled"
            << "brokers_fee"
            << "sell_brokers_fee"
            << "reprocessing_implant_bonus"
            << "arkonor_processing"
            << "bistot_processing"
            << "crokite_processing"
            << "dark_ochre_processing"
            << "gneiss_processing"
            << "hedbergite_processing"
            << "hemorphite_processing"
            << "ice_processing"
            << "jaspet_processing"
            << "kernite_processing"
            << "mercoxit_processing"
            << "omber_processing"
            << "plagioclase_processing"
            << "pyroxeres_processing"
            << "reprocessing"
            << "reprocessing_efficiency"
            << "scordite_processing"
            << "scrapmetal_processing"
            << "spodumain_processing"
            << "veldspar_processing";
    }

    void CharacterRepository::bindValues(const Character &entity, QSqlQuery &query) const
    {
        const auto keyId = entity.getKeyId();

        const auto orderAmountSkills = entity.getOrderAmountSkills();
        const auto tradeRangeSkills = entity.getTradeRangeSkills();
        const auto feeSkills = entity.getFeeSkills();
        const auto contractSkills = entity.getContractSkills();
        const auto reprocessingSkills = entity.getReprocessingSkills();

        if (entity.getId() != Character::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":key_id", (keyId) ? (*keyId) : (QVariant{QVariant::UInt}));
        query.bindValue(":name", entity.getName());
        query.bindValue(":corporation_name", entity.getCorporationName());
        query.bindValue(":corporation_id", entity.getCorporationId());
        query.bindValue(":race", entity.getRace());
        query.bindValue(":bloodline", entity.getBloodline());
        query.bindValue(":ancestry", entity.getAncestry());
        query.bindValue(":gender", entity.getGender());
        query.bindValue(":isk", entity.getISK());
        query.bindValue(":corp_standing", entity.getCorpStanding());
        query.bindValue(":faction_standing", entity.getFactionStanding());
        query.bindValue(":trade_skill", orderAmountSkills.mTrade);
        query.bindValue(":retail_skill", orderAmountSkills.mRetail);
        query.bindValue(":wholesale_skill", orderAmountSkills.mWholesale);
        query.bindValue(":tycoon_skill", orderAmountSkills.mTycoon);
        query.bindValue(":marketing_skill", tradeRangeSkills.mMarketing);
        query.bindValue(":procurement_skill", tradeRangeSkills.mProcurement);
        query.bindValue(":daytrading_skill", tradeRangeSkills.mDaytrading);
        query.bindValue(":visibility_skill", tradeRangeSkills.mVisibility);
        query.bindValue(":accounting_skill", feeSkills.mAccounting);
        query.bindValue(":broker_relations_skill", feeSkills.mBrokerRelations);
        query.bindValue(":margin_trading_skill", feeSkills.mMarginTrading);
        query.bindValue(":contracting_skill", contractSkills.mContracting);
        query.bindValue(":corporation_contracting_skill", contractSkills.mCorporationContracting);
        query.bindValue(":enabled", entity.isEnabled());
        query.bindValue(":brokers_fee", (entity.getBuyBrokersFee()) ? (*entity.getBuyBrokersFee()) : (QVariant{QVariant::Double}));
        query.bindValue(":sell_brokers_fee", (entity.getBuyBrokersFee()) ? (*entity.getSellBrokersFee()) : (QVariant{QVariant::Double}));
        query.bindValue(":reprocessing_implant_bonus", entity.getReprocessingImplantBonus());
        query.bindValue(":arkonor_processing", reprocessingSkills.mArkonorProcessing);
        query.bindValue(":bistot_processing", reprocessingSkills.mBistotProcessing);
        query.bindValue(":crokite_processing", reprocessingSkills.mCrokiteProcessing);
        query.bindValue(":dark_ochre_processing", reprocessingSkills.mDarkOchreProcessing);
        query.bindValue(":gneiss_processing", reprocessingSkills.mGneissProcessing);
        query.bindValue(":hedbergite_processing", reprocessingSkills.mHedbergiteProcessing);
        query.bindValue(":hemorphite_processing", reprocessingSkills.mHemorphiteProcessing);
        query.bindValue(":ice_processing", reprocessingSkills.mIceProcessing);
        query.bindValue(":jaspet_processing", reprocessingSkills.mJaspetProcessing);
        query.bindValue(":kernite_processing", reprocessingSkills.mKerniteProcessing);
        query.bindValue(":mercoxit_processing", reprocessingSkills.mMercoxitProcessing);
        query.bindValue(":omber_processing", reprocessingSkills.mOmberProcessing);
        query.bindValue(":plagioclase_processing", reprocessingSkills.mPlagioclaseProcessing);
        query.bindValue(":pyroxeres_processing", reprocessingSkills.mPyroxeresProcessing);
        query.bindValue(":reprocessing", reprocessingSkills.mReprocessing);
        query.bindValue(":reprocessing_efficiency", reprocessingSkills.mReprocessingEfficiency);
        query.bindValue(":scordite_processing", reprocessingSkills.mScorditeProcessing);
        query.bindValue(":scrapmetal_processing", reprocessingSkills.mScrapmetalProcessing);
        query.bindValue(":spodumain_processing", reprocessingSkills.mSpodumainProcessing);
        query.bindValue(":veldspar_processing", reprocessingSkills.mVeldsparProcessing);
    }

    void CharacterRepository::bindPositionalValues(const Character &entity, QSqlQuery &query) const
    {
        const auto keyId = entity.getKeyId();

        const auto orderAmountSkills = entity.getOrderAmountSkills();
        const auto tradeRangeSkills = entity.getTradeRangeSkills();
        const auto feeSkills = entity.getFeeSkills();
        const auto contractSkills = entity.getContractSkills();
        const auto reprocessingSkills = entity.getReprocessingSkills();

        if (entity.getId() != Character::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue((keyId) ? (*keyId) : (QVariant{QVariant::UInt}));
        query.addBindValue(entity.getName());
        query.addBindValue(entity.getCorporationName());
        query.addBindValue(entity.getCorporationId());
        query.addBindValue(entity.getRace());
        query.addBindValue(entity.getBloodline());
        query.addBindValue(entity.getAncestry());
        query.addBindValue(entity.getGender());
        query.addBindValue(entity.getISK());
        query.addBindValue(entity.getCorpStanding());
        query.addBindValue(entity.getFactionStanding());
        query.addBindValue(orderAmountSkills.mTrade);
        query.addBindValue(orderAmountSkills.mRetail);
        query.addBindValue(orderAmountSkills.mWholesale);
        query.addBindValue(orderAmountSkills.mTycoon);
        query.addBindValue(tradeRangeSkills.mMarketing);
        query.addBindValue(tradeRangeSkills.mProcurement);
        query.addBindValue(tradeRangeSkills.mDaytrading);
        query.addBindValue(tradeRangeSkills.mVisibility);
        query.addBindValue(feeSkills.mAccounting);
        query.addBindValue(feeSkills.mBrokerRelations);
        query.addBindValue(feeSkills.mMarginTrading);
        query.addBindValue(contractSkills.mContracting);
        query.addBindValue(contractSkills.mCorporationContracting);
        query.addBindValue(entity.isEnabled());
        query.addBindValue((entity.getBuyBrokersFee()) ? (*entity.getBuyBrokersFee()) : (QVariant{QVariant::Double}));
        query.addBindValue((entity.getSellBrokersFee()) ? (*entity.getSellBrokersFee()) : (QVariant{QVariant::Double}));
        query.addBindValue(entity.getReprocessingImplantBonus());
        query.addBindValue(reprocessingSkills.mArkonorProcessing);
        query.addBindValue(reprocessingSkills.mBistotProcessing);
        query.addBindValue(reprocessingSkills.mCrokiteProcessing);
        query.addBindValue(reprocessingSkills.mDarkOchreProcessing);
        query.addBindValue(reprocessingSkills.mGneissProcessing);
        query.addBindValue(reprocessingSkills.mHedbergiteProcessing);
        query.addBindValue(reprocessingSkills.mHemorphiteProcessing);
        query.addBindValue(reprocessingSkills.mIceProcessing);
        query.addBindValue(reprocessingSkills.mJaspetProcessing);
        query.addBindValue(reprocessingSkills.mKerniteProcessing);
        query.addBindValue(reprocessingSkills.mMercoxitProcessing);
        query.addBindValue(reprocessingSkills.mOmberProcessing);
        query.addBindValue(reprocessingSkills.mPlagioclaseProcessing);
        query.addBindValue(reprocessingSkills.mPyroxeresProcessing);
        query.addBindValue(reprocessingSkills.mReprocessing);
        query.addBindValue(reprocessingSkills.mReprocessingEfficiency);
        query.addBindValue(reprocessingSkills.mScorditeProcessing);
        query.addBindValue(reprocessingSkills.mScrapmetalProcessing);
        query.addBindValue(reprocessingSkills.mSpodumainProcessing);
        query.addBindValue(reprocessingSkills.mVeldsparProcessing);
    }
}
