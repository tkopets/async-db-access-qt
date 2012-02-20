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
    void slotExecute(const QString &qId, const QString &sql = QString());
    void slotPrepare(const QString &qId, const QString &sql);
    void slotBindValue(const QString &qId, const QString &placeholder, const QVariant &val);

  signals:
    void executed(const QString &qId);
    void executeFailed(const QString &qId, const QSqlError &err);
    void prepared(const QString &qId);
    void prepareFailed(const QString &qId, const QSqlError &err);
    void results(const QString &qId, const QList<QSqlRecord> &records);

   private:
     QSqlDatabase m_database;
     QHash<QString, QSqlQuery*> m_queries;
     void executeOneTime(const QString &qId, const QString &sql);
     void executePrepared(const QString &qId);
};


class QueryThread : public QThread
{
  Q_OBJECT

  public: 
    QueryThread(QObject *parent = 0);
    ~QueryThread();

    void execute(const QString &qId, const QString &sql = QString());
    void prepare(const QString &qId, const QString &sql);
    void bindValue(const QString &qId, const QString &placeholder, const QVariant &val);
 
  signals:
    void progress( const QString &msg);
    void ready(bool);
    void executed(const QString &qId);
    void executeFailed(const QString &qId, const QSqlError &err);
    void prepared(const QString &qId);
    void prepareFailed(const QString &qId, const QSqlError &err);
    void results(const QString &qId, const QList<QSqlRecord> &records);

  protected:
    void run();

  signals:
    void fwdExecute(const QString &qId, const QString &sql = QString());
    void fwdPrepare(const QString &qId, const QString &sql);
    void fwdBindValue(const QString &qId, const QString &placeholder, const QVariant &val);

  private:
     Worker* m_worker;
};

#endif
