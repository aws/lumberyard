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

#include "StarterGameGem_precompiled.h"
#include "PlayFromHereEditorSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <EditorDefs.h>
#include <EditTool.h>
#include <Resource.h>

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <IEditor.h>
#include <ViewManager.h>
#include <GameEngine.h>
#include <Objects/BaseObject.h>
#include <Include/IObjectManager.h>
#endif

#include <MathConversion.h>


namespace StarterGameGem
{
    void PlayFromHereEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PlayFromHereEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<PlayFromHereEditorSystemComponent>("Play From Here System", "Adds a 'Play From Here' option to the context menu.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void PlayFromHereEditorSystemComponent::Init()
    {
        m_errorFindingLocation = true;
        m_playFromHere = AZ::Vector3(0.0f, 0.0f, 0.0f);
    }

    void PlayFromHereEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        PlayFromHereEditorSystemComponentRequestBus::Handler::BusConnect();
    }

    void PlayFromHereEditorSystemComponent::Deactivate()
    {
        PlayFromHereEditorSystemComponentRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void PlayFromHereEditorSystemComponent::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        IEditor* editor;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

        CGameEngine* gameEngine = editor->GetGameEngine();
        if (!gameEngine || !gameEngine->IsLevelLoaded())
        {
            return;
        }

        m_contextMenuViewPoint = point;

        if (!(flags & AzToolsFramework::EditorEvents::eECMF_HIDE_ENTITY_CREATION))
        {
            QAction* action = menu->addAction(QObject::tr("Play From Here"));
            QObject::connect(action, &QAction::triggered, [this]() { PlayFromHere(); });
        }
#endif
    }

    void PlayFromHereEditorSystemComponent::PlayFromHere()
    {
#if defined(AZ_PLATFORM_WINDOWS)
        // Find where the user is clicking.
        IEditor* editor;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        CViewport* view = editor->GetViewManager()->GetSelectedViewport();
        if (view == nullptr)
        {
            AZ_Error("StarterGame", false, "%s: Failed because the viewport couldn't be found.", __FUNCTION__);
            return;
        }

        const QPoint viewPoint(m_contextMenuViewPoint.GetX(), m_contextMenuViewPoint.GetY());
        m_playFromHere = LYVec3ToAZVec3(view->ViewToWorld(viewPoint));

        m_errorFindingLocation = false;

        // Enter game mode.
        // The OnStartPlayInEditor() callback will trigger all the entities to move to the location
        // when play starts; this saves us having to store their position in the editor and move
        // them back when we exit game mode.
        editor->SetInGameMode(true);
#endif
    }

    void PlayFromHereEditorSystemComponent::OnStartPlayInEditor()
    {
#if defined(AZ_PLATFORM_WINDOWS)
        if (!m_errorFindingLocation)
        {
            AZ_Printf("StarterGame", "Playing from specified location: %.3f, %.3f, %.3f", m_playFromHere.GetX(), m_playFromHere.GetY(), m_playFromHere.GetZ());
            PlayFromHereEditorSystemComponentNotificationBus::Broadcast(&PlayFromHereEditorSystemComponentNotifications::OnPlayFromHere, m_playFromHere);
        }

        // This is reset here because only the 'Play From Here' option should be able to set it to
        // false and it needs to be reset everytime but the 'PlayFromHere()' function can be
        // by-passed if the user presses Ctrl+G.
        m_errorFindingLocation = true;
#endif
    }
}
