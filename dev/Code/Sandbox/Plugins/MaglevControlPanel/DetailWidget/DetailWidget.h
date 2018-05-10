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

#pragma once

#include <QFrame>
#include <QList>
#include <QMetaObject>
#include <QMenu>

class ResourceManagementView;

class DetailWidget
    : public QFrame
{
    Q_OBJECT

public:

    DetailWidget(ResourceManagementView* view, QWidget* parent);

    virtual ~DetailWidget();

    virtual void show();

    virtual void hide();

    virtual QMenu* GetTreeContextMenu();

protected:

    ResourceManagementView* m_view;

    // Connect only until the widget is hidden. Use this connect in the
    // show method override to temporarily connect to the main view's
    // signals.
    template <typename Func1, typename Func2>
    QMetaObject::Connection connectUntilHidden(const typename QtPrivate::FunctionPointer<Func1>::Object* sender, Func1 signal,
        const typename QtPrivate::FunctionPointer<Func2>::Object* receiver, Func2 slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        auto connection = QFrame::connect(sender, signal, receiver, slot, type);
        m_connections.append(connection);
        return connection;
    }

    template <typename Func1, typename Func2>
    QMetaObject::Connection connectUntilHidden(const typename QtPrivate::FunctionPointer<Func1>::Object* sender, Func1 signal,
        const QObject* context, Func2 slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        auto connection = QFrame::connect(sender, signal, context, slot, type);
        m_connections.append(connection);
        return connection;
    }

    // A normal QT connect. Be sure not to do anything on these signals that
    // could cause the state of the main view to be modified.
    template <typename Func1, typename Func2>
    QMetaObject::Connection connectUntilDeleted(const typename QtPrivate::FunctionPointer<Func1>::Object* sender, Func1 signal,
        const typename QtPrivate::FunctionPointer<Func2>::Object* receiver, Func2 slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return QFrame::connect(sender, signal, receiver, slot, type);
    }

    template <typename Func1, typename Func2>
    QMetaObject::Connection connectUntilDeleted(const typename QtPrivate::FunctionPointer<Func1>::Object* sender, Func1 signal,
        const QObject* context, Func2 slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return QFrame::connect(sender, signal, context, slot, type);
    }

private:

    QList<QMetaObject::Connection> m_connections;

    // Use connectUntilHidden or connectUntilDeleted
    template <typename Func1, typename Func2>
    QMetaObject::Connection connect(const typename QtPrivate::FunctionPointer<Func1>::Object* sender, Func1 signal,
        const typename QtPrivate::FunctionPointer<Func2>::Object* receiver, Func2 slot,
        Qt::ConnectionType type = Qt::AutoConnection);

    template <typename Func1, typename Func2>
    QMetaObject::Connection connect(const typename QtPrivate::FunctionPointer<Func1>::Object* sender, Func1 signal,
        const QObject* context, Func2 slot,
        Qt::ConnectionType type = Qt::AutoConnection);
};
