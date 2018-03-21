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

#include "StdAfx.h"
#include "ObjectCreateTool.h"
#include "ObjectTypeBrowser.h"
#include "PanelTreeBrowser.h"
#include "Viewport.h"
#include "ViewManager.h"
#include "DisplaySettings.h"
#include "MainWindow.h"

#include "EditMode/ObjectMode.h"
#include "Objects/ObjectLayerManager.h"

#include "QtUtil.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <QMessageBox>

//////////////////////////////////////////////////////////////////////////
CObjectCreateTool::CObjectCreateTool(CreateCallback createCallback)
{
    static const GUID guid =    {
        0xfc539aa9, 0x9ca0, 0x4db7, { 0xbb, 0xc3, 0xb4, 0x44, 0xf5, 0xff, 0x8c, 0x6b }
    };
    m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(guid);

    SetStatusText("Drag&Drop item to create an object");

    m_hCreateCursor = CMFCUtils::LoadCursor(IDC_POINTER_OBJHIT);

    m_pMouseCreateCallback = 0;
    m_createCallback = createCallback;
    m_object = 0;
    m_objectBrowserPanelId = 0;
    m_fileBrowserPanelId = 0;
}

//////////////////////////////////////////////////////////////////////////
CObjectCreateTool::~CObjectCreateTool()
{
    if (m_pMouseCreateCallback)
    {
        m_pMouseCreateCallback->Release();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::CloseFileBrowser()
{
    if (m_fileBrowserPanelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_fileBrowserPanelId);
        m_fileBrowserPanelId = 0;
    }
}

bool CObjectCreateTool::Activate(CEditTool* pPreviousTool)
{
    CEditTool* prevTool = pPreviousTool;

    // hunt down an object mode tool:
    while (prevTool)
    {
        // if we have object mode tool as the previous tool, then we still want that tool to be used when the creation tool is on
        // so we add it as parent, so it will handle mouse actions if we, the create tool, don't handle them
        if (qobject_cast<CObjectMode*>(prevTool))
        {
            SetParentTool(prevTool);
            break;
        }
        else
        {
            prevTool = prevTool->GetParentTool();
        }
    }

    return true;
};

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::SelectCategory(const QString& category)
{
    // Check if this category have more then one subtype.
    QStringList types;
    GetIEditor()->GetObjectManager()->GetClassTypes(category, types);
    if (types.size() == 1)
    {
        // If only one or less sub types in this category, assume it type itsel, and start creation.
        StartCreation(types[0]);
        return;
    }

    // Check if this category have more then one subtype.
    ObjectTypeBrowser* panel = new ObjectTypeBrowser(this, category, nullptr);

    if (category == "Geometry")
    {
        m_objectBrowserPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Geometry Type", panel, 1);
    }
    else
    {
        m_objectBrowserPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Object Type", panel, 1);
    }

    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::StartCreation(const QString& type, const QString& param)
{
    // Delete object currently in creation.
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }

    m_objectType = type;

    CObjectClassDesc* clsDesc = GetIEditor()->GetObjectManager()->FindClass(type);
    if (!clsDesc)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), tr("Warning"), tr("Object creation failed, unknown object type."));
        return;
    }
    if (param.isEmpty())
    {
        QString fileSpec = clsDesc->GetFileSpec();
        if (!fileSpec.isEmpty())
        {
            //! Check if file spec contain wildcards.
            if (fileSpec.contains("*"))
            {
                // Create file browser panel.
                // When file is selected OnSelectFile callback will be called and creation process will be finalized.
                CPanelTreeBrowser* br = new CPanelTreeBrowser(functor(*this, &CObjectCreateTool::OnSelectFile), clsDesc->GetFileSpec());
                br->SetDragAndDropCallback(functor(*this, &CObjectCreateTool::OnCreateObjectFromFile));
                if (m_fileBrowserPanelId == 0)
                {
                    m_fileBrowserPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Browser", br, 1);
                }
                br->AddPreviewPanel();
                MainWindow::instance()->setFocus();
                return;
            }
        }
        OnSelectFile(fileSpec);
    }
    else
    {
        OnSelectFile(param);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::OnCreateObjectFromFile(QString file)
{
    FileChanged(file, true);
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::OnSelectFile(QString file)
{
    if (m_object && m_object->GetType() == OBJTYPE_SOLID)
    {
        FileChanged(file, true);
    }
    else
    {
        FileChanged(file, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::FileChanged(QString file, bool bMakeNew)
{
    if (m_objectType.isEmpty())
    {
        CancelCreation();
        return;
    }

    m_file = file;
    QWaitCursor wait;

    GetIEditor()->ClearSelection();

    Matrix34 objectTM;
    objectTM.SetIdentity();
    if (m_object && !bMakeNew)
    {
        // Delete previous object.
        objectTM = m_object->GetLocalTM();
        GetIEditor()->DeleteObject(m_object);
        m_object = 0;
    }

    m_object = GetIEditor()->NewObject(m_objectType.toUtf8().data(), file.toUtf8().data());
    if (m_object)
    {
        m_object->SetLocalTM(objectTM);
        // Close file browser if was open, not needed anymore.
        //CloseFileBrowser();

        // if this object type was hidden by category, re-display it.
        int hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
        hideMask = hideMask & ~(m_object->GetType());
        GetIEditor()->GetDisplaySettings()->SetObjectHideMask(hideMask);

        // Enable display of current layer.
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
        pLayer->SetFrozen(false);
        pLayer->SetVisible(true);
        pLayer->SetModified();

        if (!m_createCallback)
        {
            GetIEditor()->GetObjectManager()->BeginEditParams(m_object, OBJECT_CREATE);
        }
        // Document modified.
        GetIEditor()->SetModifiedFlag();
        GetIEditor()->SetModifiedModule(eModifiedBrushes);
    }

    if (m_createCallback)
    {
        m_createCallback(this, m_object);
    }

    if (m_object)
    {
        m_pMouseCreateCallback = m_object->GetMouseCreateCallback();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::CancelCreation()
{
    // Make sure created object is unselected.
    if (m_object)
    {
        // Destroy ourself.
        GetIEditor()->DeleteObject(m_object);
        m_object = nullptr;
        GetIEditor()->SetEditTool(0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::Display(DisplayContext& dc)
{
    if (m_pMouseCreateCallback)
    {
        m_pMouseCreateCallback->Display(dc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::AcceptCreation(bool bAbortTool)
{
    // Make sure created object is unselected.
    if (m_object)
    {
        if (bAbortTool)
        {
            Abort();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjectCreateTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    // Currently only an object of CSolidBrushObject class has its own IMouseCreateCallback object.
    // We need to process events of mousecallback differently according to whether m_pMouseCreateCallback is NULL or not
    // to improve a way of creating a solid.
    if (m_pMouseCreateCallback == NULL)
    {
        return MouseCallBackWithOutMouseCreateCB(view, event, point, flags);
    }

    return MouseCallBackWithMouseCreateCB(view, event, point, flags);
}


bool CObjectCreateTool::MouseCallBackWithMouseCreateCB(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (m_pMouseCreateCallback == NULL)
    {
        return false;
    }

    bool bCreateNewObject = m_pMouseCreateCallback->ContinueCreation();

    if (m_object != NULL)
    {
        GetIEditor()->SuspendUndo();
        int res = m_pMouseCreateCallback->OnMouseEvent(view, event, point, flags);
        GetIEditor()->ResumeUndo();

        if (res == MOUSECREATE_ABORT)
        {
            CancelCreation();   // Cancel object creation.
        }
        else if (res == MOUSECREATE_OK)
        {
            AcceptCreation(!bCreateNewObject);
            return true;
        }

        return true;
    }

    // we didnt handled this event, let parent(s) do it (maybe CObjectMode tool)
    return false;
}


bool CObjectCreateTool::MouseCallBackWithOutMouseCreateCB(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (m_object != NULL)
    {
        GetIEditor()->SuspendUndo();
        int res = m_object->MouseCreateCallback(view, event, point, flags);
        GetIEditor()->ResumeUndo();

        if (res == MOUSECREATE_ABORT)
        {
            CancelCreation();   // Cancel object creation.
        }
        else if (res == MOUSECREATE_OK)
        {
            AcceptCreation(true);
            return true;
        }

        // we handled this event
        return true;
    }

    // we didn't handle this event, let the parent(s) do it (maybe CObjectMode tool)
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::BeginEditParams(IEditor* ie, int flags)
{
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::EndEditParams()
{
    if (m_objectBrowserPanelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_objectBrowserPanelId);
        m_objectBrowserPanelId = 0;
    }
    CloseFileBrowser();
}

//////////////////////////////////////////////////////////////////////////
bool CObjectCreateTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE || nChar == VK_DELETE)
    {
        CancelCreation();
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectCreateTool::OnSetCursor(CViewport* vp)
{
    vp->SetCursor(m_hCreateCursor);
    return true;
}

bool CObjectCreateTool::IsUpdateUIPanel()
{
    if (m_object)
    {
        return false; // if we're actually dragging an object to create then you must finish this before you can edit properties
    }

    // you may actally still edit the selected object if you haven't started creating a new one
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CObjectCreateTool::SetUserData(const char* key, void* userData)
{
    if (0 == strcmp(key, "category"))
    {
        SelectCategory((const char*)userData);
    }
    else if (0 == strcmp(key, "type"))
    {
        StartCreation((const char*)userData);
    }
}

REGISTER_QT_CLASS_DESC_SYSTEM_ID(CObjectCreateTool, "EditTool.ObjectCreate", "Object", ESYSTEM_CLASS_EDITTOOL)

#include <ObjectCreateTool.moc>