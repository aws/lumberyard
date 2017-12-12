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

// include the required headers
#include "OpenGLRenderPlugin.h"
#include "GLWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderViewWidget.h"

#include <MCore/Source/JobManager.h>
#include <MCore/Source/Job.h>
#include <MCore/Source/JobList.h>

#include <chrono>

#include <EMotionFX/Source/BlendTreeSmoothingNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>


namespace EMStudio
{
    // constructor
    OpenGLRenderPlugin::OpenGLRenderPlugin()
        : EMStudio::RenderPlugin()
    {
        mGraphicsManager = nullptr;
    }


    // destructor
    OpenGLRenderPlugin::~OpenGLRenderPlugin()
    {
        // get rid of the OpenGL graphics manager
        delete mGraphicsManager;
    }


    // init after the parent dock window has been created
    bool OpenGLRenderPlugin::Init()
    {
        MCore::LogInfo("Initializing OpenGL rendering");
        /*
            // create our OpenGL format
            QGLFormat oglFormat = QGLFormat::defaultFormat();
            oglFormat.setDoubleBuffer( true );
            oglFormat.setRgba( true );
            oglFormat.setDepth( true );
            oglFormat.setDirectRendering( true );
            oglFormat.setSampleBuffers(true);
            oglFormat.setSamples( 4 );
            QGLFormat::setDefaultFormat( oglFormat );

            if (QGLFormat::hasOpenGL() == false)
            {
                LogError( "System does not support OpenGL." );
                return false;
            }
        */
        // set the callback
        //mUpdateCallback = new GLUpdateCallBack(this);
        //EMotionFX::GetActorManager().SetCallBack( mUpdateCallback );

        RenderPlugin::Init();

        MCore::LogInfo("Render plugin initialized successfully");
        return true;
    }


    // initialize the OpenGL engine
    bool OpenGLRenderPlugin::InitializeGraphicsManager()
    {
        if (mGraphicsManager)
        {
            // initialize all already existing actors and actor instances
            ReInit();
            return true;
        }

        // get the absolute directory path where all the shaders will be located
        const MCore::String shaderPath = MysticQt::GetDataDir() + "Shaders/";

        // create graphics manager and initialize it
        mGraphicsManager = new RenderGL::GraphicsManager();
        if (mGraphicsManager->Init(shaderPath.AsChar()) == false)
        {
            MCore::LogError("Could not initialize OpenGL graphics manager.");
            //      delete mGraphicsManager;
            //      mGraphicsManager = nullptr;
            return false;
        }

        // set the render util in the base render plugin
        mRenderUtil = mGraphicsManager->GetRenderUtil();

        // initialize all already existing actors and actor instances
        ReInit();

        return true;
    }


    // overloaded version to create a OpenGL render actor
    bool OpenGLRenderPlugin::CreateEMStudioActor(EMotionFX::Actor* actor)
    {
        // check if we have already imported this actor
        if (FindEMStudioActor(actor))
        {
            MCore::LogError("The actor has already been imported.");
            return false;
        }

        MCore::String texturePath;
        if (mRenderOptions.mTexturePath.GetIsEmpty() == false)
        {
            // use the texture path specified in the render options
            texturePath = mRenderOptions.mTexturePath;
        }
        else
        {
            // extract the file path from the actor name, assuming the actor name is its full filename
            texturePath = actor->GetFileNameString().ExtractPath();
        }

        // set the automatic mip mapping creation and the skip loading textures flag
        mGraphicsManager->SetCreateMipMaps(mRenderOptions.mCreateMipMaps);
        mGraphicsManager->SetSkipLoadingTextures(mRenderOptions.mSkipLoadingTextures);

        // create a new OpenGL actor and try to initialize it
        //GetMainWindow()->GetOpenGLShareWidget()->makeCurrent();
        RenderGL::GLActor* glActor = RenderGL::GLActor::Create();
        if (glActor->Init(actor, texturePath.AsChar(), true, false) == false)
        {
            MCore::LogError("Initializing the OpenGL actor for '%s' failed.", actor->GetFileName());
            glActor->Destroy();
            return false;
        }

        // create the EMStudio OpenGL helper class, set the attributes and register it to the plugin
        OpenGLRenderPlugin::EMStudioRenderActor* emstudioActor = new EMStudioRenderActor(actor, glActor);

        // add the actor to the list and return success
        AddEMStudioActor(emstudioActor);
        return true;
    }


    // overloaded version to render a visible actor instance using OpenGL
    void OpenGLRenderPlugin::RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        // extract the RenderGL and EMStudio actors
        RenderGL::GLActor* renderActor = actorInstance->GetCustomData<RenderGL::GLActor>();
        if (renderActor == nullptr)
        {
            return;
        }

        // get the active widget & it's rendering options
        RenderViewWidget*   widget          = GetActiveViewWidget();
        RenderOptions*      renderOptions   = GetRenderOptions();

        // call the base class on render which renders everything else
        RenderPlugin::UpdateActorInstance(actorInstance, timePassedInSeconds);

        actorInstance->UpdateMeshDeformers(timePassedInSeconds);

        // solid mesh rendering
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_SOLID))
        {
            uint32 renderFlags = RenderGL::GLActor::RENDER_SKINNING;
            if (widget->GetRenderFlag(RenderViewWidget::RENDER_LIGHTING))
            {
                renderFlags |= RenderGL::GLActor::RENDER_LIGHTING;
                renderActor->SetGroundColor(renderOptions->mLightGroundColor);
                renderActor->SetSkyColor(renderOptions->mLightSkyColor);
            }
            if (widget->GetRenderFlag(RenderViewWidget::RENDER_SHADOWS))
            {
                renderFlags |= RenderGL::GLActor::RENDER_SHADOWS;
            }
            if (widget->GetRenderFlag(RenderViewWidget::RENDER_TEXTURING))
            {
                renderFlags |= RenderGL::GLActor::RENDER_TEXTURING;
            }

            renderActor->Render(actorInstance, renderFlags);
        }

        RenderPlugin::RenderActorInstance(actorInstance, timePassedInSeconds);
    }


    // overloaded version which created a OpenGL render widget
    void OpenGLRenderPlugin::CreateRenderWidget(RenderViewWidget* renderViewWidget, RenderWidget** outRenderWidget, QWidget** outWidget)
    {
        GLWidget* glWidget  = new GLWidget(renderViewWidget, this);
        *outRenderWidget    = glWidget;
        *outWidget          = glWidget;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/RenderPlugins/Source/OpenGLRender/OpenGLRenderPlugin.moc>
