#ifndef STANDARDITEMMODEL_H
#define STANDARDITEMMODEL_H

#include <QStandardItemModel>

class StandardItemModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit StandardItemModel(QObject *parent=NULL);
    ~StandardItemModel(){}

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
private slots:
    void slot_stateChanged(Qt::CheckState state);
signals:
    void stateChanged(Qt::CheckState state);
private:
    void onStateChanged();
};

#endif // STANDARDITEMMODEL_H
