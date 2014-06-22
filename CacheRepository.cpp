#include <QDateTime>

namespace Evernus
{
    template<class T>
    void CacheRepository<T>::clearOldData() const
    {
        auto query = this->prepare(QString{"DELETE FROM %1 WHERE cache_until < :dt"}.arg(this->getTableName()));
        query.bindValue(":dt", QDateTime::currentDateTimeUtc().toTime_t());
        query.exec();
    }
}
