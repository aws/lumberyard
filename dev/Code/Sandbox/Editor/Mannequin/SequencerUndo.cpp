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
#include "SequencerUndo.h"
#include "Objects/SequenceObject.h"
#include "ISequencerSystem.h"
#include "SequencerSequence.h"
#include "AnimationContext.h"

//////////////////////////////////////////////////////////////////////////
CUndoSequencerSequenceModifyObject::CUndoSequencerSequenceModifyObject(CSequencerTrack* track, CSequencerSequence* pSequence)
{
    // Stores the current state of this track.
    assert(track != 0);

    m_pTrack = track;
    m_pSequence = pSequence;

    // Store undo info.
    m_undo = XmlHelpers::CreateXmlNode("Undo");
    m_pTrack->Serialize(m_undo, false);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequencerSequenceModifyObject::Undo(bool bUndo)
{
    if (!m_undo)
    {
        return;
    }

    if (bUndo)
    {
        m_redo = XmlHelpers::CreateXmlNode("Redo");
        m_pTrack->Serialize(m_redo, false);
    }
    // Undo track state.
    m_pTrack->Serialize(m_undo, true);

    if (bUndo)
    {
        // Refresh stuff after undo.
        GetIEditor()->UpdateSequencer(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequencerSequenceModifyObject::Redo()
{
    if (!m_redo)
    {
        return;
    }

    // Redo track state.
    m_pTrack->Serialize(m_redo, true);

    // Refresh stuff after undo.
    GetIEditor()->UpdateSequencer(true);
}
