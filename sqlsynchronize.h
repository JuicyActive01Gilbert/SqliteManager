#ifndef SQLSYNCHRONIZE_H
#define SQLSYNCHRONIZE_H

#include <QObject>
#include "sqlitemanager.h"

class SqlSynchronize : public QObject
{
    Q_OBJECT
public:
    explicit SqlSynchronize(const QString &dbPath,
                                 const QString &synDBPath,
                                 QObject *parent = nullptr);
    ~SqlSynchronize();

    void synchronize();

signals:
    void haveError(QString);
    void progressStatus(int,int);//current,max
protected:
    void initialize();
    void synchronizeTables(const QStringList &tables);
    void synchronizeTable(const QString &table);
    void createSynTales(const QStringList &tables);
    void createSynTablesColums(const QStringList &synTables);//Same table, field changes
private:
    SqliteManager *m_pDB = nullptr;
    SqliteManager *m_pSynDB = nullptr;

    QString m_dbPath;
    QString m_synDBPath;

    QStringList m_synedTables;

};

#endif // SQLSYNCHRONIZE_H
