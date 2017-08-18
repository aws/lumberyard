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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERUNDO_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERUNDO_H
#pragma once


class CSequencerTrack;
class CSequencerNode;
class CSequencerSequence;

/** Undo object stored when track is modified.
*/
class CUndoSequencerSequenceModifyObject
    : public IUndoObject
{
public:
    CUndoSequencerSequenceModifyObject(CSequencerTrack* track, CSequencerSequence* pSequence);
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Track Modify"; };

    virtual void Undo(bool bUndo);
    virtual void Redo();

private:
    TSmartPtr<CSequencerTrack> m_pTrack;
    TSmartPtr<CSequencerSequence> m_pSequence;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};

class CAnimSequenceUndo
    : public CUndo
{
public:
    CAnimSequenceUndo(CSequencerSequence* pSeq, const char* sName)
        : CUndo(sName)
    {
        // CUndo::Record( new CUndoSequencerAnimSequenceObject(pSeq) );
    }
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERUNDO_H
