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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_ISEQUENCERSYSTEM_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_ISEQUENCERSYSTEM_H
#pragma once


class CSequencerTrack;
class CSequencerNode;
class CSequencerSequence;

class XmlNodeRef;

class QMenu;

#include <ICryMannequin.h>

#include <QMetaType>
#include <QStringList>

#include "QtUtil.h"

enum ESequencerParamType
{
    SEQUENCER_PARAM_UNDEFINED = 0,
    SEQUENCER_PARAM_FRAGMENTID,
    SEQUENCER_PARAM_TAGS,
    SEQUENCER_PARAM_PARAMS,
    SEQUENCER_PARAM_ANIMLAYER,
    SEQUENCER_PARAM_PROCLAYER,
    SEQUENCER_PARAM_TRANSITIONPROPS,
    SEQUENCER_PARAM_TOTAL
};


enum ESequencerNodeType
{
    SEQUENCER_NODE_UNDEFINED,
    SEQUENCER_NODE_SEQUENCE_ANALYZER_GLOBAL,
    SEQUENCER_NODE_SCOPE,
    SEQUENCER_NODE_FRAGMENT_EDITOR_GLOBAL,
    SEQUENCER_NODE_FRAGMENT,
    SEQUENCER_NODE_FRAGMENT_CLIPS,
    SEQUENCER_NODE_TOTAL
};



struct SKeyColour
{
    uint8 base[3];
    uint8 high[3];
    uint8 back[3];
};


struct CSequencerKey
    : public IKey
{
    virtual bool IsFileInsidePak() const { return m_fileState & eIsInsidePak; }
    virtual bool IsFileInsideDB() const { return m_fileState & eIsInsideDB; }
    virtual bool IsFileOnDisk() const { return m_fileState & eIsOnDisk; }
    virtual bool HasValidFile() const { return m_fileState & eHasFile; }
    virtual bool HasFileRepresentation() const
    {
        QStringList extensions;
        QString editableExtension;
        GetExtensions(extensions, editableExtension);
        return extensions.size() > 0;
    }

    virtual const QString GetFileName() const { return m_fileName; }
    virtual const QString GetFilePath() const { return m_filePath; }

    // Returns a list of all paths relevant to this file
    // E.G. for i_cafs will return paths to .ma, .icaf and .animsetting
    // N.B. path is relative to game root
    virtual void GetFilePaths(QStringList& paths, QString& relativePath, QString& fileName, QString& editableExtension) const
    {
        QStringList extensions;
        GetExtensions(extensions, editableExtension);

        relativePath = m_filePath;
        fileName = m_fileName;

        QString fullPath = Path::AddSlash(m_filePath.toUtf8().data());
        fullPath += m_fileName;

        paths.reserve(extensions.size());
        for (QStringList::const_iterator iter = extensions.begin(), itEnd = extensions.end(); iter != itEnd; ++iter)
        {
            paths.append(fullPath + (*iter));
        }
    }

    virtual void UpdateFlags()
    {
        m_fileState = eNone;
    };
protected:

    // Provides all extensions relevant to the clip in extensions, and the one used to edit the clip
    // in editableExtension...
    // E.G. for i_cafs will return .ma, .animsetting, .i_caf and return .ma also as editableExtension
    virtual void GetExtensions(QStringList& extensions, QString& editableExtension) const
    {
        extensions.clear();
        editableExtension.clear();
    };

    enum ESequencerKeyFileState
    {
        eNone = 0,
        eHasFile = BIT(0),
        eIsInsidePak = BIT(1),
        eIsInsideDB = BIT(2),
        eIsOnDisk = BIT(3),
    };

    ESequencerKeyFileState m_fileState; // Flags to represent the state of (m_filePath + m_fileName + m_fileExtension)
    QString m_filePath; // The path relative to game root
    QString m_fileName; // The filename of the key file (e.g. filename.i_caf for CClipKey)
};

