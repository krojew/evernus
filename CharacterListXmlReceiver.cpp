#include <stdexcept>

#include <QCoreApplication>

namespace Evernus
{
    template<>
    void APIXmlReceiver<Character::IdType>::attribute(const QXmlName &name, const QStringRef &value)
    {
        if (name.localName(mNamePool) == "characterID")
        {
            auto ok = false;
            *mCurrentElement = value.toULongLong(&ok);

            if (!ok)
                throw std::runtime_error{QCoreApplication::translate("APIXmlReceiver", "Couldn't convert %1 to id!").arg(value.toString()).toStdString()};
        }
    }
}
