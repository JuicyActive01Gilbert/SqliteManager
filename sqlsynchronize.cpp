#include "sqlsynchronize.h"
#include <QDebug>
#include <QUuid>

SqlSynchronize::SqlSynchronize(const QString &dbPath,
                                         const QString &synDBPath,
                                         QObject *parent):
    m_dbPath(dbPath),
    m_synDBPath(synDBPath),
    QObject(parent)
{
    initialize();
}

SqlSynchronize::~SqlSynchronize()
{

}

void SqlSynchronize::synchronize()
{
    QStringList synTables = m_pSynDB->getTableList();

    createSynTales(synTables);
    createSynTablesColums(synTables);

    m_synedTables.clear();
    synchronizeTables(synTables);
}

void SqlSynchronize::initialize()
{
    m_pDB = new SqliteManager(this);
    m_pSynDB = new SqliteManager(this);

    m_pDB->setDBName(m_dbPath,QUuid::createUuid().toString().replace("-",""););
    if(!m_pDB->openDB()){
        emit haveError(m_pDB->lastErrorString());
        return;
    }
    m_pDB->closeDB();

    m_pSynDB->setDBName(m_synDBPath,QUuid::createUuid().toString().replace("-",""););
    if(!m_pSynDB->openDB()){
        emit haveError(m_pSynDB->lastErrorString());
        return;
    }
    m_pSynDB->closeDB();
}

void SqlSynchronize::synchronizeTables(const QStringList &tables)
{
    int i = 0;
    int count = tables.count();
    foreach (QString synTable, tables) {
        synchronizeTable(synTable);
        emit progressStatus(i + 1,count);
    }

}

void SqlSynchronize::synchronizeTable(const QString &table)
{
    if(m_synedTables.contains(table)){
        return;
    }

    QStringList parentTables = m_pSynDB->getForeignTables(table);

    if(parentTables.count() != 0){
        foreach (QString parentTable, parentTables) {
            if(m_synedTables.contains(parentTable)){
                continue;
            }else{
                synchronizeTable(parentTable);
            }
        }
    }

    QVariantList synDatas = m_pSynDB->selectDatas(table);
    QVariantList datas = m_pDB->selectDatas(table);

    foreach (QVariant data, datas) {
        foreach (QVariant synData, synDatas) {
            if(data == synData){
                synDatas.removeOne(synData);
            }
        }
    }

    QStringList pks = m_pDB->getTablePrimarykeys(table);

    foreach (QVariant synData, synDatas) {
        QVariantMap map = synData.toMap();
        QVariantMap condition;
        foreach (QString key, pks) {
            condition.insert(key,map.value(key));
        }
        int count = m_pDB->selectDatas(table,condition).count();

        //update
        if(count != 0){
            m_pDB->updateData(table,map,condition);
        }else{//insert
            m_pDB->insertData(table,map);
        }

    }

    m_synedTables.append(table);
}

void SqlSynchronize::createSynTales(const QStringList &tables)
{
    foreach (QString table, tables) {
        if(m_pDB->getTableInfo(table).count() != 0){
            continue;
        }else {
            QStringList parentTables = m_pSynDB->getForeignTables(table);

            if(parentTables.count() != 0){
                createSynTales(parentTables);
            }
            m_pDB->createTable(table,m_pSynDB->getTableInfo(table),m_pSynDB->getForeignKeys(table));
        }
    }
}

void SqlSynchronize::createSynTablesColums(const QStringList &synTables)
{
    QStringList tables = m_pDB->getTableList();
    foreach (QString table, synTables) {
        if(!tables.contains(table)){
            continue;
        }

        QVariantList synTableInfo = m_pSynDB->getTableInfo(table);
        foreach (QVariant synCol, synTableInfo) {
            if(m_pDB->haveTableColum(table,synCol.toMap())){
                continue;
            }
            if(!m_pDB->addColumInTable(table,synCol.toMap())){
                emit haveError(m_pDB->lastErrorString());
            }
        }
    }
}



