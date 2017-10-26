#include "ccalendarwidget.h"
#include "ui_ccalendarwidget.h"
#include <QTextCharFormat>
#include <QSlider>

CCalendarWidget::CCalendarWidget(QWidget *parent) :
    QWidget(parent,Qt::Popup),
    ui(new Ui::CCalendarWidget)
{
    ui->setupUi(this);

    //当前日期
    QTextCharFormat fmt_today = ui->calendarWidget->dateTextFormat(QDate::currentDate());
    //        hightlighted_today.setBackground({"#b3b3b3"});
    fmt_today.setForeground({"#CF861D"});
    fmt_today.setFontWeight(QFont::DemiBold);
    ui->calendarWidget->setDateTextFormat(QDate::currentDate(), fmt_today);

    //v 周数
    ui->calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    //h 表头
    auto fmt_header = ui->calendarWidget->headerTextFormat();
    //和默认背景颜色一样
    fmt_header.setBackground(QBrush("#f0f0f0"));
    // fmt_header.setFontWeight(QFont::DemiBold);
    ui->calendarWidget->setHeaderTextFormat(fmt_header);

    ui->calendarWidget->setWeekdayTextFormat(Qt::Saturday,QTextCharFormat());
    ui->calendarWidget->setWeekdayTextFormat(Qt::Sunday,QTextCharFormat());

    connect(ui->horizontalSliderHour,&QSlider::valueChanged,[=](int val){ui->labelHourVal->setText(QString::number(val));});
    connect(ui->horizontalSliderMinute,&QSlider::valueChanged,[=](int val){ui->labelMinuteVal->setText(QString::number(val));});
    connect(ui->horizontalSliderSecond,&QSlider::valueChanged,[=](int val){ui->labelSecondVal->setText(QString::number(val));});
}

CCalendarWidget::~CCalendarWidget()
{
    delete ui;
}

void CCalendarWidget::setMaximumDate(const QDate &d)
{
    ui->calendarWidget->setMaximumDate(d);
}

void CCalendarWidget::setMinimumDate(const QDate &d)
{
    ui->calendarWidget->setMinimumDate(d);
}

void CCalendarWidget::setDateTime(const QDateTime &dt)
{
    ui->calendarWidget->setSelectedDate(dt.date());
    ui->horizontalSliderHour->setValue(dt.time().hour());
    ui->horizontalSliderMinute->setValue(dt.time().minute());
    ui->horizontalSliderSecond->setValue(dt.time().second());
}

void CCalendarWidget::on_pushButtonSubmit_clicked()
{
    QDateTime t;
    t.setDate(ui->calendarWidget->selectedDate());
    t.setTime(QTime(ui->horizontalSliderHour->value(),ui->horizontalSliderMinute->value(),ui->horizontalSliderSecond->value()));
    emit dateTimeSelect(t);
    this->close();
}

void CCalendarWidget::mousePressEvent(QMouseEvent *e)
{
    if(!this->rect().contains(e->pos()))
    {
        setAttribute(Qt::WA_NoMouseReplay);
        this->close();
    }
}
