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

#include <Serialization/TypeID.h>
#include <QObject>
#include <QScopedPointer>
#include <QWidget>
#include <QDockWidget>

#include "Serialization.h"
#include "Strings.h"
#include <vector>
#include <memory>

namespace CharacterTool
{
    using std::vector;
    using std::unique_ptr;
    struct System;

    struct IDockWidgetType
    {
        virtual ~IDockWidgetType() = default;
        virtual const char* Name() const = 0;
        virtual const char* Title() const = 0;
        virtual QWidget* Create(QDockWidget* dw, System* system, QMainWindow* mainWindow) = 0;
        virtual int DockArea() const = 0;
        virtual const char* TabifyDockWidget() const = 0;
        virtual Serialization::SStruct Serializer(QWidget* widget) const = 0;
    };

    template<class T>
    struct DockWidgetType
        : IDockWidgetType
    {
        string m_title;
        int m_dockArea;
        string m_dockWidget;

        DockWidgetType(const char* title, int dockArea, const char* tabifyDockWidget)
            : m_title(title)
            , m_dockArea(dockArea)
            , m_dockWidget(tabifyDockWidget)
        {}

        const char* Name() const override { return Serialization::TypeID::get<T>().name(); }
        const char* Title() const override { return m_title; }
        const char* TabifyDockWidget() const{ return m_dockWidget.c_str(); }
        QWidget* Create(QDockWidget* dw, System* system, QMainWindow* mainWindow) override
        {
            T* result = new T(0, system, mainWindow);
            result->SetDockWidget(dw);
            return result;
        }
        int DockArea() const override { return m_dockArea; }
        Serialization::SStruct Serializer(QWidget* widget) const override
        {
            return Serialization::SStruct(*static_cast<T*>(widget));
        }
    };

    // Controls dock-windows that can be splitted.
    class DockWidgetManager
        : public QObject
    {
        Q_OBJECT
    public:
        DockWidgetManager(QMainWindow* mainWindow, System* system);
        ~DockWidgetManager();

        template<class T>
        void AddDockWidgetType(const char* title, Qt::DockWidgetArea dockArea, const char* tabifyDockWidget)
        {
            DockWidgetType<T>* dockWidgetType = new DockWidgetType<T>(title, (int)dockArea, tabifyDockWidget);
            m_types.push_back(unique_ptr<IDockWidgetType>(dockWidgetType));
        }

        QWidget* CreateByTypeName(const char* typeName, QDockWidget* dockWidget) const;

        void RemoveOrHideDockWidget(QDockWidget* dockWidget);

        void ResetToDefault();
        void CreateDefaultWidgets();

        void AddDockWidgetsToMainWindow(bool tabifyWithExistingWidgets);
        void Clear();


        void Serialize(IArchive& ar);
    signals:
        void SignalChanged();
    protected slots:
        void OnDockSplit();
    private:
        void CreateDefaultWidget(IDockWidgetType* type);
        string MakeUniqueDockWidgetName(const char* type) const;
        IDockWidgetType* FindType(const char* name, int* typeIndex = 0);
        void SplitOpenWidget(int index);
        QDockWidget* CreateDockWidget(IDockWidgetType* type, QDockWidget* dockForSplit);
        Serialization::SStruct WidgetSerializer(QWidget* widget, const char* dockTypeName);

        struct OpenWidget
        {
            bool addedToMainWindow;
            string name;
            int dockArea;
            QWidget* widget;
            QDockWidget* dockWidget;
            string type;
            string createdType;

            OpenWidget()
                : widget()
                , dockWidget()
                , addedToMainWindow(false) {}
            void Serialize(IArchive& ar);
            void Destroy(QMainWindow* window);
        };

        QMainWindow* m_mainWindow;
        vector<OpenWidget> m_openWidgets;
        vector<unique_ptr<IDockWidgetType> > m_types;
        System* m_system;
    };
}
