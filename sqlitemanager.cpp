#include "sqlitemanager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantList>
#include <QSqlRecord>

#include <QDebug>

SqliteManager::SqliteManager(QObject *parent) :
    QObject(parent)
{

}

SqliteManager::~SqliteManager()
{
    closeDB();
}

void SqliteManager::setDBName(const QString &dbName,
                                   const QString &connectName)
{
    m_strDBName = dbName;
    m_connectName = connectName;
    initialize();
}

bool SqliteManager::openDB()
{
    bool isOpen = m_db.open();
    m_strError = m_db.lastError().text();
    return isOpen;
}

void SqliteManager::closeDB()
{
    if(m_db.isOpen()){
        m_db.close();
    }
}

bool SqliteManager::insertData(const QString &table, const QVariantMap &data)
{
    if(!openDB()){
        return false;
    }
    QStringList keys = data.keys();
    QString sql = "insert into " + table;
    QString sqlItemName = "(";
    QString sqlItemVal = ") values(";

    m_strError = "";

    foreach (QString key, keys) {
        QVariant value = data.value(key);
        QVariant::Type type = value.type();
        sqlItemName += QString("%1,").arg(key);
        switch (type) {
        case QVariant::Int:case QVariant::Double:case QVariant::LongLong:
        {
            sqlItemVal += QString("%1,").arg(value.toString());
        }
            break;
        case QVariant::String:
        {
            sqlItemVal += QString("'%1',").arg(value.toString());
        }
            break;
        default:
            break;
        }
    }

    sqlItemName.truncate(sqlItemName.count() - 1);//减去最后一个','
    sqlItemVal.truncate(sqlItemVal.count() - 1);//减去最后一个','
    sqlItemVal += ")";

    sql += (sqlItemName + sqlItemVal);

    QSqlQuery query = QSqlQuery(m_db);

    //执行SQL语句
    if(!query.exec(sql))
    {
        m_strError = query.lastError().text();
        closeDB();
        return false;
    }

    closeDB();
    return true;
}

int SqliteManager::insertDatas(const QString &table, const QVariantList &datas)
{
    int count = 0;
    foreach (QVariant var, datas) {
        if(insertData(table,var.toMap())){
            count++;
        }
    }

    return count;
}

bool SqliteManager::deleteData(const QString &table, const QVariantMap &conditions)
{
    if(!openDB()){
        return false;
    }
    m_strError = "";
    QString sql = QString("delete from %1 where ").arg(table) + getCondition(conditions);
    QSqlQuery query = QSqlQuery(m_db);

    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        closeDB();
        return false;
    }

    closeDB();
    return true;
}

bool SqliteManager::updateData(const QString &table,
                                    const QVariantMap &data,
                                    const QVariantMap &conditions)
{
    if(!openDB()){
        return false;
    }
    QString sql = QString("update %1 set ").arg(table);
    QStringList keys = data.keys();
    m_strError = "";
    foreach (QString key, keys) {
        QVariant value = data.value(key);
        QVariant::Type type = value.type();

        switch (type) {
        case QVariant::Int:case QVariant::Double:case QVariant::LongLong:
        {
            sql += QString("%1 = %2,").arg(key).arg(value.toString());
        }
            break;
        case QVariant::String:
        {
            sql += QString("%1 = '%2',").arg(key).arg(value.toString());
        }
            break;
        default:
            break;
        }
    }

    if(sql.endsWith(",")){
        sql.truncate(sql.count() - 1);
    }

    sql += " where " + getCondition(conditions);
    QSqlQuery query = QSqlQuery(m_db);

    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        closeDB();
        return false;
    }

    closeDB();
    return true;
}

QVariantList SqliteManager::selectDatas(const QString &table, const QVariantMap &conditions)
{
    QVariantList datas;
    if(!openDB()){
        return datas;
    }
    m_strError = "";
    QString sql = QString("select * from %1 where ").arg(table);

    if(conditions.isEmpty()){
        sql += "1=1";
    }else{
        sql += getCondition(conditions);
    }

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return datas;
    }

    while(query.next()){
        QVariantMap map;
        QSqlRecord record = query.record();
        int count = record.count();
        for(int i = count - 1;i >= 0;i--){
            QString name = record.fieldName(i);
            map.insert(name,record.value(name));
        }
        datas.append(map);
    }
    closeDB();
    return datas;
}

QStringList SqliteManager::getTableList()
{
    QStringList tables;
    if(!openDB()){
        return tables;
    }

    QString sql = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return tables;
    }

    while(query.next()){
        QSqlRecord record = query.record();
        if(!record.contains("name")){
            continue;
        }
        tables.append(record.value("name").toString());

    }

    closeDB();
    return tables;
}

QStringList SqliteManager::getForeignTables(const QString &table)
{
    QStringList tables;
    QVariantList foreignKeys = getForeignKeys(table);

    foreach (QVariant key, foreignKeys) {
        QVariantMap map = key.toMap();
        tables.append(map.value("table").toString());
    }
    return tables;
}

