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


#include "StdAfx.h"
#include "MannequinEditorPage.h"
#include "../MannequinModelViewport.h"
#include "../MannequinNodes.h"
#include "MannDopeSheet.h"

#include <QKeyEvent>

CMannequinEditorPage::CMannequinEditorPage(QWidget* pParentWnd)
    : QWidget(pParentWnd)
{
}

CMannequinEditorPage::~CMannequinEditorPage()
{
}

//////////////////////////////////////////////////////////////////////////
void CMannequinEditorPage::keyPressEvent(QKeyEvent* event)
{
    event->ignore();
    if (event->type() == QEvent::KeyPress)
    {
        if (event->key() == Qt::Key_F && isVisible())
        {
            CMannequinModelViewport* pViewport = ModelViewport();
            CMannDopeSheet* pTrackPanel = TrackPanel();
            CMannNodesWidget* pNodeCtrl = Nodes();

            if (pViewport == NULL)
            {
                return;
            }

            // Get the nodes from dope sheet and node control
            std::vector< CSequencerNode* > pNodes;
            std::vector< CSequencerNode* > pSelectedNodes;

            if (pTrackPanel)
            {
                pTrackPanel->GetSelectedNodes(pSelectedNodes);
            }

            if (pNodeCtrl)
            {
                pNodeCtrl->GetSelectedNodes(pNodes);
            }

            // If selected keys are from non-expanded tracks, then ignore them (causes confusion for users)
            if (pNodeCtrl)
            {
                std::vector<CSequencerNode*>::reverse_iterator itEnd = pSelectedNodes.rend();
                for (std::vector<CSequencerNode*>::reverse_iterator it = pSelectedNodes.rbegin(); it != itEnd; it++)
                {
                    if (!pNodeCtrl->IsNodeExpanded(*it))
                    {
                        pSelectedNodes.erase(--(it.base()));
                    }
                }
            }

            // Merge list, ensure not empty
            stl::push_back_range_unique(pSelectedNodes, pNodes.begin(), pNodes.end());
            if (pSelectedNodes.empty())
            {
                return;
            }

            AABB selectionBB, entityBB;
            bool bInitialised = false;
            for (std::vector<CSequencerNode*>::iterator it = pSelectedNodes.begin(); it != pSelectedNodes.end(); ++it)
            {
                IEntity* pEntity = (*it)->GetEntity();

                if (pEntity == NULL)
                {
                    continue;
                }

                pEntity->GetWorldBounds(entityBB);

                if (!bInitialised)
                {
                    selectionBB = entityBB;
                    bInitialised = true;
                }
                else
                {
                    selectionBB.Add(entityBB);
                }
            }

            if (bInitialised)
            {
                pViewport->Focus(selectionBB);

                // Focus can turn off first person cam, if this happens the button state needs updating
                ValidateToolbarButtonsState();
            }

            // We don't want to consume in case of data entry
            return;
        }
    }
}

#include <Mannequin/Controls/MannequinEditorPage.moc>