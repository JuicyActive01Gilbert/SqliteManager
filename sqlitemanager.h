#ifndef SQLITEMANAGER_H
#define SQLITEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QVariantMap>

class SqliteManager : public QObject
{
    Q_OBJECT
public:
    explicit SqliteManager(QObject *parent = nullptr);
    ~SqliteManager();

    void setDBName(const QString &dbName,
                   const QString &connectName);

    bool openDB();
    void closeDB();

    bool insertData(const QString &table,const QVariantMap &data);
    int insertDatas(const QString &table,const QVariantList &datas);
    bool deleteData(const QString &table,const QVariantMap &conditions);
    bool updateData(const QString &table,const QVariantMap &data,const QVariantMap &conditions);
  //  int updateDatas(const QString &table,const QVariantList &datas,const QVariantMap &conditions);
    QVariantList selectDatas(const QString &table,const QVariantMap &conditions=QVariantMap());

    QStringList getTableList();
    QStringList getForeignTables(const QString &table);
    QStringList getTablePrimarykeys(const QString &table);

    QVariantList getTableInfo(const QString &table);
    QVariantList getForeignKeys(const QString &table);

    //tableInfo is getTableInfo return or query "PRAGMA table_info('table');"
    //foreignKeys is getForeignKeys return or query "PRAGMA foreign_key_list('table');";
    bool createTable(const QString &name,
                     const QVariantList &tableInfo,
                     const QVariantList &foreignKeys = QVariantList());
    bool addColumInTable(const QString &table, const QVariantMap &col);
    bool deleteTable(const QString &table);
    bool haveTableColum(const QString &table, const QVariantMap &col);

    QString lastErrorString();
protected:
    void initialize();

    QString getCondition(const QVariantMap &conditions);
private:
    QString m_strDBName;
    QString m_connectName;
    QString m_strError;
    QSqlDatabase m_db;
};

#endif // SQLITEMANAGER_H
