#include "QColorEyeDropper.h"

// Qt Libraries
#include <QEvent>
#include <QApplication>
#include <QCursor>
#include <QTimer>
#include <QDesktopWidget>
#include <QScreen>
#include <QImage>
#include <QStyle>
#include <QBoxLayout>
#include <QPainter>
#include <QBitmap>
#include <QMouseEvent>
#include <QKeyEvent>

#define QTUI_EYEDROPPER_TIMER 10
#define QTUI_EYEDROPPER_WIDTH 80
#define QTUI_CURSORMAP_SIZE 120
#define QTUI_CURSORMAP_OFFSET 20
#define QTUI_EYEDROPPER_HEIGHT 80

QColorEyeDropper::QColorEyeDropper(QWidget* parent)
    : QWidget(parent)
    , m_isMouseInException(false)
    , m_currentExceptionWidget(nullptr)
{
    setFixedSize(QTUI_CURSORMAP_SIZE, QTUI_CURSORMAP_SIZE);
    setMouseTracking(true);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_UnderMouse);
    //setStyleSheet("background: tranparent;background-color: transparent;");
    //setMask(QPixmap().createHeuristicMask());
    layout = new QVBoxLayout(this);
    setLayout(layout);

    m_colorDescriptor = new QLabel(this);
    m_colorDescriptor->setFixedSize(QTUI_CURSORMAP_SIZE, QTUI_CURSORMAP_OFFSET);
    m_colorDescriptor->setAlignment(Qt::AlignCenter);
    m_colorDescriptor->setObjectName("EyeDropperTextInfo");
    m_colorDescriptor->setText("R:123 G:234 B:12");

    layout->addWidget(m_colorDescriptor);
    layout->setAlignment(Qt::AlignCenter);
    m_colorDescriptor->hide();



    // Set default color
    m_centerColor.setRgb(255, 255, 255, 255);
    m_cursorPos = QCursor::pos();
    m_sample = QPixmap(5, 5);
    m_mouseMask = QPixmap(QTUI_EYEDROPPER_WIDTH, QTUI_EYEDROPPER_HEIGHT);
    m_mouseMask.load(":/particleQT/icons/eyedropper_mask.png");
    //Reset cursor to eyedropper px
    m_borderMap = QPixmap(QTUI_EYEDROPPER_WIDTH, QTUI_EYEDROPPER_HEIGHT);
    m_borderMap.load(":/particleQT/icons/eyedropper.png");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]()
        {
            UpdateColor();
            timer->start(QTUI_EYEDROPPER_TIMER);
        });
}


QColorEyeDropper::~QColorEyeDropper()
{
    for (QWidget* w : m_exceptionWidgets)
    {
        UnregisterExceptionWidget(w);
    }
    disconnect(this, 0);
}

void QColorEyeDropper::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        UpdateColor();
        emit SignalEyeDropperColorPicked(m_centerColor);
    }
    else if (event->button() == Qt::RightButton)
    {
        EndEyeDropperMode();
    }
    return;
}

void QColorEyeDropper::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        EndEyeDropperMode();
    }
    return;
}

bool QColorEyeDropper::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Leave)
    {
        if (obj == m_currentExceptionWidget && m_exceptionWidgets.contains((QWidget*)obj))
        {
            if (!m_EyeDropperActive)
            {
                StartEyeDropperMode();
                m_isMouseInException = false;
                m_currentExceptionWidget = nullptr;
                return true;
            }
        }
    }
    if (event->type() == QEvent::Destroy)
    {
        if (m_exceptionWidgets.contains((QWidget*)obj))
        {
            UnregisterExceptionWidget((QWidget*)obj);
        }
    }
    return false;
}


void QColorEyeDropper::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isMouseInException)
    {
        return;
    }
    // We track the mouse enter the exceptionWidget here instead of the enterEvent of the exceptionWidgets becasuse
    // the enter event will not be caught in eyedropper mode since the eyedropper will grab the mouse and keyboard. 
    // On the call of "EndEyeDropperMode", the eyedropper will release the mouse and keyboard, that's why the exception 
    // widget could catch the Leave event and handle it.
    for (QWidget* w : m_exceptionWidgets)
    {
        if (w->rect().contains(w->mapFromGlobal(QCursor::pos())))
        {
            m_isMouseInException = true;
            m_currentExceptionWidget = w;
            break;
        }
    }
    if (m_isMouseInException)
    {
        if (m_EyeDropperActive)
        {
            EndEyeDropperMode();
        }
    }
    else
    {
        UpdateColor();
    }
}

