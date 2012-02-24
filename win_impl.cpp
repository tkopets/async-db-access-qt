#include "win_impl.h"
#include "querythread.h"
#include "db.h"

#include <QDebug>
#include <QDateTime>

Win::Win( QWidget* parent )
    : QWidget(parent)
{
    m_querythread = new QueryThread();

    setupUi( this );

    // Buttons
    connect( goButton, SIGNAL( clicked() ),
             this, SLOT( slotGo() ) );
    connect( closeButton, SIGNAL( clicked() ),
             this, SLOT( close() ) );

    // Worker thread
    connect( m_querythread, SIGNAL(progress(QString)),
             textBrowser, SLOT(append(QString)) );
    connect( m_querythread, SIGNAL( ready(bool) ),
             goButton, SLOT( setEnabled(bool) ) );
    connect( m_querythread, SIGNAL(results(QString,QList<QSqlRecord>,QString)),
             this, SLOT(slotResults(QString,QList<QSqlRecord>,QString)) );

    // Launch worker thread
    m_querythread->start();
    
    m_model = new SqlRecModel(this);
    tableView->setModel(m_model);
}

Win::~Win()
{
    m_querythread->quit();
    m_querythread->wait();
    delete m_querythread;
    delete m_model;
}

void Win::slotGo()
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
    int rand = qrand() % (SAMPLE_RECORDS - 100);


    textBrowser->append("Running queries, please wait...");
    dispatch("1", "select avg(id) from item;");
    dispatch("2", "select name from item;");
    dispatch("3", "select min(id) from item;");
    dispatch("4", "select max(id) from item;");
    dispatch("5", "select distinct(id) from item;");
    m_querythread->prepare("6", "SELECT * FROM item WHERE id = :value");
    m_querythread->bindValue("6", ":value", 10);
    m_querythread->executePrepared("6");
    dispatch("model", QString("select id, name from item limit 100 offset %1").arg(rand) );
    textBrowser->append("Dispatched all queries.\n");
}

void Win::dispatch(const QString &queryId, const QString &query)
{
    textBrowser->append(queryId +  ":: Executing:" + query);
    m_querythread->execute(queryId, query);
}


void Win::slotResults(const QString &queryId, const QList<QSqlRecord> &records, const QString &resultId)
{
    textBrowser->append( QString("RESULTS: queryId: %1, resultId: %2, count: %3").arg(queryId).arg(resultId).arg(records.size()) );

    if (records.size() == 1)
        qDebug() << records.at(0);;

    m_model->setRecordList(records);
}

