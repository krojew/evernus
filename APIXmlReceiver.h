#pragma once

#include <memory>
#include <vector>

#include <QAbstractXmlReceiver>

namespace Evernus
{
    template<class T>
    struct APIXmlReceiverAdditionalData { };

    template<class T, class CurElem = std::unique_ptr<T>>
    class APIXmlReceiver
        : public QAbstractXmlReceiver
    {
    public:
        typedef CurElem CurElemType;

        typedef std::vector<T> Container;

        APIXmlReceiver(Container &container, const QXmlNamePool &namePool);
        virtual ~APIXmlReceiver() = default;

        virtual void atomicValue(const QVariant &value) override;
        virtual void attribute(const QXmlName &name, const QStringRef &value) override;
        virtual void characters(const QStringRef &value) override;
        virtual void comment(const QString &value) override;
        virtual void endDocument() override;
        virtual void endElement() override;
        virtual void endOfSequence() override;
        virtual void namespaceBinding(const QXmlName &name) override;
        virtual void processingInstruction(const QXmlName &target, const QString &value) override;
        virtual void startDocument() override;
        virtual void startElement(const QXmlName &name) override;
        virtual void startOfSequence() override;

    private:
        Container &mContainer;
        const QXmlNamePool &mNamePool;

        CurElemType mCurrentElement;

        APIXmlReceiverAdditionalData<T> mAdditionalData;

        template<class U>
        static U convert(QVariant value);
    };
}

#include "APIXmlReceiver.cpp"
