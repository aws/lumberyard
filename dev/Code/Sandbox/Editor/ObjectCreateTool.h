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

// Description : Definition of ObjectCreateTool, edit tool for creating objects..


#ifndef CRYINCLUDE_EDITOR_OBJECTCREATETOOL_H
#define CRYINCLUDE_EDITOR_OBJECTCREATETOOL_H

#pragma once

class CBaseObject;

//////////////////////////////////////////////////////////////////////////
class CObjectCreateTool
    : public CEditTool
{
    Q_OBJECT
public:
    // This callback called when creation tool created new object.
    typedef Functor2<CObjectCreateTool*, CBaseObject*> CreateCallback;

    Q_INVOKABLE CObjectCreateTool(CreateCallback createCallback = 0);
    ~CObjectCreateTool();

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CEditTool
    virtual void SetUserData(const char* key, void* userData);
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool Activate(CEditTool* pPreviousTool);

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; };
    virtual bool OnSetCursor(CViewport* vp);

    bool IsUpdateUIPanel() override;

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // CObjectCreateTool methods.
    void SelectCategory(const QString& category);
    //! Start creation of object with given type.
    void StartCreation(const QString& type, const QString& param = "");
    //! Cancel creation of object.
    void CancelCreation();
    //! Cancel creation of object.
    void AcceptCreation(bool bAbortTool);

    static const GUID& GetClassID()
    {
        // {FC539AA9-9CA0-4db7-BBC3-B444F5FF8C6B}
        static const GUID guid = {
            0xfc539aa9, 0x9ca0, 0x4db7, { 0xbb, 0xc3, 0xb4, 0x44, 0xf5, 0xff, 0x8c, 0x6b }
        };
        return guid;
    }

protected:
    // Delete itself.
    void DeleteThis() { delete this; };
    //! Callback called when file selected in file browser.
    void OnSelectFile(QString file);
    void OnCreateObjectFromFile(QString file);

    void FileChanged(QString file, bool bMakeNewObject);

    void CloseFileBrowser();

    bool MouseCallBackWithMouseCreateCB(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool MouseCallBackWithOutMouseCreateCB(CViewport* view, EMouseEvent event, QPoint& point, int flags);

private:
    int m_objectBrowserPanelId;
    int m_fileBrowserPanelId;

    QCursor m_hCreateCursor;
    CreateCallback m_createCallback;

    //! Created object type.
    QString m_objectType;
    QString m_file;
    //! Created object.
    CBaseObjectPtr m_object;

    IMouseCreateCallback* m_pMouseCreateCallback;
};


#endif // CRYINCLUDE_EDITOR_OBJECTCREATETOOL_H
