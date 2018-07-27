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

#ifndef CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H
#define CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/HyperGraphBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

// Sandbox imports.
#include "../Editor/ViewManager.h"
#include "../Editor/Viewport.h"
#include "../Editor/Undo/IUndoManagerListener.h"

#include <QApplication>
#include <QPointer>

#include <AzCore/std/string/conversions.h>

#include <AzCore/Component/ComponentApplicationBus.h>


/**
* Integration of ToolsApplication behavior and Cry undo/redo and selection systems
* with respect to component entity operations.
*
* Undo/Redo
* - CToolsApplicationUndoLink represents a component application undo operation within
*   the Sandbox undo system. When an undo-able component operation is performed, we
*   intercept ToolsApplicationEventBus::OnBeginUndo()/OnEndUndo() events, and in turn
*   create and register a link instance.
* - When the user attempts to undo/redo a CToolsApplicationUndoLink event in Sandbox,
*   CToolsApplicationUndoLink::Undo()/Redo() is invoked, and the request is passed
*   to the component application via ToolsApplicationRequestBus::OnUndoPressed/OnRedoPressed,
*   where restoration of the previous entity snapshot is handled.
*
* AzToolsFramework::ToolsApplication Extensions
* - Provides engine UI customizations, such as using the engine's built in asset browser
*   when assigning asset references to component properties.
* - Handles component edit-time display requests (using the editor's drawing context).
* - Handles source control requests from AZ components or component-related UI.
*/

class CToolsApplicationUndoLink;

class QMenu;
class QWidget;
class CComponentEntityObject;
class CHyperGraph;
struct IFlowGraph;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetSelectionModel;
    }
}

//////////////////////////////////////////////////////////////////////////

