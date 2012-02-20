#include "querythread.h"

#include "db.h"

#include <QDebug>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>


Worker::Worker( QObject* parent )
    : QObject( parent )
{
    // thread-specific connection, see db.h
    m_database = QSqlDatabase::addDatabase( DATABASE_DRIVER, 
                                            "WorkerDatabase" ); // named connection
    m_database.setDatabaseName( DATABASE_NAME );
    m_database.setHostName( DATABASE_HOST );    
    if ( !m_database.open() )
    {
        qWarning() << "Unable to connect to database, giving up:" << m_database.lastError().text();
        return;
    }

    // initialize db
    if ( !m_database.tables().contains( "item" ) )
    {
        // some data
        m_database.exec( "create table item(id int, name varchar);" );
        m_database.transaction();
        QSqlQuery query(m_database);
        query.prepare("INSERT INTO item (id, name) "
                      "VALUES (?,?)");
        for ( int i = 0; i < SAMPLE_RECORDS; ++i )
        {
            query.addBindValue(i);
            query.addBindValue(QString::number(i));
            query.exec();
        }
        m_database.commit();
    }

}

Worker::~Worker()
{
    qDeleteAll(m_queries);
}

void Worker::slotExecute( const QString& qId, const QString& sql)
{
    // if sql is not valid, treat this as already prepared statment
    // that needs to be executed
    bool isPrepared = (sql.isEmpty() || sql.isNull());

    if (isPrepared)
        executePrepared(qId);
    else
        executeOneTime(qId, sql);
}

void Worker::executeOneTime(const QString& qId, const QString& sql)
{
    // use previously defined db connection
    QSqlQuery query(m_database);
    // execute query, get result
    bool ok = query.exec(sql);
    // check for errors
    if (!ok) {
        qDebug() << QString("execute failed for one time query id [%1]").arg(qId);
        // emit error signal
        emit executeFailed(qId, query.lastError());
        return;
    }
    // accumulate data and emit result
    QList<QSqlRecord> recs;
    while (query.next())
    {
        recs.append(query.record());
    }
    emit results(qId, recs);
}

void Worker::executePrepared(const QString& qId)
{
    // check if there is query with specified id
    if (!m_queries.contains(qId)) {
        qDebug() << QString("prepared query id [%1] not found").arg(qId);
        return;
    }
    // get query from hash
    QSqlQuery *query;
    query = m_queries.value(qId);
    // execute and check query status
    bool ok = query->exec();

    if (!ok) {
        qDebug() << QString("execute failed for prepared query id [%1]").arg(qId);
        emit executeFailed(qId, query->lastError());
        return;
    }
    // send executed signal
    emit executed(qId);

    QList<QSqlRecord> recs;
    while (query->next())
    {
        recs.append(query->record());
    }
    // result saved - release resources
    query->finish();
    // send results signal
    emit results(qId, recs);
}

void Worker::slotPrepare(const QString &qId, const QString &sql)
{
    // check if there is query with specified id
    if (m_queries.contains(qId)) {
        qDebug() << QString("already prepared query id [%1]").arg(qId);
        return;
    }
    QSqlQuery *query;
    // use previously defined db connection
    query = new QSqlQuery(m_database);
    // prepare and check query status
    bool ok = query->prepare(sql);

    if (!ok) {
        qDebug() << QString("prepare failed for query id [%1]").arg(qId);
        emit prepareFailed(qId, query->lastError());
        return;
    }
    // save query to our list
    m_queries.insert(qId, query);
    // send prepared signal
    emit prepared(qId);
}

void Worker::slotBindValue(const QString &qId, const QString &placeholder, const QVariant &val)
{
    // check if there is query with specified id
    if (!m_queries.contains(qId)) {
        qDebug() << QString("prepared query id [%1] not found").arg(qId);
        return;
    }
    // get query from hash
    QSqlQuery *query;
    query = m_queries.value(qId);
    // bind value
    query->bindValue(placeholder, val);
}

////

QueryThread::QueryThread(QObject *parent)
    : QThread(parent)
{
}

QueryThread::~QueryThread()
{
    delete m_worker;
}

void QueryThread::execute(const QString &qId, const QString &sql)
{
    emit fwdExecute(qId, sql); // forwards to the worker
}

void QueryThread::prepare(const QString &qId, const QString &sql)
{
    emit fwdPrepare(qId, sql); // forwards to the worker
}

void QueryThread::bindValue(const QString &qId, const QString &placeholder, const QVariant &val)
{
    emit fwdBindValue(qId, placeholder, val); // forwards to the worker
}

void QueryThread::run()
{
    emit ready(false);
    emit progress( "QueryThread starting, one moment please..." );

    // Create worker object within the context of the new thread
    m_worker = new Worker();

    connect( this, SIGNAL(fwdExecute(QString,QString)),
             m_worker, SLOT(slotExecute(QString,QString)) );

    connect( this, SIGNAL(fwdPrepare(QString,QString)),
             m_worker, SLOT(slotPrepare(QString,QString)) );

    connect( this, SIGNAL(fwdBindValue(QString,QString,QVariant)),
             m_worker, SLOT(slotBindValue(QString,QString,QVariant)) );

    // Critical: register new type so that this signal can be
    // dispatched across thread boundaries by Qt using the event
    // system
    qRegisterMetaType< QList<QSqlRecord> >( "QList<QSqlRecord>" );

    // forward final signals
    connect( m_worker, SIGNAL(results(QString,QList<QSqlRecord>)),
             this, SIGNAL(results(QString,QList<QSqlRecord>)) );

    connect( m_worker, SIGNAL( executed(QString)),
             this, SIGNAL(executed(QString)) );

    connect( m_worker, SIGNAL(executeFailed(QString,QSqlError)),
             this, SIGNAL(executeFailed(QString,QSqlError)) );

    connect( m_worker, SIGNAL(prepared(QString)),
             this, SIGNAL(prepared(QString)) );

    connect( m_worker, SIGNAL(prepareFailed(QString,QSqlError)),
             this, SIGNAL(prepareFailed(QString,QSqlError)) );

    emit progress( "Press 'Go' to run a query." );
    emit ready(true);

    exec();  // our event loop
}
