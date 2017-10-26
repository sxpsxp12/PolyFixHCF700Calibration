#include "ItemDelegate.h"
#include <QCheckBox>
#include <QApplication>
#include <QMouseEvent>
#include <QDebug>
#include <QToolTip>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

ItemDelegate::ItemDelegate(TableType type, QObject *parent):
    QStyledItemDelegate(parent),m_table_type(type),m_deletePbt(new QPushButton)
{
    m_width = 48;
    m_height = 24;
    m_nspace = 5;
    m_operate_list << "删除";
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    if (option.state.testFlag(QStyle::State_HasFocus))
        viewOption.state = viewOption.state ^ QStyle::State_HasFocus;

    QStyledItemDelegate::paint(painter, viewOption, index);

    if (!index.parent().isValid() && index.column() == CHECK_BOX_COLUMN)
    {
        Qt::CheckState data = (Qt::CheckState)index.model()->data(index, Qt::UserRole).toInt();
        QRect rect = option.rect;

        QStyleOptionButton checkBoxStyle;
        checkBoxStyle.state = (data==Qt::Checked) ? QStyle::State_On : QStyle::State_Off;
        checkBoxStyle.state |= QStyle::State_Enabled;
        checkBoxStyle.iconSize = QSize(20, 20);
        checkBoxStyle.rect = QRect(QPoint(rect.left()+(rect.width()-20)/2,rect.top()+(rect.height()-20)/2),QPoint(rect.left()+(rect.width()-20)/2+20,rect.bottom()-(rect.height()-20)/2));

        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkBoxStyle, painter);
    }
    if(m_table_type == TABLE_NODEDATA)
    {
        if(index.column() == DATA_OPERATE_COLUMN)
        {
            // 计算按钮显示区域
            int nCount = m_operate_list.count();
            int nHalf = (option.rect.width() - m_width * nCount - m_nspace * (nCount - 1)) / 2; //左右留白
            int nTop = (option.rect.height() - m_height) / 2;
            for (int i = 0; i < nCount; ++i) { // 绘制按钮
              QStyleOptionButton button;
              button.rect = QRect(option.rect.left() + nHalf + m_width * i + m_nspace * i, option.rect.top() + nTop, m_width, m_height);
              button.state |= QStyle::State_Enabled;
              button.text = "删除";
              if (button.rect.contains(m_mousePoint))
              {
                  if (m_pbt_status == 0)
                  {
                      button.state |= QStyle::State_MouseOver;
                  } else if (m_pbt_status == 1)
                  {
                      button.state |= QStyle::State_Sunken;
                  }
              }
              QWidget *pWidget = (i == 0)?m_deletePbt.data():NULL;
              QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter, pWidget);
            }
        }
    }
}

bool ItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if(index.column() != CHECK_BOX_COLUMN && index.column() != DATA_OPERATE_COLUMN)
        return false;

    QRect decorationRect = option.rect;
    QMouseEvent *mouseEvent = (QMouseEvent*)(event);
    if (event->type() == QEvent::MouseButtonPress && decorationRect.contains(mouseEvent->pos()))
    {
        if (index.column() == CHECK_BOX_COLUMN)
        {
            Qt::CheckState data = (Qt::CheckState)model->data(index, Qt::UserRole).toInt();
            if(data == Qt::Checked)
                data = Qt::Unchecked;
            else
                data = Qt::Checked;
            model->setData(index, data, Qt::UserRole+1);
            return true;
        }
    }

    if(m_table_type == TABLE_NODEDATA)
    {
        m_mousePoint = mouseEvent->pos();
        int nCount = m_operate_list.count();
        int nHalf = (option.rect.width() - m_width * nCount - m_nspace * (nCount - 1)) / 2;
        int nTop = (option.rect.height() - m_height) / 2;

        // 还原鼠标样式
        QApplication::restoreOverrideCursor();

        for (int i = 0; i < nCount; ++i)
        {
            QStyleOptionButton button;
            button.rect = QRect(option.rect.left() + nHalf + m_width * i + m_nspace * i,
                                option.rect.top() + nTop,  m_width, m_height);

            // 鼠标位于按钮之上
            if (!button.rect.contains(m_mousePoint))
                continue;

            switch (event->type())
            {
            case QEvent::MouseMove:
            {
                // 设置鼠标样式为手型
                QApplication::setOverrideCursor(Qt::PointingHandCursor);

                m_pbt_status = 0;
                QToolTip::showText(mouseEvent->globalPos(), m_operate_list.at(i));
                break;
            }
            case QEvent::MouseButtonPress:
            {
                m_pbt_status = 1;
                break;
            }
            case QEvent::MouseButtonRelease:
            {
                if (i == 0)
                {
                    //删除
                    emit signal_deleteData(index);
                }
                break;
            }
            default:
                break;
            }
        }
        return true;
    }

    return QAbstractItemDelegate::editorEvent(event,model,option,index);
}
