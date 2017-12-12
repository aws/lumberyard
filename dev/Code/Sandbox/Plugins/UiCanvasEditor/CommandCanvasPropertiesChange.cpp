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
#include "stdafx.h"

#include "EditorCommon.h"

CommandCanvasPropertiesChange::CommandCanvasPropertiesChange(UndoStack* stack,
    AZStd::string& undoXml, AZStd::string& redoXml, EditorWindow* editorWindow)
    : QUndoCommand()
    , m_stack(stack)
    , m_isFirstExecution(true)
    , m_undoXml(undoXml)
    , m_redoXml(redoXml)
    , m_editorWindow(editorWindow)
{
    setText("properties change");
}

void CommandCanvasPropertiesChange::undo()
{
    UndoStackExecutionScope s(m_stack);

    Recreate(true);
}

void CommandCanvasPropertiesChange::redo()
{
    UndoStackExecutionScope s(m_stack);

    Recreate(false);
}

void CommandCanvasPropertiesChange::Recreate(bool isUndo)
{
    if (m_isFirstExecution)
    {
        m_isFirstExecution = false;

        // Nothing else to do.
    }
    else
    {
        // We are going to load a saved canvas from XML and replace the existing canvas
        // with it. 
        // Create a new entity context for the new canvas
        UiEditorEntityContext* newEntityContext = new UiEditorEntityContext(m_editorWindow);

        // Create a new canvas from the XML and release the old canvas, use the new entity context for
        // the new canvas
        const AZStd::string& xml = isUndo ? m_undoXml : m_redoXml;
        gEnv->pLyShine->ReloadCanvasFromXml(xml, newEntityContext);

        // Tell the editor window to use the new entity context
        m_editorWindow->ReplaceEntityContext(newEntityContext);

        // Tell the UI animation system that the active canvas has changed
        EBUS_EVENT(UiEditorAnimationBus, CanvasLoaded);

        // Clear any selected elements from the hierarchy widget. If an element is selected,
        // this will trigger the properties pane to refresh with the new canvas, but the
        // refresh is on a timer so it won't happen right away.
        HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
        if (hierarchyWidget)
        {
            hierarchyWidget->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);
        }

        // Tell the properties pane that the canvas pointer changed
        PropertiesWidget* propertiesWidget = m_editorWindow->GetProperties();
        if (propertiesWidget)
        {
            propertiesWidget->SelectedEntityPointersChanged();
        }
    }
}

void CommandCanvasPropertiesChange::Push(UndoStack* stack, AZStd::string& undoXml,
    AZStd::string& redoXml, EditorWindow* editorWindow)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandCanvasPropertiesChange(stack, undoXml, redoXml, editorWindow));
}
