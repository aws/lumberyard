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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SEQUENCEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_SEQUENCEOBJECT_H
#pragma once


#include "BaseObject.h"
#include <QtViewPane.h>

class CSequenceObject
    : public CBaseObject
    , public IAnimLegacySequenceObject
{
    Q_OBJECT
public:
    struct CZoomScrollSettings
    {
        // For dope sheet
        float dopeSheetZoom;
        int dopeSheetHScroll;
        // For nodes list
        float nodesListVScroll;
        // For curve editor
        Vec2 curveEditorZoom;
        Vec2 curveEditorScroll;

        CZoomScrollSettings()
            : dopeSheetZoom(100.0f)
            , dopeSheetHScroll(0)
            , nodesListVScroll(0)
            , curveEditorZoom(200, 100)
            , curveEditorScroll(0, 0) {}

        void Save(XmlNodeRef node) const;
        bool Load(XmlNodeRef node);
    };

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Display(DisplayContext& dc);

    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);
    bool CreateGameObject();
    void Done();

    bool HasValidZoomScrollSettings() const
    { return m_bValidSettings; }
    void SetZoomScrollSettings(const CZoomScrollSettings& settings)
    {
        m_zoomScrollSettings = settings;
        m_bValidSettings = true;
    }
    void GetZoomScrollSettings(CZoomScrollSettings& settings) const
    {
        assert(m_bValidSettings);
        settings = m_zoomScrollSettings;
    }

    IAnimSequence* GetSequence() {return m_sequence.get(); }

    //////////////////////////////////////////////////////////////////////////
    // Overrides from IAnimLegacySequenceObject
    void OnNameChanged() override;
    void OnModified() override;
    //////////////////////////////////////////////////////////////////////////

protected:
    friend class CTemplateObjectClassDesc<CSequenceObject>;
    //! Dtor must be protected.
    CSequenceObject();

    void DeleteThis() { delete this; };

    static const GUID& GetClassID()
    {
        // {3951b7f0-51df-4765-8b6f-13493818b9c7}
        static const GUID guid = {
            0x3951b7f0, 0x51df, 0x4765, { 0x8b, 0x6f, 0x13, 0x49, 0x38, 0x18, 0xb9, 0xc7 }
        };
        return guid;
    }
    //////////////////////////////////////////////////////////////////////////
    // Local callbacks.

private:
    AZStd::intrusive_ptr<struct IAnimSequence> m_sequence;

    // Zoom/scroll settings for this sequence in TrackView dialog
    CZoomScrollSettings m_zoomScrollSettings;
    bool m_bValidSettings;
    uint32 m_sequenceId;
};

	
#endif // CRYINCLUDE_EDITOR_OBJECTS_SEQUENCEOBJECT_H
