#ifndef CRYINCLUDE_EDITORUI_QT_ATTRIBUTELISTVIEW_H
#define CRYINCLUDE_EDITORUI_QT_ATTRIBUTELISTVIEW_H
#pragma once

#include "api.h"

#include <qdockwidget.h>

enum InsertPosition
{
    INSERT_ABOVE,
    INSERT_BELOW
};

class EDITOR_QT_UI_API CAttributeListView
    : public QDockWidget
{
    Q_OBJECT
public:
    CAttributeListView(bool isCustom = false, QString groupvisibility = "Both");
    CAttributeListView(QWidget* parent, bool isCustom = false, QString groupvisibility = "Both");

    // QDragDrop Related Events
    void dragMoveEvent(QDragMoveEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

    void setCustomPanel(bool isCustom);
    bool isCustomPanel();
    bool isShowInGroup();
    bool isHideInEmitter();
    QString getGroupVisibility();

private:
    void SetDragStyle(QWidget* widget, QString style);

    bool m_isCustom;
    QWidget* m_previousGroup;
    InsertPosition positionIndex;
    QString m_GroupVisibility;
};

#endif // CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEM_H
