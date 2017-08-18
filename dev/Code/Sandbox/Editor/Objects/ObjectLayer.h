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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_OBJECTLAYER_H
#define CRYINCLUDE_EDITOR_OBJECTS_OBJECTLAYER_H
#pragma once

#include <QColor>

// forward declarations.
class CObjectLayerManager;
class CObjectManager;

//////////////////////////////////////////////////////////////////////////
/*!
     Object Layer.
     All objects are organized in hierarchical layers.
     Every layer can be made hidden/frozen or can be exported or imported back.
*/
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CObjectLayer
    : public CRefCountBase
{
public:
    CObjectLayer(const GUID* pGUID = 0);

    void BuildGuid();
    //! Set layer name.
    void SetName(const QString& name, bool IsUpdateDepends = false);
    //! Get layer name.
    const QString& GetName() const { return m_name; }

    //! Get layer name including its parent names.
    QString GetFullName() const;

    //! Get GUID assigned to this layer.
    REFGUID GetGUID() const { return m_guid; }

    //////////////////////////////////////////////////////////////////////////
    // Query layer status.
    //////////////////////////////////////////////////////////////////////////
    bool IsVisible() const { return !m_hidden; }
    bool IsFrozen() const { return m_frozen; }
    bool IsExternal() const { return m_external; };
    bool IsExportable() const { return m_exportable; };
    bool IsExporLayerPak() const { return m_exportLayerPak; };
    bool IsDefaultLoaded() const { return m_defaultLoaded; };
    bool IsPhysics() const { return m_havePhysics; }
    int GetSpecs() const { return m_specs; }
    QColor GetColor() const;
    bool IsDefaultColor() const;

    //////////////////////////////////////////////////////////////////////////
    // Set layer status.
    //////////////////////////////////////////////////////////////////////////
    void SetVisible(bool b, bool bRecursive = false);
    void SetFrozen(bool b, bool bRecursive = false);
    void SetExternal(bool b) { m_external = b; };
    void SetExportable(bool b) { m_exportable = b; };
    void SetExportLayerPak(bool b) { m_exportLayerPak = b; };
    void SetDefaultLoaded(bool b) { m_defaultLoaded = b; };
    void SetHavePhysics(bool bHave) { m_havePhysics = bHave; }
    void SetSpecs(int specs) { m_specs = specs; }
    void SetColor(const QColor& color);

    //////////////////////////////////////////////////////////////////////////
    //! Save/Load layer to/from xml node.
    void SerializeBase(XmlNodeRef& node, bool bLoading);
    void Serialize(XmlNodeRef& node, bool bLoading);

    //! Get number of objects.
    int GetObjectCount() const;

    //////////////////////////////////////////////////////////////////////////
    // Child layers.
    //////////////////////////////////////////////////////////////////////////
    void AddChild(CObjectLayer* pLayer);
    void RemoveChild(CObjectLayer* pLayer);
    int GetChildCount() const { return m_childLayers.size(); }
    CObjectLayer* GetChild(int index) const;
    CObjectLayer* GetParent() const { return m_parent; }

    //! Check if specified layer is direct or indirect parent of this layer.
    bool IsChildOf(CObjectLayer* pParent);
    //! Find child layer with specified GUID.
    //! Search done recursively, so it find child layer in any depth.
    CObjectLayer* FindChildLayer(REFGUID guid);
    //////////////////////////////////////////////////////////////////////////

    bool IsParentExternal() const;
    void SetModified(bool isModified = true);
    bool IsModified() { return m_isModified; }

    //////////////////////////////////////////////////////////////////////////
    // User interface support.
    //////////////////////////////////////////////////////////////////////////
    void Expand(bool bExpand);
    bool IsExpanded() const;

    // Setup Layer ID for LayerSwith
    void SetLayerID(uint16 nLayerId);
    uint16 GetLayerID() const;

    QString GetExternalLayerPath();

private:
    void GetFullNameRecursively(const CObjectLayer* const pObjectLayer, QString& name) const;

    friend CObjectLayerManager;
    //! GUID of this layer.
    GUID m_guid;
    //! Layer Name.
    QString m_name;

    // Layer state.
    //! If true Object of this layer is hidden.
    bool m_hidden;
    //! If true Object of this layer is frozen (not selectable).
    bool m_frozen;
    //! True if layer is stored externally to project file.
    bool m_external;
    //! True if objects on this layer must be exported to the game.
    bool m_exportable;
    //! True if layer needs to create its own per layer pak for faster streaming.
    bool m_exportLayerPak;
    //! True if layer needs to be activated on default level loading (map command)
    bool m_defaultLoaded;
    //! True when brushes from layer have physics
    bool m_havePhysics;

    //! True when layer is expanded in GUI. (Should not be serialized)
    bool m_expanded;

    //! Layer switch on only for specified specs, like PC, etc.
    int m_specs;

    //! True when layer was changed
    bool m_isModified;

    //! Backgroung color in list box
    QColor m_color;
    bool m_isDefaultColor;

    // List of child layers.
    typedef std::vector<TSmartPtr<CObjectLayer> > ChildLayers;
    ChildLayers m_childLayers;

    //! Pointer to parent layer.
    CObjectLayer* m_parent;
    //! Parent layer GUID.
    GUID m_parentGUID;

    //! Layer ID for LayerSwith
    uint16 m_nLayerId;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_OBJECTLAYER_H