QStringList SqliteManager::getTablePrimarykeys(const QString &table)
{
    QStringList names;
    QVariantList tableInfo = getTableInfo(table);

    foreach (QVariant col, tableInfo) {
        QVariantMap map = col.toMap();
        int pk = map.value("pk").toInt();
        if(pk != 0){
            names.append(map.value("name").toString());
        }
    }

    return names;
}

QVariantList SqliteManager::getTableInfo(const QString &table)
{
    QVariantList tableInfo;
    if(!openDB()){
        return tableInfo;
    }

    QString sql = "PRAGMA table_info('%1');";
    sql = sql.arg(table);

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return tableInfo;
    }

    while(query.next()){
        QSqlRecord record = query.record();
        int count = record.count();
        if(count != 6){
            continue;
        }
        QVariantMap map;

        for(int i = count - 1;i >= 0;i--){
            QString name = record.fieldName(i);
            map.insert(name,record.value(name));
        }
        tableInfo.append(map);
    }

    closeDB();
    return tableInfo;
}

QVariantList SqliteManager::getForeignKeys(const QString &table)
{
    QVariantList keys;
    if(!openDB()){
        return keys;
    }

    QString sql = "PRAGMA foreign_key_list('%1');";
    sql = sql.arg(table);

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return keys;
    }

    while(query.next()){
        QSqlRecord record = query.record();
        int count = record.count();
        if(count != 8){
            continue;
        }
        QVariantMap map;
        for(int i = count - 1;i >= 0;i--){
            QString name = record.fieldName(i);
            map.insert(name,record.value(name));
        }
        keys.append(map);
    }

    closeDB();

    return keys;
}

bool SqliteManager::createTable(const QString &name,
                                     const QVariantList &tableInfo,
                                     const QVariantList &foreignKeys)
{
    QStringList tables = getTableList();

    if(!openDB()){
        return false;
    }
    QStringList names;
    QString sql = "CREATE TABLE %1(";
    sql = sql.arg(name);


    foreach (QVariant tableCol, tableInfo) {
        QVariantMap map = tableCol.toMap();
        QString name = map.value("name").toString();

        names.append(name);
        sql += name + " " + map.value("type").toString() + " ";
        if(map.value("pk").toInt() == 1){
            sql += "PRIMARY KEY ";
        }
        if(map.value("notnull").toInt() == 1){
            sql += "NOT NULL,";
        }else{
            sql += "NULL,";
        }
    }

    foreach (QVariant key, foreignKeys) {
        QVariantMap map = key.toMap();
        QString table = map.value("table").toString();
        QString from = map.value("from").toString();
        QString onDelete = map.value("on_delete").toString();
        QString onUpdate = map.value("on_update").toString();
        QString to = map.value("to").toString();
        if(!tables.contains(table) ||
               !names.contains(from) ){
            continue;
        }

        sql += QString("FOREIGN KEY(%1) REFERENCES %2(%3) ON UPDATE %4 ON DELETE %5,").
                arg(from).arg(table).arg(to).arg(onUpdate).arg(onDelete);

    }

    sql.chop(1);//delete last ,
    sql += ");";

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return false;
    }

    closeDB();
    return true;
}

bool SqliteManager::addColumInTable(const QString &table,
                                         const QVariantMap &col)
{
    if(!openDB()){
        return false;
    }
    QString sql = "ALTER TABLE %1 ADD COLUMN %2 %3;";

    sql = sql.arg(table).arg(col.value("name").toString())
            .arg(col.value("type").toString());

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return false;
    }

    closeDB();
    return true;
}

bool SqliteManager::deleteTable(const QString &table)
{
    if(!openDB()){
        return false;
    }

    QString sql = "DROP TABLE %1;";
    sql = sql.arg(table);

    QSqlQuery query = QSqlQuery(m_db);
    if(!query.exec(sql)){
        m_strError = query.lastError().text();
        qDebug() << sql << m_strError;
        closeDB();
        return false;
    }

    closeDB();
    return true;
}

bool SqliteManager::haveTableColum(const QString &table, const QVariantMap &col)
{
    QVariantList cols = getTableInfo(table);

    foreach (QVariant data, cols) {
        if(data.toMap() == col){
            return true;
        }
    }

    return false;
}

QString SqliteManager::lastErrorString()
{
    return m_strError;
}

void SqliteManager::initialize()
{
    if(QSqlDatabase::contains(m_connectName)){
        m_db = QSqlDatabase::database("QSQLITE");
    }else{
        m_db = QSqlDatabase::addDatabase("QSQLITE",m_connectName);
    }
    m_db.setDatabaseName(m_strDBName);
}

QString SqliteManager::getCondition(const QVariantMap &conditions)
{
    QString strCondition;
    QStringList keys = conditions.keys();

    foreach (QString key, keys) {
        QVariant value = conditions.value(key);
        QVariant::Type type = value.type();

        switch (type) {
        case QVariant::Int:case QVariant::Double:
        {
            strCondition += QString("%1 = %2").arg(key).arg(value.toString());
        }
            break;
        case QVariant::String:
        {
            strCondition += QString("%1 = '%2'").arg(key).arg(value.toString());
        }
            break;
        default:
            break;
        }

        strCondition += " and ";
    }

    if(strCondition.endsWith(" and ")){
        strCondition.truncate(strCondition.count() - 5);
    }

    return strCondition;
}