class SandboxIntegrationManager
    : private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    , private AzToolsFramework::EditorRequests::Bus::Handler
    , private AzToolsFramework::EditorPickModeRequests::Bus::Handler
    , private AzToolsFramework::EditorEvents::Bus::Handler
    , private AzFramework::EntityDebugDisplayRequestBus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::HyperGraphRequestBus::Handler
    , private IUndoManagerListener
    , private AZ::LegacyConversion::LegacyConversionRequestBus::Handler
{
public:

    SandboxIntegrationManager();
    ~SandboxIntegrationManager();

    void Setup();

private:

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::ToolsApplicationEvents::Bus::Handler overrides
    void OnBeginUndo(const char* label) override;
    void OnEndUndo(const char* label, bool changed) override;

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorRequests::Bus::Handler overrides
    void RegisterViewPane(const char* name, const char* category, const AzToolsFramework::ViewPaneOptions& viewOptions, const WidgetCreationFunc& widgetCreationFunc) override;
    void UnregisterViewPane(const char* name) override;
    void OpenViewPane(const char* paneName) override;
    QDockWidget* InstanceViewPane(const char* paneName) override;
    void CloseViewPane(const char* paneName) override;
    void BrowseForAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel& selection) override;
    void GenerateAllCubemaps() override;
    void GenerateCubemapForEntity(AZ::EntityId entityId, AZStd::string* cubemapOutputPath, bool hideEntity) override;
    void HandleObjectModeSelection(const AZ::Vector2& point, int flags, bool& handled) override;    
    void UpdateObjectModeCursor(AZ::u32& cursorId, AZStd::string& cursorStr) override;
    void StartObjectPickMode() override;
    void StopObjectPickMode() override;
    void CreateEditorRepresentation(AZ::Entity* entity) override;
    bool DestroyEditorRepresentation(AZ::EntityId entityId, bool deleteAZEntity) override;
    void CloneSelection(bool& handled) override;
    void DeleteSelectedEntities(bool includeDescendants) override;
    AZ::EntityId CreateNewEntity(AZ::EntityId parentId = AZ::EntityId()) override;
    AZ::EntityId CreateNewEntityAsChild(AZ::EntityId parentId) override;
    AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& /*pos*/, AZ::EntityId parentId = AZ::EntityId()) override;
    QWidget* GetMainWindow() override;
    IEditor* GetEditor() override;
    void SetEditTool(const char* tool) override;
    void LaunchLuaEditor(const char* files) override;
    bool IsLevelDocumentOpen() override;
    AZStd::string GetLevelName() override;
    AZStd::string SelectResource(const AZStd::string& resourceType, const AZStd::string& previousValue) override;
    void GenerateNavigationArea(const AZStd::string& name, const AZ::Vector3& position, const AZ::Vector3* points, size_t numPoints, float height) override;
    virtual const char* GetDefaultAgentNavigationTypeName() override;
    float CalculateAgentNavigationRadius(const char* agentTypeName) override;
    void OpenPinnedInspector(const AzToolsFramework::EntityIdList& entities) override;
    void ClosePinnedInspector(AzToolsFramework::EntityPropertyEditor* editor) override;
    AZStd::vector<AZStd::string> GetAgentTypes() override;
    void GoToSelectedOrHighlightedEntitiesInViewports() override;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEvents::Bus::Handler overrides
    void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus::Handler
    void OnContextReset() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::HyperGraphRequestBus::Handler
    AZStd::string GetHyperGraphName(IFlowGraph* runtimeGraphPtr) override;
    void RegisterHyperGraphEntityListener(IFlowGraph* runtimeGraphPtr, IEntityObjectListener* listener) override;
    void UnregisterHyperGraphEntityListener(IFlowGraph* runtimeGraphPtr, IEntityObjectListener* listener) override;
    void SetHyperGraphEntity(IFlowGraph* runtimeGraphPtr, const AZ::EntityId& /*id*/) override;
    void OpenHyperGraphView(IFlowGraph* runtimeGraphPtr) override;
    void ReleaseHyperGraph(IFlowGraph* runtimeGraphPtr) override;
    void SetHyperGraphGroupName(IFlowGraph* runtimeGraphPtr, const char* name) override;
    void SetHyperGraphName(IFlowGraph* runtimeGraphPtr, const char* name) override;
    void BuildSerializedFlowGraph(IFlowGraph* flowGraph, LmbrCentral::SerializedFlowGraph& graphData) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorRequests::Bus::Handler overrides
    void SetColor(float r, float g, float b, float a) override;
    void SetColor(const AZ::Color& color) override;
    void SetColor(const AZ::Vector4& color) override;
    void SetAlpha(float a) override;
    void DrawQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) override;
    void DrawQuadGradient(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) override;
    void DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3) override;
    void DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color) override;
    void DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color) override;
    void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
    void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
    void DrawSolidOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) override;
    void DrawPoint(const AZ::Vector3& p, int nSize) override;
    void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2) override;
    void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2) override;
    void DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color) override;
    void DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled) override;
    void DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) override;
    void DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) override;
    void DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) override;
    void DrawWireCircle2d(const AZ::Vector2& center, float radius, float z) override;
    void DrawTerrainCircle(const AZ::Vector3& worldPos, float radius, float height) override;
    void DrawTerrainCircle(const AZ::Vector3& center, float radius, float angle1, float angle2, float height) override;
    void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis) override;
    void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const AZ::Vector3& fixedAxis) override;
    void DrawCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height) override;
    void DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis) override;
    void DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis) override;
    void DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) override;
    void DrawSolidCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) override;
    void DrawWireCapsule(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) override;
    void DrawTerrainRect(float x1, float y1, float x2, float y2, float height) override;
    void DrawTerrainLine(AZ::Vector3 worldPos1, AZ::Vector3 worldPos2) override;
    void DrawWireSphere(const AZ::Vector3& pos, float radius) override;
    void DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius) override;
    void DrawBall(const AZ::Vector3& pos, float radius) override;
    void DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float fHeadScale, bool b2SidedArrow) override;
    void DrawTextLabel(const AZ::Vector3& pos, float size, const char* text, const bool bCenter, int srcOffsetX, int scrOffsetY) override;
    void Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter) override;
    void DrawTextOn2DBox(const AZ::Vector3& pos, const char* text, float textScale, const AZ::Vector4& TextColor, const AZ::Vector4& TextBackColor) override;
    void DrawTextureLabel(ITexture* texture, const AZ::Vector3& pos, float sizeX, float sizeY, int texIconFlags) override;
    void SetLineWidth(float width) override;
    bool IsVisible(const AZ::Aabb& bounds) override;
    int SetFillMode(int nFillMode) override;
    float GetLineWidth() override;
    float GetAspectRatio() override;
    void DepthTestOff() override;
    void DepthTestOn() override;
    void DepthWriteOff() override;
    void DepthWriteOn() override;
    void CullOff() override;
    void CullOn() override;
    bool SetDrawInFrontMode(bool bOn) override;
    AZ::u32 GetState() override;
    AZ::u32 SetState(AZ::u32 state) override;
    AZ::u32 SetStateFlag(AZ::u32 state) override;
    AZ::u32 ClearStateFlag(AZ::u32 state) override;
    void PushMatrix(const AZ::Transform& tm) override;
    void PopMatrix() override;
    void SetDC(DisplayContext* dc) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AZ::LegacyConversion::LegacyConversionRequestBus::Handler 
    AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> CreateConvertedEntity(CBaseObject* sourceObject, bool failIfParentNotFound, const AZ::ComponentTypeList& componentsToAdd) override;
    AZ::EntityId FindCreatedEntity(const AZ::Uuid& sourceObjectUUID, const char* sourceObjectName) override;
    AZ::EntityId FindCreatedEntityByExistingObject(const CBaseObject* sourceObject) override;
    //////////////////////////////////////////////////////////////////////////

    // Context menu handlers.
    void ContextMenu_NewEntity();
    void ContextMenu_MakeSlice(AzToolsFramework::EntityIdList entities);
    void ContextMenu_InheritSlice(AzToolsFramework::EntityIdList entities);
    void ContextMenu_InstantiateSlice();
    void ContextMenu_SelectSlice();
    void ContextMenu_PushEntitiesToSlice(AzToolsFramework::EntityIdList entities,
        AZ::SliceComponent::EntityAncestorList ancestors,
        AZ::Data::AssetId targetAncestorId,
        bool affectEntireHierarchy);
    void ContextMenu_DetachSliceEntities(AzToolsFramework::EntityIdList entities);
    void ContextMenu_DetachSliceInstances(AzToolsFramework::EntityIdList entities);
    void ContextMenu_Duplicate();
    void ContextMenu_DeleteSelected();
    void ContextMenu_ResetToSliceDefaults(AzToolsFramework::EntityIdList entities);

    bool CreateFlowGraphNameDialog(AZ::EntityId entityId, AZStd::string& flowGraphName);
    void ContextMenu_NewFlowGraph(AzToolsFramework::EntityIdList entities);
    void ContextMenu_OpenFlowGraph(AZ::EntityId entityId, IFlowGraph* flowgraph);
    void ContextMenu_AddFlowGraph(AZ::EntityId entities);
    void ContextMenu_RemoveFlowGraph(AZ::EntityId entityId, IFlowGraph* flowgraph);

    void MakeSliceFromEntities(const AzToolsFramework::EntityIdList& entities, bool inheritSlices);

    void GetSelectedEntities(AzToolsFramework::EntityIdList& entities);
    void GetSelectedOrHighlightedEntities(AzToolsFramework::EntityIdList& entities);

    AZStd::string GetDefaultComponentViewportIcon() override
    {
        return m_defaultComponentViewportIconLocation;
    }

    AZStd::string GetDefaultComponentEditorIcon() override
    {
        return m_defaultComponentIconLocation;
    }

    AZStd::string GetDefaultEntityIcon() override
    {
        return m_defaultEntityIconLocation;
    }

    AZStd::string GetComponentEditorIcon(const AZ::Uuid& componentType, AZ::Component* component) override;
    AZStd::string GetComponentIconPath(const AZ::Uuid& componentType, AZ::Crc32 componentIconAttrib, AZ::Component* component) override;

    //////////////////////////////////////////////////////////////////////////
    // IUndoManagerListener
    // Listens for Cry Undo System events.
    void UndoStackFlushed() override;

