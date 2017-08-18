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

#include "pch.h"
#include <QMainWindow>
#include "DockWidgetManager.h"
#include "../../EditorCommon/DockTitleBarWidget.h"
#include "Expected.h"

namespace CharacterTool
{
    void DockWidgetManager::OpenWidget::Serialize(IArchive& ar)
    {
        ar(name, "name");
        string oldType = type;
        ar(type, "type");
        ar(dockArea, "dockArea");

        if (DockWidgetManager* manager = ar.FindContext<DockWidgetManager>())
        {
            IDockWidgetType* type = manager->FindType(this->type.c_str());
            if (ar.IsInput())
            {
                if (!dockWidget)
                {
                    dockWidget = manager->CreateDockWidget(type, 0);
                    addedToMainWindow = false;
                }
                dockWidget->setWindowTitle(type->Title());
                dockWidget->setObjectName(name.c_str());

                if (createdType != this->type)
                {
                    if (widget)
                    {
                        widget->setParent(0);
                        widget->deleteLater();
                    }
                    widget = manager->CreateByTypeName(this->type.c_str(), dockWidget);
                    dockWidget->setWidget(widget);
                    createdType = this->type;
                }
            }

            Serialization::SStruct serializer = manager->WidgetSerializer(widget, createdType.c_str());
            if (serializer)
            {
                ar(serializer, "widget");
            }
        }
    }

    void DockWidgetManager::OpenWidget::Destroy(QMainWindow* mainWindow)
    {
        if (dockWidget)
        {
            if (addedToMainWindow)
            {
                mainWindow->removeDockWidget(dockWidget);
            }
            dockWidget->deleteLater();
            dockWidget = 0;
        }
        if (widget)
        {
            widget->setParent(0);
            widget->deleteLater();
            widget = 0;
        }
    }

    // ---------------------------------------------------------------------------

    class CustomDockWidget
        : public QDockWidget
    {
    public:
        CustomDockWidget(DockWidgetManager* manager, const char* title)
            : QDockWidget(title)
            , m_manager(manager)
        {
        }

        void closeEvent(QCloseEvent* ev) override
        {
            m_manager->RemoveOrHideDockWidget(this);
        }
    private:
        DockWidgetManager* m_manager;
    };

    // ---------------------------------------------------------------------------

    DockWidgetManager::DockWidgetManager(QMainWindow* mainWindow, System* system)
        : m_mainWindow(mainWindow)
        , m_system(system)
    {
    }

    DockWidgetManager::~DockWidgetManager()
    {
        Clear();
    }

    QDockWidget* DockWidgetManager::CreateDockWidget(IDockWidgetType* type, QDockWidget* dockForSplit)
    {
        QDockWidget* dock = new CustomDockWidget(this, type->Title());

        CDockTitleBarWidget* titleBar = new CDockTitleBarWidget(dock);
        dock->setTitleBarWidget(titleBar);
        titleBar->AddCustomButton(QIcon("Editor/Icons/split.png"), "Split Pane", 0);
        EXPECTED(QObject::connect(titleBar, SIGNAL(SignalCustomButtonPressed(int)), this, SLOT(OnDockSplit())));
        return dock;
    }

    template<class TOpenWidget>
    static int CountOpenWidgetsOfType(const vector<TOpenWidget>& widgets, const char* type)
    {
        int result = 0;
        for (size_t i = 0; i < widgets.size(); ++i)
        {
            if (widgets[i].type == type)
            {
                ++result;
            }
        }
        return result;
    }

    void DockWidgetManager::RemoveOrHideDockWidget(QDockWidget* dockWidget)
    {
        for (size_t i = 0; i < m_openWidgets.size(); ++i)
        {
            OpenWidget& w = m_openWidgets[i];
            if (w.dockWidget == dockWidget)
            {
                int count = CountOpenWidgetsOfType(m_openWidgets, w.type.c_str());
                if (count > 1)
                {
                    w.dockWidget->setParent(0);
                    w.dockWidget->deleteLater();
                    w.widget->setParent(0);
                    w.widget->deleteLater();
                    m_openWidgets.erase(m_openWidgets.begin() + i);
                    SignalChanged();
                    return;
                }
                else
                {
                    w.dockWidget->hide();
                }
            }
        }
    }

    QWidget* DockWidgetManager::CreateByTypeName(const char* typeName, QDockWidget* dockWidget) const
    {
        for (size_t i = 0; i < m_types.size(); ++i)
        {
            IDockWidgetType* type = m_types[i].get();
            if (strcmp(type->Name(), typeName) == 0)
            {
                return type->Create(dockWidget, m_system, m_mainWindow);
            }
        }
        return 0;
    }

    void DockWidgetManager::Serialize(IArchive& ar)
    {
        Serialization::SContext<DockWidgetManager> x(ar, this);
        vector<OpenWidget> unusedWidgets;
        if (ar.IsInput())
        {
            unusedWidgets = m_openWidgets;
        }
        ar(m_openWidgets, "openWidgets");

        vector<int> widgetTypeCount(m_types.size());

        if (ar.IsInput())
        {
            for (size_t i = 0; i < m_openWidgets.size(); ++i)
            {
                OpenWidget& w = m_openWidgets[i];
                int typeIndex = 0;
                IDockWidgetType* type = FindType(w.type.c_str(), &typeIndex);
                if (!type)
                {
                    m_openWidgets.erase(m_openWidgets.begin() + i);
                    --i;
                    continue;
                }

                if (size_t(typeIndex) < widgetTypeCount.size())
                {
                    widgetTypeCount[typeIndex] += 1;
                }

                for (size_t j = 0; j < unusedWidgets.size(); ++j)
                {
                    if (unusedWidgets[j].dockWidget == w.dockWidget)
                    {
                        unusedWidgets.erase(unusedWidgets.begin() + j);
                        --j;
                    }
                }
            }

            for (size_t i = 0; i < unusedWidgets.size(); ++i)
            {
                unusedWidgets[i].Destroy(m_mainWindow);
            }

            for (size_t i = 0; i < widgetTypeCount.size(); ++i)
            {
                if (widgetTypeCount[i] == 0)
                {
                    CreateDefaultWidget(m_types[i].get());
                }
            }
        }
    }

