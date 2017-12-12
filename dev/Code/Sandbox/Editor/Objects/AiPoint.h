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

// Description : AIPoint object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_AIPOINT_H
#define CRYINCLUDE_EDITOR_OBJECTS_AIPOINT_H
#pragma once


#include "BaseObject.h"

enum EAIPointType
{
    EAIPOINT_WAYPOINT = 0,  //!< AI Graph node, waypoint.
    EAIPOINT_HIDE,          //!< Good hiding point.
    EAIPOINT_ENTRYEXIT,         //!< Entry/exit point to indoors.
    EAIPOINT_EXITONLY,          //!< Exit point from indoors.
    EAIPOINT_HIDESECONDARY,         //!< secondary hiding point (use for advancing, etc.).
};

enum EAINavigationType
{
    EAINAVIGATION_HUMAN = 0,  //!< Normal human-type waypoint navigation
    EAINAVIGATION_3DSURFACE,  //!< General 3D-surface navigation
};

struct IVisArea;

/*!
 *  CAIPoint is an object that represent named AI waypoint in the world.
 *
 */
class CAIPoint
    : public CBaseObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();
    void Display(DisplayContext& disp);
    //! Get object color.
    QColor GetColor() const;

    virtual QString GetTypeDescription() const;

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    bool HitTest(HitContext& hc);

    void GetLocalBounds(AABB& box);

    void OnEvent(ObjectEvent event);

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    //! Invalidates cached transformation matrix.
    virtual void InvalidateTM(int nWhyFlags);

    virtual void SetHelperScale(float scale);
    virtual float GetHelperScale() { return m_helperScale; };
    //////////////////////////////////////////////////////////////////////////

    //! Retreive number of link points.
    int GetLinkCount() const { return m_links.size(); }
    CAIPoint*   GetLink(int index);
    void AddLink(CAIPoint* obj, bool bIsPassable, bool bNeighbour = false);
    void RemoveLink(CAIPoint* obj, bool bNeighbour = false);
    bool IsLinkSelected(int iLink) const { return m_links[iLink].selected; };
    void SelectLink(int iLink, bool bSelect) { m_links[iLink].selected = bSelect; };

    /// Removes all of our links
    void RemoveAllLinks();

    void SetAIType(EAIPointType type);
    EAIPointType GetAIType() const { return m_aiType; };

    void SetAINavigationType(EAINavigationType type);
    EAINavigationType GetAINavigationType() const { return m_aiNavigationType; };

    const struct GraphNode* GetGraphNode() const {return m_aiNode; }

    void MakeRemovable(bool bRemovable);
    bool GetRemovable() const {return m_bRemovable; }

    // Enable picking of AI points.
    void StartPick();
    void StartPickImpass();

    // Prompts a regeneration of all links in the same nav region as this one
    void RegenLinks();

    //////////////////////////////////////////////////////////////////////////
    void Validate(IErrorReport* report) override;

protected:
    friend class CTemplateObjectClassDesc<CAIPoint>;

    //! Dtor must be protected.
    CAIPoint();
    ~CAIPoint();

    static const GUID& GetClassID()
    {
        // {07303078-B211-40b9-9621-9910A0271AB7}
        static const GUID guid = {
            0x7303078, 0xb211, 0x40b9, { 0x96, 0x21, 0x99, 0x10, 0xa0, 0x27, 0x1a, 0xb7 }
        };
        return guid;
    }

    void DeleteThis() { delete this; };

    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);
    void ResolveLink(CBaseObject* pOtherLink);
    void ResolveImpassableLink(CBaseObject* pOtherLink);
    void ResolveInternalAILink(CBaseObject* pOtherLink, uint32 userData);

    // Attaches the associated navigation GraphNode (if none present)
    void CreateNavGraphNode();
    // Detaches the associated navigation GraphNode (if present)
    void DeleteNavGraphNode();

    // Update links in AI nav graph.
    void UpdateNavLinks();
    float GetRadius();

    //! Ids of linked waypoints.
    struct Link
    {
        CAIPoint* object;
        GUID id;
        bool selected; // True if link is currently selected.
        bool passable;
        Link() { object = 0; id = GUID_NULL; selected = false; passable = true; }
    };
    std::vector<Link> m_links;
    // on load serialisation we get the list of links that are internal to AI but
    // can only actually update later on when the ai node has been created etc - so store
    // them in this list
    std::vector< std::pair<CAIPoint*, float> > m_internalAILinks;

    EAIPointType m_aiType;
    EAINavigationType m_aiNavigationType;
    bool m_bRemovable;

    //! True if this waypoint is indoors.
    bool m_bIndoors;
    IVisArea* m_pArea;

    //////////////////////////////////////////////////////////////////////////
    bool m_bLinksValid;
    bool m_bIgnoreUpdateLinks;

    // AI graph node.
    int m_nodeIndex;
    struct GraphNode* m_aiNode;
    bool m_bIndoorEntrance;

    static int m_rollupId;
    static class CAIPointPanel* m_panel;
    static float m_helperScale;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_AIPOINT_H
