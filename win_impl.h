#ifndef WIN_IMPL_H
#define WIN_IMPL_H

#include <QWidget>
#include <QList>
#include <QSqlRecord>
#include "ui_win.h"
#include "sqlrecmodel.h"

class QueryThread;

class Win : public QWidget,
        protected Ui::win
{
    Q_OBJECT
public:
    Win( QWidget* parent = 0 );
    ~Win();

signals:
    void exec(const QString &);

private slots:
    void slotGo();
    void slotResults(const QString &qId, const QList<QSqlRecord> &records);

private:
    QueryThread* m_querythread;
    SqlRecModel *m_model;
    void dispatch(const QString &qId, const QString &query);
};


#endif
