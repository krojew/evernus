#pragma once

#include "Repository.h"
#include "Character.h"

namespace Evernus
{
    class KeyRepository;

    class CharacterRepository
        : public Repository<Character>
    {
    public:
        using Repository::Repository;
        virtual ~CharacterRepository() = default;

        virtual QString getTableName() const override;

        virtual Character populate(const QSqlRecord &record) const override;

        void create(const KeyRepository &keyRepository) const;

    private:
        virtual QStringList getColumns() const override;
        virtual QString getIdColumn() const override;
        virtual void bindValues(const Character &entity, QSqlQuery &query) const override;
    };
}
