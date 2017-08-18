/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <QString>
#include <CryString.h>
#include "UnicodeFunctions.h"

#include <QApplication>
#include <QDropEvent>
#include <QWidget>

#ifdef LoadCursor
#undef LoadCursor
#endif

class QWaitCursor
{
public:
    QWaitCursor()
    {
        QGuiApplication::setOverrideCursor(Qt::BusyCursor);
    }
    ~QWaitCursor()
    {
        QGuiApplication::restoreOverrideCursor();
    }
};

class DropTarget : public QObject
{
public:
    DropTarget(QWidget* widget)
        : m_widget(widget)
    {
        widget->installEventFilter(this);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_widget)
        {
            switch (event->type())
            {
            case QEvent::DragEnter:
            {
                QDragEnterEvent* e = static_cast<QDragEnterEvent*>(event);
                e->setDropAction(OnDragEnter(m_widget, e->mimeData(), e->keyboardModifiers(), e->pos()));
                e->setAccepted(e->dropAction() != Qt::IgnoreAction);
                break;
            }
            case QEvent::DragMove:
            {
                QDragMoveEvent* e = static_cast<QDragMoveEvent*>(event);
                e->setDropAction(OnDragOver(m_widget, e->mimeData(), e->keyboardModifiers(), e->pos()));
                e->setAccepted(e->dropAction() != Qt::IgnoreAction);
                break;
            }
            case QEvent::Drop:
            {
                QDropEvent* e = static_cast<QDropEvent*>(event);
                e->setAccepted(OnDrop(m_widget, e->mimeData(), e->dropAction(), e->pos()));
                break;
            }
            default:
                break;
            }
        }
        return QObject::eventFilter(watched, event);
    }

    virtual Qt::DropAction OnDragEnter(QWidget* pWnd, const QMimeData* pDataObject, Qt::KeyboardModifiers dwKeyState, const QPoint& point) = 0;
    virtual Qt::DropAction OnDragOver(QWidget* pWnd, const QMimeData* pDataObject, Qt::KeyboardModifiers dwKeyState, const QPoint& point) = 0;
    virtual bool OnDrop(QWidget* pWnd, const QMimeData* pDataObject, Qt::DropAction dropEffect, const QPoint& point) = 0;
    virtual void OnDragLeave(QWidget* pWnd) {}

private:
    QWidget* m_widget;
};

namespace QtUtil
{
    // From QString to CryString
    inline CryStringT<char> ToString(const QString& str)
    {
        return Unicode::Convert<CryStringT<char> >(str);
    }

    // From CryString to QString
    inline QString ToQString(const CryStringT<char>& str)
    {
        return Unicode::Convert<QString>(str);
    }

    // From const char * to QString
    inline QString ToQString(const char* str, size_t len = -1)
    {
        if (len == -1)
        {
            len = strlen(str);
        }
        return Unicode::Convert<QString>(str, str + len);
    }

    // Replacement for CString::trimRight()
    inline QString trimRight(const QString& str)
    {
        // We prepend a char, so that the left doesn't get trimmed, then we remove it after trimming
        return QString(QStringLiteral("A") + str).trimmed().remove(0, 1);
    }

    template<typename ... Args>
    struct Select
    {
        template<typename C, typename R>
        static auto OverloadOf(R(C::* pmf)(Args...))->decltype(pmf) {
            return pmf;
        }
    };
}

namespace stl
{
    //! Case insensitive less key for QString
    template <>
    struct less_stricmp<QString>
        : public std::binary_function<QString, QString, bool>
    {
        bool operator()(const QString& left, const QString& right) const
        {
            return left.compare(right, Qt::CaseInsensitive) < 0;
        }
    };
}
