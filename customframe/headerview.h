#ifndef HEADERVIEW_H
#define HEADERVIEW_H

#include <QHeaderView>

class HeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit HeaderView(Qt::Orientation orientation,bool is_indentation = false, QWidget *parent = 0);
    ~HeaderView(){}
protected:
    virtual void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
signals:
    void stateChanged(Qt::CheckState state);
private slots:
    void slot_stateChanged(Qt::CheckState state);
private:
    Qt::CheckState m_state;
    bool m_is_pressed;
    bool m_is_indentation;
};

#endif // HEADERVIEW_H
