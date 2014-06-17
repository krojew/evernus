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

        void create(const KeyRepository &keyRepository) const;

    private:
        virtual Character populate(const QSqlQuery &query) const override;
        virtual QStringList getColumns() const override;
        virtual void bindValues(const Character &entity, QSqlQuery &query) const override;
    };
}