    IDockWidgetType* DockWidgetManager::FindType(const char* name, int* typeIndex)
    {
        for (size_t i = 0; i < m_types.size(); ++i)
        {
            IDockWidgetType* type = m_types[i].get();
            if (strcmp(type->Name(), name) == 0)
            {
                if (typeIndex)
                {
                    *typeIndex = int(i);
                }
                return m_types[i].get();
            }
        }
        return 0;
    }

    void DockWidgetManager::OnDockSplit()
    {
        CDockTitleBarWidget* titleBar = qobject_cast<CDockTitleBarWidget*>(sender());
        if (!titleBar)
        {
            return;
        }

        QWidget* dock = titleBar->parentWidget();

        for (size_t i = 0; i < m_openWidgets.size(); ++i)
        {
            if (m_openWidgets[i].dockWidget == dock)
            {
                SplitOpenWidget((int)i);
                break;
            }
        }
    }

    void DockWidgetManager::ResetToDefault()
    {
        Clear();

        for (size_t i = 0; i < m_types.size(); ++i)
        {
            IDockWidgetType* type = m_types[i].get();
            CreateDefaultWidget(type);
        }
    }

    void DockWidgetManager::CreateDefaultWidget(IDockWidgetType* type)
    {
        OpenWidget w;
        w.dockWidget = CreateDockWidget(type, 0);
        w.name = MakeUniqueDockWidgetName(type->Name());
        w.dockWidget->setObjectName(w.name.c_str());
        w.dockArea = type->DockArea();
        w.widget = type->Create(w.dockWidget, m_system, m_mainWindow);
        w.dockWidget->setWidget(w.widget);
        w.type = type->Name();
        w.createdType = type->Name();
        m_openWidgets.push_back(w);
    }

    string DockWidgetManager::MakeUniqueDockWidgetName(const char* type) const
    {
        string name;
        for (int index = 0; true; ++index)
        {
            name = QString("%1-%2").arg(type).arg(index).toLocal8Bit().data();
            for (size_t i = 0; i < m_openWidgets.size(); ++i)
            {
                if (m_openWidgets[i].name == name)
                {
                    name.clear();
                    break;
                }
            }
            if (!name.empty())
            {
                break;
            }
        }
        return name;
    }

    void DockWidgetManager::Clear()
    {
        for (size_t i = 0; i < m_openWidgets.size(); ++i)
        {
            OpenWidget& w = m_openWidgets[i];
            w.Destroy(m_mainWindow);
        }
        m_openWidgets.clear();
    }

    void DockWidgetManager::SplitOpenWidget(int openWidgetIndex)
    {
        OpenWidget& existingWidget = m_openWidgets[openWidgetIndex];

        IDockWidgetType* type = FindType(existingWidget.type.c_str());
        if (!type)
        {
            return;
        }
        OpenWidget newWidget = existingWidget;
        newWidget.addedToMainWindow = false;
        newWidget.dockWidget = CreateDockWidget(type, 0);
        newWidget.widget = type->Create(newWidget.dockWidget, m_system, m_mainWindow);
        newWidget.name = MakeUniqueDockWidgetName(type->Name());
        newWidget.dockWidget->setObjectName(newWidget.name.c_str());
        newWidget.dockWidget->setWidget(newWidget.widget);
        newWidget.type = type->Name();
        newWidget.createdType = type->Name();

        vector<char> buffer;
        SerializeToMemory(&buffer, type->Serializer(existingWidget.widget));
        SerializeFromMemory(type->Serializer(newWidget.widget), buffer);

        m_openWidgets.push_back(newWidget);

        AddDockWidgetsToMainWindow(false);

        SignalChanged();
    }

    void DockWidgetManager::AddDockWidgetsToMainWindow(bool tabifyWithExistingWidgets)
    {
        for (size_t i = 0; i < m_openWidgets.size(); ++i)
        {
            OpenWidget& w = m_openWidgets[i];
            if (!w.addedToMainWindow)
            {
                m_mainWindow->addDockWidget((Qt::DockWidgetArea)w.dockArea, w.dockWidget);
                if (tabifyWithExistingWidgets)
                {
                    if (IDockWidgetType* type = FindType(w.type.c_str()))
                    {
                        if (QDockWidget* tabifyWidget = m_mainWindow->findChild<QDockWidget*>(type->TabifyDockWidget()))
                        {
                            m_mainWindow->tabifyDockWidget(w.dockWidget, tabifyWidget);
                            w.dockWidget->raise();
                        }
                    }
                }
                w.addedToMainWindow = true;
            }
        }
    }


    Serialization::SStruct DockWidgetManager::WidgetSerializer(QWidget* widget, const char* dockTypeName)
    {
        IDockWidgetType* type = FindType(dockTypeName);
        if (!type)
        {
            return Serialization::SStruct();
        }
        return type->Serializer(widget);
    }

    // ---------------------------------------------------------------------------
}

#include <CharacterTool/DockWidgetManager.moc>
