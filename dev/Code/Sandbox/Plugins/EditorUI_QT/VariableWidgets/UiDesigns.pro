#-------------------------------------------------
#
# Project created by QtCreator 2015-05-14T13:40:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UiDesigns
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    QCollapsePanel.cpp \
    QCollapseWidget.cpp \
    QCurveWidget.cpp \
    QColorWidget.cpp \
    QFileSelectWidget.cpp \
    QHGradientWidget.cpp \
    QBitmapPreviewDialog.cpp

HEADERS  += mainwindow.h \
    QCollapsePanel.h \
    QCollapseWidget.h \
    QCurveWidget.h \
    QColorWidget.h \
    QFileSelectWidget.h \
    QHGradientWidget.h \
    QBitmapPreviewDialog.h

FORMS    += \
    mainwindow.ui \
    QCollapsePanel.ui \
    qcollapsewidget.ui \
    QCurveWidget.ui \
    QColorWidget.ui \
    QFileSelectWidget.ui \
    QHGradientWidget.ui \
    QBitmapPreviewDialog.ui
