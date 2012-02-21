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
    connect( m_querythread, SIGNAL(results(QString,QList<QSqlRecord>)),
             this, SLOT(slotResults(QString,QList<QSqlRecord>)) );

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
    m_querythread->execute("6");
    dispatch("model", QString("select id, name from item limit 100 offset %1").arg(rand) );
    textBrowser->append("Dispatched all queries.\n");
}

void Win::dispatch(const QString &qId, const QString &query)
{
    textBrowser->append(qId +  ":: Executing:" + query);
    m_querythread->execute(qId, query);
}


void Win::slotResults(const QString &qId, const QList<QSqlRecord> &records)
{
    textBrowser->append( QString(qId+":: Result count:%1").arg(records.size()) );

    if (records.size() > 10) {
        textBrowser->append("more than 10");
    }
    else {

        for (int i = 0; i < records.size(); ++i) {
            QSqlRecord rec = records.at(i);
            textBrowser->append(QString("%1: %2,%3").arg(qId, rec.value(0).toString(), rec.value(1).toString() ));
        }
    }

    m_model->setRecordList(records);
}

