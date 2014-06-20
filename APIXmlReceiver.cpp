#include "APIXmlReceiver.h"

namespace Evernus
{
    template<class T>
    APIXmlReceiver<T>::APIXmlReceiver(Container &container, const QXmlNamePool &namePool)
        : QAbstractXmlReceiver{}
        , mContainer{container}
        , mNamePool{namePool}
    {
    }

    template<class T>
    void APIXmlReceiver<T>::atomicValue(const QVariant &value)
    {
        Q_UNUSED(value);
    }

    template<class T>
    void APIXmlReceiver<T>::attribute(const QXmlName &name, const QStringRef &value)
    {
        Q_UNUSED(name);
        Q_UNUSED(value);
    }

    template<class T>
    void APIXmlReceiver<T>::characters(const QStringRef &value)
    {
        Q_UNUSED(value);
    }

    template<class T>
    void APIXmlReceiver<T>::comment(const QString &value)
    {
        Q_UNUSED(value);
    }

    template<class T>
    void APIXmlReceiver<T>::endDocument()
    {
    }

    template<class T>
    void APIXmlReceiver<T>::endElement()
    {
        mContainer.emplace_back(*mCurrentElement);
    }

    template<class T>
    void APIXmlReceiver<T>::endOfSequence()
    {
    }

    template<class T>
    void APIXmlReceiver<T>::namespaceBinding(const QXmlName &name)
    {
        Q_UNUSED(name);
    }

    template<class T>
    void APIXmlReceiver<T>::processingInstruction(const QXmlName &target, const QString &value)
    {
        Q_UNUSED(target);
        Q_UNUSED(value);
    }

    template<class T>
    void APIXmlReceiver<T>::startDocument()
    {
    }

    template<class T>
    void APIXmlReceiver<T>::startElement(const QXmlName &name)
    {
        Q_UNUSED(name);

        mCurrentElement.reset(new T{});
    }

    template<class T>
    void APIXmlReceiver<T>::startOfSequence()
    {
    }
}
