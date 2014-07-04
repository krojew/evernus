#pragma once

#include <memory>
#include <string>

namespace Evernus
{
    class ItemPriceImporter;

    class ItemPriceImporterRegistry
    {
    public:
        typedef std::unique_ptr<ItemPriceImporter> ImporterPtr;

        ItemPriceImporterRegistry() = default;
        virtual ~ItemPriceImporterRegistry() = default;

        virtual void registerImporter(const std::string &name, ImporterPtr &&importer) = 0;
    };
}
