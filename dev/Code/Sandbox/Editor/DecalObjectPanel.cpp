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
#include "DecalObjectPanel.h"

#include "Objects/DecalObject.h"

#include <ui_DecalObjectPanel.h>


//////////////////////////////////////////////////////////////////////////
// CDecalObjectTool dialog
//////////////////////////////////////////////////////////////////////////


class CDecalObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    CDecalObjectTool();

    virtual void Display(DisplayContext& dc) {};
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual void SetUserData(const char* userKey, void* userData);

protected:
    virtual ~CDecalObjectTool();
    void DeleteThis() { delete this; };

private:
    CDecalObject* m_pDecalObj;
};


//////////////////////////////////////////////////////////////////////////

CDecalObjectTool::CDecalObjectTool()
{
}

//////////////////////////////////////////////////////////////////////////
CDecalObjectTool::~CDecalObjectTool()
{
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObjectTool::OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_pDecalObj)
    {
        return true;
    }

    if (m_pDecalObj->MouseCallbackImpl(view, event, point, flags))
    {
        return true;
    }

    if (event == eMouseLDown)
    {
        Abort();
        GetIEditor()->ClearSelection();
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CDecalObjectTool::SetUserData(const char* userKey, void* userData)
{
    m_pDecalObj = static_cast< CDecalObject* >(userData);
}


//////////////////////////////////////////////////////////////////////////
// CDecalObjectPanel dialog
//////////////////////////////////////////////////////////////////////////

CDecalObjectPanel::CDecalObjectPanel(QWidget* pParent)
    : QWidget(pParent)
    , m_ui(new Ui::DecalObjectPanel)
    , m_pDecalObj(nullptr)
{
    m_ui->setupUi(this);
    connect(m_ui->updateProjectionButton, &QPushButton::clicked, this, &CDecalObjectPanel::OnUpdateProjection);
}

//////////////////////////////////////////////////////////////////////////
CDecalObjectPanel::~CDecalObjectPanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CDecalObjectPanel::SetDecalObject(CDecalObject* pObj)
{
    m_pDecalObj = pObj;
    if (m_pDecalObj)
    {
        m_ui->reorientateButton->SetToolClass(&CDecalObjectTool::staticMetaObject, "decal", m_pDecalObj);
    }
    else
    {
        m_ui->reorientateButton->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CDecalObjectPanel::OnUpdateProjection()
{
    if (m_pDecalObj && m_pDecalObj->GetProjectionType() > 0)
    {
        m_pDecalObj->UpdateEngineNode();
    }
}

#include <DecalObjectPanel.moc>