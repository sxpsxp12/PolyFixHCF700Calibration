#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QEvent>
#include "sensorglobal.h"
#include <QPushButton>
#include <QScopedPointer>

class ItemDelegate:public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ItemDelegate(TableType type=TABLE_NONE,QObject *parent = NULL);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
Q_SIGNALS:
    void signal_deleteData(const QModelIndex index);
private:
    enum{
        CHECK_BOX_COLUMN = 0,
        DATA_OPERATE_COLUMN = 7
    };
    TableType m_table_type;
    QScopedPointer<QPushButton> m_deletePbt;

    QPoint m_mousePoint;    //鼠标位置
    QStringList m_operate_list; //操作列表
    int m_pbt_status;   //按键状态 1：按下；0：悬浮
    int m_nspace;   //按键之间的间距
    int m_height;   //按键高
    int m_width;    //按键宽

};

#endif // ITEMDELEGATE_H
