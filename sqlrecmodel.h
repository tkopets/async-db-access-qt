#ifndef SQLRECMODEL_H
#define SQLRECMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QSqlRecord>

class SqlRecModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SqlRecModel(QObject *parent);
    void setRecordList(const QList<QSqlRecord> &records);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<QSqlRecord> m_recList;
};

#endif // SQLRECMODEL_H
