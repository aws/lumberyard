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

// Description : A parent class for mannequin editor pages


#ifndef __MannequinEditorPage_h__
#define __MannequinEditorPage_h__
#pragma once

#include <QWidget>

class CMannequinModelViewport;
class CMannDopeSheet;
class CMannNodesWidget;

class CMannequinEditorPage
    : public QWidget
{
    Q_OBJECT
public:
    CMannequinEditorPage(QWidget* pParentWnd = nullptr);
    virtual ~CMannequinEditorPage();

    virtual CMannequinModelViewport* ModelViewport() const { return NULL; }
    virtual CMannDopeSheet* TrackPanel() { return NULL; }
    virtual CMannNodesWidget* Nodes() { return NULL; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

    virtual void ValidateToolbarButtonsState() {};

private:
};

#endif // __MannequinEditorPage_h__
