#include <QCoreApplication>

namespace Evernus
{
    template<class T, class CurElem>
    APIXmlReceiver<T, CurElem>::APIXmlReceiver(Container &container, const QXmlNamePool &namePool)
        : QAbstractXmlReceiver{}
        , mContainer{container}
        , mNamePool{namePool}
    {
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::atomicValue(const QVariant &value)
    {
        Q_UNUSED(value);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::attribute(const QXmlName &name, const QStringRef &value)
    {
        Q_UNUSED(name);
        Q_UNUSED(value);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::characters(const QStringRef &value)
    {
        Q_UNUSED(value);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::comment(const QString &value)
    {
        Q_UNUSED(value);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::endDocument()
    {
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::endElement()
    {
        mContainer.emplace_back(*mCurrentElement);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::endOfSequence()
    {
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::namespaceBinding(const QXmlName &name)
    {
        Q_UNUSED(name);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::processingInstruction(const QXmlName &target, const QString &value)
    {
        Q_UNUSED(target);
        Q_UNUSED(value);
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::startDocument()
    {
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::startElement(const QXmlName &name)
    {
        Q_UNUSED(name);

        mCurrentElement.reset(new T{});
    }

    template<class T, class CurElem>
    void APIXmlReceiver<T, CurElem>::startOfSequence()
    {
    }

    template<class T, class CurElem>
    template<class U>
    U APIXmlReceiver<T, CurElem>::convert(QVariant value)
    {
        if (!value.template canConvert<U>())
        {
            throw std::runtime_error{QCoreApplication::translate("APIXmlReceiver", "Couldn't convert %1 to %2!")
                .arg(value.toString())
                .arg(typeid(U).name())
                .toStdString()};
        }

        return value.template value<U>();
    }
}
