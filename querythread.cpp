#include "querythread.h"

#include "db.h"

#include <QDebug>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>


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

void Worker::slotExecute( const QString& queryId, const QString& sql)
{
    // if sql is not valid, treat this as already prepared statment
    // that needs to be executed
    bool isPrepared = (sql.isEmpty() || sql.isNull());

    if (isPrepared)
        executePrepared(queryId);
    else
        executeOneTime(queryId, sql);
}

void Worker::slotExecutePrepared(const QString &queryId, const QString &resultId)
{
    executePrepared(queryId, resultId);
}


void Worker::executeOneTime(const QString& queryId, const QString& sql)
{
    // use previously defined db connection
    QSqlQuery query(m_database);
    // execute query, get result
    bool ok = query.exec(sql);
    // check for errors
    if (!ok) {
        qDebug() << QString("execute failed for one time query id [%1]").arg(queryId);
        // emit error signal
        emit executeFailed(queryId, query.lastError(), QString());
        return;
    }
    // send executed signal
    emit executed(queryId, QString());
    // accumulate data and emit result
    QList<QSqlRecord> recs;
    while (query.next())
    {
        recs.append(query.record());
    }
    emit results(queryId, recs, QString());
}

void Worker::executePrepared(const QString& queryId, const QString &resultId)
{
    // constuct result_id, if not set
    QString newResultId;
    if (resultId.isEmpty() || resultId.isNull())
        newResultId = QString("queryresult_%1").arg(QDateTime::currentMSecsSinceEpoch());
    else
        newResultId = resultId;

    // check if there is query with specified id
    if (!m_queries.contains(queryId)) {
        qDebug() << QString("prepared query id [%1] not found").arg(queryId);
        return;
    }
    // get query from hash
    QSqlQuery *query;
    query = m_queries.value(queryId);
    // execute and check query status
    bool ok = query->exec();

    if (!ok) {
        qDebug() << QString("execute failed for prepared query id [%1]").arg(queryId);
        emit executeFailed(queryId, query->lastError(), newResultId);
        return;
    }
    // send executed signal
    emit executed(queryId, newResultId);

    QList<QSqlRecord> recs;
    while (query->next())
    {
        recs.append(query->record());
    }
    // result saved - release resources
    query->finish();
    // send results signal
    emit results(queryId, recs, newResultId);
}

void Worker::slotPrepare(const QString &queryId, const QString &sql)
{
    // check if there is query with specified id
    if (m_queries.contains(queryId)) {
        qDebug() << QString("already prepared query id [%1]").arg(queryId);
        return;
    }
    QSqlQuery *query;
    // use previously defined db connection
    query = new QSqlQuery(m_database);
    // prepare and check query status
    bool ok = query->prepare(sql);

    if (!ok) {
        qDebug() << QString("prepare failed for query id [%1]").arg(queryId);
        emit prepareFailed(queryId, query->lastError());
        return;
    }
    // save query to our list
    m_queries.insert(queryId, query);
    // send prepared signal
    emit prepared(queryId);
}

void Worker::slotBindValue(const QString &queryId, const QString &placeholder, const QVariant &val)
{
    // check if there is query with specified id
    if (!m_queries.contains(queryId)) {
        qDebug() << QString("prepared query id [%1] not found").arg(queryId);
        return;
    }
    // get query from hash
    QSqlQuery *query;
    query = m_queries.value(queryId);
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

void QueryThread::execute(const QString &queryId, const QString &sql)
{
    emit fwdExecute(queryId, sql); // forwards to the worker
}

void QueryThread::executePrepared(const QString &queryId, const QString &resultId)
{
    emit fwdExecutePrepared(queryId, resultId);
}

void QueryThread::prepare(const QString &queryId, const QString &sql)
{
    emit fwdPrepare(queryId, sql); // forwards to the worker
}

void QueryThread::bindValue(const QString &queryId, const QString &placeholder, const QVariant &val)
{
    emit fwdBindValue(queryId, placeholder, val); // forwards to the worker
}

void QueryThread::run()
{
    emit ready(false);
    emit progress( "QueryThread starting, one moment please..." );

    // Create worker object within the context of the new thread
    m_worker = new Worker();

    connect( this, SIGNAL(fwdExecute(QString,QString)),
             m_worker, SLOT(slotExecute(QString,QString)) );

    connect( this, SIGNAL(fwdExecutePrepared(QString,QString)),
             m_worker, SLOT(slotExecutePrepared(QString,QString)) );

    connect( this, SIGNAL(fwdPrepare(QString,QString)),
             m_worker, SLOT(slotPrepare(QString,QString)) );

    connect( this, SIGNAL(fwdBindValue(QString,QString,QVariant)),
             m_worker, SLOT(slotBindValue(QString,QString,QVariant)) );

    // Critical: register new type so that this signal can be
    // dispatched across thread boundaries by Qt using the event
    // system
    qRegisterMetaType< QList<QSqlRecord> >( "QList<QSqlRecord>" );

    // forward final signals
    connect( m_worker, SIGNAL(results(QString,QList<QSqlRecord>,QString)),
             this, SIGNAL(results(QString,QList<QSqlRecord>,QString)) );

    connect( m_worker, SIGNAL(executed(QString,QString)),
             this, SIGNAL(executed(QString,QString)) );

    connect( m_worker, SIGNAL(executeFailed(QString,QSqlError,QString)),
             this, SIGNAL(executeFailed(QString,QSqlError,QString)) );

    connect( m_worker, SIGNAL(prepared(QString)),
             this, SIGNAL(prepared(QString)) );

    connect( m_worker, SIGNAL(prepareFailed(QString,QSqlError)),
             this, SIGNAL(prepareFailed(QString,QSqlError)) );

    emit progress( "Press 'Go' to run a query." );
    emit ready(true);

    exec();  // our event loop
}