// Please make sure EndEyeDropperMode is called whenever exit the parent dialog (if any)
void QColorEyeDropper::EndEyeDropperMode()
{
    releaseKeyboard();
    releaseMouse();
    QApplication::restoreOverrideCursor();
    m_EyeDropperActive = false;
    hide();
    timer->stop();
    emit SignalEndEyeDropper();
}


void QColorEyeDropper::StartEyeDropperMode()
{
    grabKeyboard();
    grabMouse();
    setMouseTracking(true);
    QApplication::setOverrideCursor(Qt::BlankCursor);
    m_EyeDropperActive = true;
    timer->start(QTUI_EYEDROPPER_TIMER);
    // Reset ExceptionWidget status
    m_isMouseInException = false;
    m_currentExceptionWidget = nullptr;
    show();
}

bool QColorEyeDropper::EyeDropperIsActive()
{
    return m_EyeDropperActive;
}

void QColorEyeDropper::RegisterExceptionWidget(QVector<QWidget*> widgets)
{
    for (QWidget* w : widgets)
    {
        RegisterExceptionWidget(w);
    }
}

void QColorEyeDropper::UnregisterExceptionWidget(QVector<QWidget*> widgets)
{
    for (QWidget* w : widgets)
    {
        UnregisterExceptionWidget(w);
    }
}

void QColorEyeDropper::RegisterExceptionWidget(QWidget* widget)
{
    if (!m_exceptionWidgets.contains(widget))
    {
        m_exceptionWidgets.push_back(widget);
        widget->installEventFilter(this);
    }
}
void QColorEyeDropper::UnregisterExceptionWidget(QWidget* widget)
{
    if (m_exceptionWidgets.contains(widget))
    {
        m_exceptionWidgets.removeOne(widget);
        widget->removeEventFilter(this);
    }
}


void QColorEyeDropper::UpdateColor()
{
    QPoint position = QCursor::pos();
    QColor color = PaintWidget(position);

    m_centerColor.setRgb(color.red(), color.green(), color.blue(), color.alpha());

    QString R, G, B, A;
    R.setNum(color.red());
    G.setNum(color.green());
    B.setNum(color.blue());
    A.setNum(color.alpha());

    QString colortext = "R:" + R + " G:" + G + " B:" + B;
    m_colorDescriptor->setText(colortext);
    move(QPoint(position.x() - QTUI_EYEDROPPER_HEIGHT / 2, position.y() + QTUI_EYEDROPPER_HEIGHT));
    m_cursorPos = QCursor::pos();
}

QColor QColorEyeDropper::GrabScreenColor(const QPoint& pos)
{
    QPoint startPos(pos.x(), pos.y());
    const QDesktopWidget* desk = QApplication::desktop();
    m_sample = QGuiApplication::screens().at(desk->screenNumber())->grabWindow(desk->winId(), startPos.x(), startPos.y(), 5, 5);
    QImage image = m_sample.toImage();

    return image.pixel(2, 2); //center pixel on the preview
}

// Mouse Position is the Left-top corner of the whole widget
QColor QColorEyeDropper::PaintWidget(const QPoint& mousePosition)
{
    QPixmap eyeDropperPixmap = m_sample.scaled(QTUI_EYEDROPPER_WIDTH, QTUI_EYEDROPPER_HEIGHT);
    eyeDropperPixmap.setMask(m_mouseMask.createMaskFromColor(Qt::red, Qt::MaskOutColor));


    QPixmap cursorpixmap(QTUI_CURSORMAP_SIZE, QTUI_CURSORMAP_SIZE);
    cursorpixmap.fill(Qt::transparent);
    QPixmap labelPixmap = m_colorDescriptor->grab(QRect(QPoint(0, 0), QSize(QTUI_CURSORMAP_SIZE, 20)));
    QPainter cursorPainter(&cursorpixmap);
    cursorPainter.drawPixmap(20, 0, eyeDropperPixmap.scaled(QTUI_EYEDROPPER_WIDTH, QTUI_EYEDROPPER_HEIGHT));
    cursorPainter.drawPixmap(20, 0, m_borderMap.scaled(QTUI_EYEDROPPER_WIDTH, QTUI_EYEDROPPER_HEIGHT));
    cursorPainter.drawPixmap(0, 95, labelPixmap);

    //Offset the cursor position to the center of the color viewer.
    QApplication::changeOverrideCursor(QCursor(cursorpixmap.scaled(QTUI_CURSORMAP_SIZE, QTUI_CURSORMAP_SIZE), QTUI_CURSORMAP_SIZE/2, QTUI_EYEDROPPER_HEIGHT / 2));

    return GrabScreenColor(mousePosition);
}


#include <VariableWidgets/QColorEyeDropper.moc>