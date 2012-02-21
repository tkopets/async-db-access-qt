#include "sqlrecmodel.h"

SqlRecModel::SqlRecModel(QObject *parent)
    :QAbstractTableModel(parent)
{
}

void SqlRecModel::setRecordList(const QList<QSqlRecord> &records)
{
    emit layoutAboutToBeChanged();
    m_recList = records;
    emit layoutChanged();
}

int SqlRecModel::rowCount(const QModelIndex &parent) const
{
    return m_recList.size();
}

int SqlRecModel::columnCount(const QModelIndex &parent) const
{
    if (!m_recList.isEmpty())
        return m_recList.first().count();
    else
        return 0;
}

QVariant SqlRecModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        return m_recList.at(index.row()).value(index.column());
    }
    return QVariant();
}

QVariant SqlRecModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            if (!m_recList.isEmpty()) {
                return m_recList.first().fieldName(section);
            }
        }
    }
    return QVariant();
}