class CSequencerTrack
    : public _i_reference_target_t
{
public:
    enum ESequencerTrackFlags
    {
        SEQUENCER_TRACK_HIDDEN   = BIT(1), //!< Set when track is hidden in track view.
        SEQUENCER_TRACK_SELECTED = BIT(2), //!< Set when track is selected in track view.
        SEQUENCER_TRACK_READONLY = BIT(3), //!< Set when track is read only
    };

public:
    CSequencerTrack()
        : m_nParamType(SEQUENCER_PARAM_UNDEFINED)
        , m_bModified(false)
        , m_flags(0)
        , m_changeCount(0)
        , m_muted(false)
    {
    }

    virtual ~CSequencerTrack() {}

    virtual void SetNumKeys(int numKeys) = 0;
    virtual int GetNumKeys() const = 0;

    virtual int CreateKey(float time) = 0;

    virtual void SetKey(int index, CSequencerKey* key) = 0;
    virtual void GetKey(int index, CSequencerKey* key) const = 0;

    virtual const CSequencerKey* GetKey(int index) const = 0;
    virtual CSequencerKey* GetKey(int index) = 0;

    virtual void RemoveKey(int num) = 0;

    virtual void GetKeyInfo(int key, QString& description, float& duration) = 0;
    virtual void GetTooltip(int key, QString& description, float& duration){ return GetKeyInfo(key, description, duration); }
    virtual float GetKeyDuration(const int key) const = 0;
    virtual const SKeyColour& GetKeyColour(int key) const = 0;
    virtual const SKeyColour& GetBlendColour(int key) const = 0;

    virtual bool CanEditKey(int key) const {return true; }
    virtual bool CanMoveKey(int key) const {return true; }

    virtual bool CanAddKey(float time) const {return true; }
    virtual bool CanRemoveKey(int key) const {return true; }

    virtual int CloneKey(int key) = 0;
    virtual int CopyKey(CSequencerTrack* pFromTrack, int nFromKey) = 0;

    virtual int GetNumSecondarySelPts(int key) const { return 0; }
    virtual int GetSecondarySelectionPt(int key, float timeMin, float timeMax) const { return 0; }
    virtual int FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const { return 0; }
    virtual void SetSecondaryTime(int key, int id, float time) {}
    virtual float GetSecondaryTime(int key, int id) const { return 0.0f; }
    virtual QString GetSecondaryDescription(int key, int id) const { return QString(); }
    virtual bool CanMoveSecondarySelection(int key, int id) const { return true; }
    virtual ECurveType GetBlendCurveType(int key) const { return ECurveType::Linear; }

    virtual void InsertKeyMenuOptions(QMenu* menu, int keyID) {}
    virtual void ClearKeyMenuOptions(QMenu* menu, int keyID) {}
    virtual void OnKeyMenuOption(int menuOption, int keyID) {}

    virtual ColorB GetColor() const { return ColorB(220, 220, 220); }

    ESequencerParamType GetParameterType() const { return m_nParamType; }
    void SetParameterType(ESequencerParamType type) { m_nParamType = type; }

    void UpdateKeys() { MakeValid(); }

    int NextKeyByTime(int key) const
    {
        if (key + 1 < GetNumKeys())
        {
            return key + 1;
        }
        else
        {
            return -1;
        }
    }

    int FindKey(float time) const
    {
        const int keyCount = GetNumKeys();
        for (int i = 0; i < keyCount; i++)
        {
            const CSequencerKey* pKey = GetKey(i);
            assert(pKey);
            if (pKey->time == time)
            {
                return i;
            }
        }
        return -1;
    }

    virtual void SetKeyTime(int index, float time)
    {
        CSequencerKey* key = GetKey(index);
        assert(key);
        if (CanMoveKey(index))
        {
            key->time = time;
            Invalidate();
        }
    }

    float GetKeyTime(int index) const
    {
        const CSequencerKey* key = GetKey(index);
        assert(key);
        return key->time;
    }

    void SetKeyFlags(int index, int flags)
    {
        CSequencerKey* key = GetKey(index);
        assert(key);
        key->flags = flags;
        Invalidate();
    }

    int GetKeyFlags(int index) const
    {
        const CSequencerKey* key = GetKey(index);
        assert(key);
        return key->flags;
    }

    virtual void SelectKey(int index, bool select)
    {
        CSequencerKey* key = GetKey(index);
        assert(key);
        if (select)
        {
            key->flags |= AKEY_SELECTED;
        }
        else
        {
            key->flags &= ~AKEY_SELECTED;
        }
    }

    bool IsKeySelected(int index) const
    {
        const CSequencerKey* key = GetKey(index);
        assert(key);
        const bool isKeySelected = (key->flags & AKEY_SELECTED);
        return isKeySelected;
    }

    void SortKeys()
    {
        DoSortKeys();
        m_bModified = false;
    }

    void SetFlags(int flags) { m_flags = flags; }
    int GetFlags() const { return m_flags; }

    void SetSelected(bool bSelect)
    {
        if (bSelect)
        {
            m_flags |= SEQUENCER_TRACK_SELECTED;
        }
        else
        {
            m_flags &= ~SEQUENCER_TRACK_SELECTED;
        }
    }

    void SetTimeRange(const Range& timeRange) { m_timeRange = timeRange; }
    const Range& GetTimeRange() const { return m_timeRange; }

    void OnChange()
    {
        m_changeCount++;
        OnChangeCallback();
    }
    uint32 GetChangeCount() const { return m_changeCount; }
    void ResetChangeCount() { m_changeCount = 0; }

    virtual bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) = 0;
    virtual bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) = 0;

    void Mute(bool bMute) { m_muted =  bMute; }
    bool IsMuted() const { return m_muted; }

protected:
    virtual void OnChangeCallback() {}
    virtual void DoSortKeys() = 0;

    void MakeValid()
    {
        if (m_bModified)
        {
            SortKeys();
        }
        assert(!m_bModified);
    }

    void Invalidate() { m_bModified = true; }

private:
    ESequencerParamType m_nParamType;
    bool m_bModified;
    Range m_timeRange;
    int m_flags;
    uint32 m_changeCount;
    bool m_muted;
};

Q_DECLARE_METATYPE(CSequencerTrack*);

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_ISEQUENCERSYSTEM_H
