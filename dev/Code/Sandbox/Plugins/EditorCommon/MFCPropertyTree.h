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

#ifndef CRYINCLUDE_EDITORCOMMON_MFCPROPERTYTREE_H
#define CRYINCLUDE_EDITORCOMMON_MFCPROPERTYTREE_H
#pragma once

#include <QObject>
#include <Include/IPropertyTree.h>
#include <Include/IPropertyTreeDialog.h>

namespace Serialization {
    class IArchive;
}
using Serialization::IArchive;
class QPropertyTree;
class CMFCPropertyTree;

class CMFCPropertyTreeSignalHandler
    : public QObject
{
    Q_OBJECT

public:

    CMFCPropertyTreeSignalHandler(CMFCPropertyTree* propertyTree);

protected slots:

    void OnSizeChanged();
    void OnPropertyChanged();

private:

    CMFCPropertyTree*   m_propertyTree;
};

class CMFCPropertyTree
    : public IPropertyTree
{
    DECLARE_DYNCREATE(CMFCPropertyTree)
public:

    CMFCPropertyTree();
    ~CMFCPropertyTree();

    BOOL Create(CWnd* parent, const RECT& rect) override;
    void Release() override;

    void Serialize(IArchive& ar) override;
    void Attach(Serialization::SStruct& ser) override;
    void Detach() override;
    void Revert() override;
    void SetExpandLevels(int expandLevels) override;
    void SetCompact(bool compact) override;
    void SetArchiveContext(Serialization::SContextLink* context) override;
    void SetHideSelection(bool hideSelection) override;
    void SetPropertyChangeHandler(const Functor0& callback) override;
    void SetSizeChangeHandler(const Functor0& callback) override;
    SIZE ContentSize() const override;

    static const char* ClassName() { return "PropertyTreeMFCAdapter"; }
    static bool RegisterWindowClass();
    static void UnregisterWindowClass();

    DECLARE_MESSAGE_MAP()
protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* oldWnd);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

private:
    friend class CMFCPropertyTreeSignalHandler;
    QPropertyTree* m_propertyTree;
    CMFCPropertyTreeSignalHandler* m_signalHandler;
    Functor0 m_sizeChangeCallback;
    Functor0 m_propertyChangeCallback;
};

class CMFCPropertyTreeDialog
    : public IPropertyTreeDialog
{
    DECLARE_DYNCREATE(CMFCPropertyTreeDialog)
public:
    bool Edit(Serialization::SStruct& ser, const char* title, HWND parentWindow, const char* stateFilename = 0) override;
    void Release() override;
};

void RegisterMFCPropertyTreeClass();
void UnregisterMFCPropertyTreeClass();

#endif // CRYINCLUDE_EDITORCOMMON_MFCPROPERTYTREE_H
