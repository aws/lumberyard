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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWUNDO_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWUNDO_H
#pragma once


#include "TrackViewTrack.h"

class CTrackViewNode;
class CTrackViewSequence;
class CTrackViewAnimNode;

/** Undo object for sequence settings
*/
class CUndoSequenceSettings
    : public IUndoObject
{
public:
    CUndoSequenceSettings(CTrackViewSequence* pSequence);

protected:
    virtual int GetSize() override { return sizeof(*this); }
    virtual QString GetDescription() override { return "Undo Sequence Settings"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    CTrackViewSequence* m_pSequence;

    Range m_oldTimeRange;
    Range m_newTimeRange;
    IAnimSequence::EAnimSequenceFlags m_newFlags;
    IAnimSequence::EAnimSequenceFlags m_oldFlags;
};

/** Undo object stored when keys were selected
*/
class CUndoAnimKeySelection
    : public IUndoObject
{
    friend class CUndoTrackObject;
public:
    CUndoAnimKeySelection(CTrackViewSequence* pSequence);

    // Checks if the selection was actually changed
    bool IsSelectionChanged() const;

protected:
    CUndoAnimKeySelection(CTrackViewTrack* pTrack);

    virtual int GetSize() override { return sizeof(*this); }
    virtual QString GetDescription() override { return "Undo Sequence Key Selection"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

    std::vector<bool> SaveKeyStates(CTrackViewSequence* pSequence) const;
    void RestoreKeyStates(CTrackViewSequence* pSequence, const std::vector<bool> keyStates);

    CTrackViewSequence* m_pSequence;
    std::vector<bool> m_undoKeyStates;
    std::vector<bool> m_redoKeyStates;
};

/** Undo object stored when track is modified.
*/
class CUndoTrackObject
    : public CUndoAnimKeySelection
{
public:
    CUndoTrackObject(CTrackViewTrack* pTrack, bool bStoreKeySelection = true);

protected:
    virtual int GetSize() override { return sizeof(*this); }
    virtual QString GetDescription() override { return "Undo Track Modify"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    CTrackViewTrack* m_pTrack;
    bool m_bStoreKeySelection;

    CTrackViewTrackMemento m_undo;
    CTrackViewTrackMemento m_redo;
};


/** Undo object stored when track is modified for component entity.
* Stores ids, not raw pointers.
*/
class CUndoComponentEntityTrackObject
    : public IUndoObject
{
public:
    CUndoComponentEntityTrackObject(CTrackViewTrack* track);

protected:
    virtual int GetSize() override { return sizeof(*this); }
    virtual QString GetDescription() override { return "Undo Component Entity Track Modify"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:

    // Helper function to get a pointer to the track based.
    CTrackViewTrack* FindTrack(CTrackViewSequence* sequence);

    // Internal state are id's used to uniquely identify
    // a track. This does not store pointers to tracks because
    // those can change when an AZ::Undo event happens and the entity
    // is reloaded.
    AZ::EntityId m_sequenceId;
    AZ::EntityId m_entityId;
    AZStd::string m_trackName;
    AZ::ComponentId m_trackComponentId;

    CTrackViewTrackMemento m_undo;
    CTrackViewTrackMemento m_redo;
};

/** Base class for sequence add/remove
*/
class CAbstractUndoSequenceTransaction
    : public IUndoObject
{
public:
    CAbstractUndoSequenceTransaction(CTrackViewSequence* pSequence);

protected:
    void AddSequence();
    void RemoveSequence(bool bAquireOwnership);

    CTrackViewSequence* m_pSequence;

    // This smart pointer is used to hold the sequence if not in TV anymore
    std::unique_ptr<CTrackViewSequence> m_pStoredTrackViewSequence;
};

/** Undo for adding a sequence
*/
class CUndoSequenceAdd
    : public CAbstractUndoSequenceTransaction
{
public:
    CUndoSequenceAdd(CTrackViewSequence* pNewSequence)
        : CAbstractUndoSequenceTransaction(pNewSequence) {}

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Add Sequence"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for remove a sequence
*/
class CUndoSequenceRemove
    : public CAbstractUndoSequenceTransaction
{
public:
    CUndoSequenceRemove(CTrackViewSequence* pRemovedSequence);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Remove Sequence"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Base class for anim node add/remove
*/
class CAbstractUndoAnimNodeTransaction
    : public IUndoObject
{
public:
    CAbstractUndoAnimNodeTransaction(CTrackViewAnimNode* pNode);

protected:
    void AddNode();
    void RemoveNode(bool bAquireOwnership);

    CTrackViewAnimNode* m_pParentNode;
    CTrackViewAnimNode* m_pNode;

    // This smart pointer is used to hold the node if not in the sequence anymore.
    std::unique_ptr<CTrackViewNode> m_pStoredTrackViewNode;
};

/** Undo for adding a sub node to a node
*/
class CUndoAnimNodeAdd
    : public CAbstractUndoAnimNodeTransaction
{
public:
    CUndoAnimNodeAdd(CTrackViewAnimNode* pNewNode)
        : CAbstractUndoAnimNodeTransaction(pNewNode) {}

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Add TrackView Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for remove a sub node from a node
*/
class CUndoAnimNodeRemove
    : public CAbstractUndoAnimNodeTransaction
{
public:
    CUndoAnimNodeRemove(CTrackViewAnimNode* pRemovedNode);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Remove TrackView Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Base class for anim track add/remove
*/
class CAbstractUndoTrackTransaction
    : public IUndoObject
{
public:
    CAbstractUndoTrackTransaction(CTrackViewTrack* pTrack);

protected:
    void AddTrack();
    void RemoveTrack(bool bAquireOwnership);

private:
    CTrackViewAnimNode* m_pParentNode;
    CTrackViewTrack* m_pTrack;

    // This smart pointers is used to hold the track if not in the sequence anymore.
    std::unique_ptr<CTrackViewNode> m_pStoredTrackViewTrack;
};

/** Undo for adding a track to a node
*/
class CUndoTrackAdd
    : public CAbstractUndoTrackTransaction
{
public:
    CUndoTrackAdd(CTrackViewTrack* pNewTrack)
        : CAbstractUndoTrackTransaction(pNewTrack) {}

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Add TrackView Track"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for removing a track from a node
*/
class CUndoTrackRemove
    : public CAbstractUndoTrackTransaction
{
public:
    CUndoTrackRemove(CTrackViewTrack* pRemovedTrack);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Remove TrackView Track"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for re-parenting an anim node
*/
class CUndoAnimNodeReparent
    : public CAbstractUndoAnimNodeTransaction
{
public:
    CUndoAnimNodeReparent(CTrackViewAnimNode* pAnimNode, CTrackViewAnimNode* pNewParent);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Reparent TrackView Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    void Reparent(CTrackViewAnimNode* pNewParent);
    void AddParentsInChildren(CTrackViewAnimNode* pCurrentNode);

    CTrackViewAnimNode* m_pNewParent;
    CTrackViewAnimNode* m_pOldParent;
};

/* Undo for renaming an anim node
*/
class CUndoAnimNodeRename
    : public IUndoObject
{
public:
    CUndoAnimNodeRename(CTrackViewAnimNode* pNode, const string& oldName);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Rename TrackView Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    CTrackViewAnimNode* m_pNode;
    string m_newName;
    string m_oldName;
};

/* Undo for renaming an anim entity node
*/
class CUndoAnimNodeObjectRename
    : public IUndoObject
{
public:
    CUndoAnimNodeObjectRename(CBaseObject* entityObj, const string& newName);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual QString GetDescription() override { return "Undo Rename TrackView Entity Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

    void Rename(const string& newName);

private:
    CBaseObject* m_baseObject;
    string m_newName;
    string m_oldName;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWUNDO_H