private:
    void SetupFileExtensionMap();
    void SetupSliceContextMenu(QMenu* menu);
    void SetupSliceContextMenu_Push(QMenu* menu, const AzToolsFramework::EntityIdList& selectedEntities, const AZ::u32 numEntitiesInSlices);
    void SetupFlowGraphContextMenu(QMenu* menu);
    void SetupScriptCanvasContextMenu(QMenu* menu);

    //! \return whether user confirmed detach, false if cancelled
    bool ConfirmDialog_Detach(const QString& title, const QString& text);

    typedef AZStd::unordered_map<AZ::u32, IFileUtil::ECustomFileType> ExtensionMap;
    ExtensionMap m_extensionToFileType;

    AZ::Vector2 m_contextMenuViewPoint;
    AZ::Vector3 m_sliceWorldPos;

    int m_inObjectPickMode;
    short m_startedUndoRecordingNestingLevel;   // used in OnBegin/EndUndo to ensure we only accept undo's we started recording

    DisplayContext* m_dc;

    AZStd::unique_ptr<class ComponentEntityDebugPrinter> m_entityDebugPrinter;

    const AZStd::string m_defaultComponentIconLocation = "Editor/Icons/Components/Component_Placeholder.png";
    const AZStd::string m_defaultComponentViewportIconLocation = "Editor/Icons/Components/Viewport/Component_Placeholder.png";
    const AZStd::string m_defaultEntityIconLocation = "Editor/Icons/Components/Viewport/Transform.png";
};

//////////////////////////////////////////////////////////////////////////
class CToolsApplicationUndoLink
    : public IUndoObject
{
public:

    CToolsApplicationUndoLink(const char* description)
        : m_description(description)
    {
    }

    int GetSize() override
    {
        return 0;
    }

    QString GetDescription() override
    {
        return m_description.c_str();
    }

    void Undo(bool bUndo = true) override
    {
        if (bUndo)
        {
            // Restore previous focus after applying the undo.
            QPointer<QWidget> w = QApplication::focusWidget();

            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, UndoPressed);

            if (!w.isNull())
            {
                w->setFocus(Qt::OtherFocusReason);
            }
        }
    }

    void Redo() override
    {
        // Restore previous focus after applying the undo.
        QPointer<QWidget> w = QApplication::focusWidget();

        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, RedoPressed);

        if (!w.isNull())
        {
            w->setFocus(Qt::OtherFocusReason);
        }
    }

    AZStd::string m_description;
};

#endif // CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H
