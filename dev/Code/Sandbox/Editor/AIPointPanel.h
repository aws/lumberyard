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

#ifndef CRYINCLUDE_EDITOR_AIPOINTPANEL_H
#define CRYINCLUDE_EDITOR_AIPOINTPANEL_H
#pragma once

#include <QWidget>
#include "Controls/PickObjectButton.h"
#include "Objects/AiPoint.h"

class QStringListModel;
class QItemSelection;

namespace Ui
{
    class AIPointPanel;
}

class CAIPointPanel
    : public QWidget
{
    Q_OBJECT

public:
    CAIPointPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CAIPointPanel();

    void SetObject(CAIPoint* object);
    void StartPick();
    void StartPickImpass();

protected slots:
    void OnBnClickedRegenLinks();
    void OnBnClickedSelect();
    void OnBnClickedRemove();
    void OnBnClickedRemoveAll();
    void OnBnClickedRemoveAllInArea();
    void OnLbnLinksSelChange();
    void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

protected:
    void ReloadLinks();
    // Called by SPickObjectCallback
    void OnPick(bool impass, CBaseObject* picked);
    bool OnPickFilter(bool impass, CBaseObject* picked);
    void OnCancelPick(bool impass);

    void SetAIType(EAIPointType eAIPointType);
    void SetAINavigationType(EAINavigationType eAINavigationType);

    void EnableLinkedWaypointsUI();

    QScopedPointer<Ui::AIPointPanel> m_ui;

    CAIPoint* m_object;

    QStringListModel* m_linksModel;

    // Need this to filter the two pick buttons
    struct SPickObjectCallback
        : public IPickObjectCallback
    {
        SPickObjectCallback(CAIPointPanel* owner, bool impass)
            : m_owner(owner)
            , m_impass(impass)
        { }
        // Do this via Init since MS compiler warnins about passing "this" in initialisers
        void Init(CAIPointPanel* owner, bool impass) {m_owner = owner; m_impass = impass; }
        virtual void OnPick(CBaseObject* picked) {m_owner->OnPick(m_impass, picked); }
        virtual bool OnPickFilter(CBaseObject* picked) {return m_owner->OnPickFilter(m_impass, picked); }
        virtual void OnCancelPick() {m_owner->OnCancelPick(m_impass); }
    private:
        CAIPointPanel* m_owner;
        bool m_impass;
    };
    SPickObjectCallback m_pickCallback;
    SPickObjectCallback m_pickImpassCallback;

protected slots:
    void OnBnClickedWaypoint();
    void OnBnClickedHidepoint();
    void OnBnClickedHidepointSecondary();
    void OnBnClickedEntrypoint();
    void OnBnClickedExitpoint();
    void OnBnClickedRemovable();
    void OnBnClickedHuman();
    void OnBnClicked3dsurface();
};

#endif // CRYINCLUDE_EDITOR_AIPOINTPANEL_H
