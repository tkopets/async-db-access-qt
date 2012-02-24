#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QList>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QHash>
#include <QSqlQuery>

// The class that does all the work with the database. This class will
// be instantiated in the thread object's run() method.
class Worker : public QObject
{
    Q_OBJECT

public:
    Worker( QObject* parent = 0);
    ~Worker();

public slots:
    void slotExecute(const QString &queryId, const QString &sql);
    void slotExecutePrepared(const QString &queryId, const QString &resultId = QString());
    void slotPrepare(const QString &queryId, const QString &sql);
    void slotBindValue(const QString &queryId, const QString &placeholder, const QVariant &val);

signals:
    void executed(const QString &queryId, const QString &resultId);
    void executeFailed(const QString &queryId, const QSqlError &err, const QString &resultId);
    void prepared(const QString &queryId);
    void prepareFailed(const QString &queryId, const QSqlError &err);
    void results(const QString &queryId, const QList<QSqlRecord> &records, const QString &resultId);

private:
    QSqlDatabase m_database;
    QHash<QString, QSqlQuery*> m_queries;
    void executeOneTime(const QString &queryId, const QString &sql);
    void executePrepared(const QString &queryId, const QString &resultId = QString());
};


class QueryThread : public QThread
{
    Q_OBJECT

public:
    QueryThread(QObject *parent = 0);
    ~QueryThread();

    void execute(const QString &queryId, const QString &sql);
    void executePrepared(const QString &queryId, const QString &resultId = QString());
    void prepare(const QString &queryId, const QString &sql);
    void bindValue(const QString &queryId, const QString &placeholder, const QVariant &val);

signals:
    void progress( const QString &msg);
    void ready(bool);
    void executed(const QString &queryId, const QString &resultId);
    void executeFailed(const QString &queryId, const QSqlError &err, const QString &resultId);
    void prepared(const QString &queryId);
    void prepareFailed(const QString &queryId, const QSqlError &err);
    void results(const QString &queryId, const QList<QSqlRecord> &records, const QString &resultId);

protected:
    void run();

signals:
    void fwdExecute(const QString &queryId, const QString &sql);
    void fwdExecutePrepared(const QString &queryId, const QString &resultId = QString());
    void fwdPrepare(const QString &queryId, const QString &sql);
    void fwdBindValue(const QString &queryId, const QString &placeholder, const QVariant &val);

private:
    Worker *m_worker;
};

#endif
