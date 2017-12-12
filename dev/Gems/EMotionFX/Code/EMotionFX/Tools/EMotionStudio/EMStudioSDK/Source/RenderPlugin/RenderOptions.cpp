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
#include "RenderOptions.h"
#include <MysticQt/Source/MysticQtConfig.h>
#include <EMotionFX/Rendering/Common/OrbitCamera.h>


namespace EMStudio
{
    // constructor
    RenderOptions::RenderOptions()
    {
        // render background
        mBackgroundColor        = MCore::RGBAColor(0.359f, 0.3984f, 0.4492f);
        mGradientSourceColor    = MCore::RGBAColor(0.4941f, 0.5686f, 0.6470f);
        mGradientTargetColor    = MCore::RGBAColor(0.0941f, 0.1019f, 0.1098f);
        mWireframeColor         = MCore::RGBAColor(0.0f, 0.0f, 0.0f);
        mVertexNormalsColor     = MCore::RGBAColor(0.0f, 1.0f, 0.0f);
        mFaceNormalsColor       = MCore::RGBAColor(0.5f, 0.5f, 1.0f);
        mTangentsColor          = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        mMirroredBinormalsColor = MCore::RGBAColor(1.0f, 1.0f, 0.0f);
        mBinormalsColor         = MCore::RGBAColor(1.0f, 1.0f, 1.0f);
        mNodeAABBColor          = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        mStaticAABBColor        = MCore::RGBAColor(0.0f, 0.7f, 0.7f);
        mMeshAABBColor          = MCore::RGBAColor(0.0f, 0.0f, 0.7f);
        mCollisionMeshAABBColor = MCore::RGBAColor(0.0f, 0.7f, 0.0f);
        mCollisionMeshColor     = MCore::RGBAColor(0.0f, 1.0f, 1.0f);
        mOBBsColor              = MCore::RGBAColor(1.0f, 1.0f, 0.0f);
        mLineSkeletonColor      = MCore::RGBAColor(1.0f, 1.0f, 0.0f);
        mSkeletonColor          = MCore::RGBAColor(0.19f, 0.58f, 0.19f);
        mSelectionColor         = MCore::RGBAColor(1.0f, 1.0f, 1.0f);
        mNodeNameColor          = MCore::RGBAColor(1.0f, 1.0f, 1.0f);
        mSelectedObjectColor    = MCore::RGBAColor(1.0f, 0.647f, 0.0f);

        mGridColor              = MCore::RGBAColor(0.3242f, 0.3593f, 0.40625f);
        mMainAxisColor          = MCore::RGBAColor(0.0f, 0.01f, 0.04f);
        mSubStepColor           = MCore::RGBAColor(0.2460f, 0.2851f, 0.3320f);

        mLightSkyColor          = MCore::RGBAColor(0.635f, 0.6f, 0.572f);
        mLightGroundColor       = MCore::RGBAColor(0.117f, 0.015f, 0.07f);

        mTrajectoryArrowInnerColor  = MCore::RGBAColor(0.184f, 0.494f, 0.866f);
        mTrajectoryArrowBorderColor = MCore::RGBAColor(0.184f, 0.494f, 0.866f);

        mGridUnitSize           = 0.2f;
        mFaceNormalsScale       = 1.0f;
        mVertexNormalsScale     = 1.0f;
        mTangentsScale          = 1.0f;
        mCreateMipMaps          = true;
        mMainLightIntensity     = 1.00f;
        mMainLightAngleA        = 0.0f;
        mMainLightAngleB        = 0.0f;
        mSpecularIntensity      = 1.0f;
        mNodeOrientationScale   = 1.0f;
        mScaleBonesOnLength     = true;
        mRenderBonesOnly        = false;

        mAspiredRenderFPS       = 100;
        mSkipLoadingTextures    = false;
        mShowFPS                = false;

        // advanced render options
        mEnableAdvancedRendering = false;
        mBloomEnabled           = true;
        mBloomThreshold         = 0.8f;
        mBloomIntensity         = 0.7f;
        mBloomRadius            = 4.0f;
        mDOFEnabled             = false;
        mDOFFocalPoint          = 10.0f;
        mDOFNear                = 0.01f;
        mDOFFar                 = 100.0f;
        mDOFBlurRadius          = 2.0f;
        mRimIntensity           = 1.5f;
        mRimAngle               = 60.0f;
        mRimColor               = MCore::RGBAColor(1.0f, 0.70f, 0.109f);
        mRimWidth               = 0.65f;
        mLastUsedLayout         = "Single";

        MCommon::OrbitCamera tempCam;
        mNearClipPlaneDistance  = tempCam.GetNearClipDistance();
        mFarClipPlaneDistance   = tempCam.GetFarClipDistance();
        mFOV                    = tempCam.GetFOV();

        mRenderSelectionBox     = true;
    }



    void RenderOptions::Save(QSettings* settings)
    {
        settings->setValue("backgroundColor",          ColorToString(mBackgroundColor));
        settings->setValue("gradientSourceColor",      ColorToString(mGradientSourceColor));
        settings->setValue("gradientTargetColor",      ColorToString(mGradientTargetColor));
        settings->setValue("wireframeColor",           ColorToString(mWireframeColor));
        settings->setValue("vertexNormalsColor",       ColorToString(mVertexNormalsColor));
        settings->setValue("faceNormalsColor",         ColorToString(mFaceNormalsColor));
        settings->setValue("tangentsColor",            ColorToString(mTangentsColor));
        settings->setValue("mirroredBinormalsColor",   ColorToString(mMirroredBinormalsColor));
        settings->setValue("binormalsColor",           ColorToString(mBinormalsColor));
        settings->setValue("nodeAABBColor",            ColorToString(mNodeAABBColor));
        settings->setValue("staticAABBColor",          ColorToString(mStaticAABBColor));
        settings->setValue("meshAABBColor",            ColorToString(mMeshAABBColor));
        settings->setValue("collisionMeshAABBColor",   ColorToString(mCollisionMeshAABBColor));
        settings->setValue("collisionMeshColor",       ColorToString(mCollisionMeshColor));
        settings->setValue("OBBsColor",                ColorToString(mOBBsColor));
        settings->setValue("lineSkeletonColor",        ColorToString(mLineSkeletonColor));
        settings->setValue("skeletonColor",            ColorToString(mSkeletonColor));
        settings->setValue("selectionColor",           ColorToString(mSelectionColor));
        settings->setValue("nodeNameColor",            ColorToString(mNodeNameColor));

        settings->setValue("gridColor",                ColorToString(mGridColor));
        settings->setValue("gridMainAxisColor",        ColorToString(mMainAxisColor));
        settings->setValue("gridSubStepColor",         ColorToString(mSubStepColor));

        settings->setValue("lightSkyColor",            ColorToString(mLightSkyColor));
        settings->setValue("lightGroundColor",         ColorToString(mLightGroundColor));
        settings->setValue("rimColor",                 ColorToString(mRimColor));

        settings->setValue("trajectoryArrowInnerColor", ColorToString(mTrajectoryArrowInnerColor));
        settings->setValue("trajectoryArrowBorderColor", ColorToString(mTrajectoryArrowBorderColor));

        settings->setValue("gridUnitSize",             (double)mGridUnitSize);
        settings->setValue("faceNormalsScale",         (double)mFaceNormalsScale);
        settings->setValue("vertexNormalsScale",       (double)mVertexNormalsScale);
        settings->setValue("tangentsScale",            (double)mTangentsScale);
        settings->setValue("nearClipPlaneDistance",    (double)mNearClipPlaneDistance);
        settings->setValue("farClipPlaneDistance",     (double)mFarClipPlaneDistance);
        settings->setValue("fieldOfView",              (double)mFOV);
        settings->setValue("showFPS",                  mShowFPS);

        settings->setValue("texturePath",              mTexturePath.AsChar());
        settings->setValue("autoCreateMipMaps",        mCreateMipMaps);

        settings->setValue("lastUsedLayout",           mLastUsedLayout.AsChar());

        settings->setValue("aspiredRenderFPS",         mAspiredRenderFPS);
        settings->setValue("skipLoadingTextures",      mSkipLoadingTextures);

        settings->setValue("nodeOrientationScale",     (double)mNodeOrientationScale);
        settings->setValue("scaleBonesOnLength",       mScaleBonesOnLength);
        settings->setValue("renderBonesOnly",          mRenderBonesOnly);

        settings->setValue("mainLightIntensity",       (double)mMainLightIntensity);
        settings->setValue("mainLightAngleA",          (double)mMainLightAngleA);
        settings->setValue("mainLightAngleB",          (double)mMainLightAngleB);
        settings->setValue("specularIntensity",        (double)mSpecularIntensity);
        /*
            settings->setValue( "enableAdvancedRendering",  mEnableAdvancedRendering );
            settings->setValue( "bloomEnabled",             mBloomEnabled);
            settings->setValue( "bloomIntensity",           (double)mBloomIntensity );
            settings->setValue( "bloomThreshold",           (double)mBloomThreshold );
            settings->setValue( "bloomRadius",              (double)mBloomRadius );

            settings->setValue( "dofEnabled",               mDOFEnabled);
            settings->setValue( "dofFocusDist",             (double)mDOFFocalPoint );
            settings->setValue( "dofNear",                  (double)mDOFNear );
            settings->setValue( "dofFar",                   (double)mDOFFar);
            settings->setValue( "dofBlurRadius",            (double)mDOFBlurRadius);
        */
        settings->setValue("rimIntensity",             (double)mRimIntensity);
        settings->setValue("rimAngle",                 (double)mRimAngle);
        settings->setValue("rimWidth",                 (double)mRimWidth);

        settings->setValue("renderSelectionBox",   mRenderSelectionBox);
    }


    void RenderOptions::Load(QSettings* settings)
    {
        mLastUsedLayout         = FromQtString(settings->value("lastUsedLayout",          mLastUsedLayout.AsChar()).toString());
        mBackgroundColor        = StringToColor(settings->value("backgroundColor",         ColorToString(mBackgroundColor)).toString());
        mGradientSourceColor    = StringToColor(settings->value("gradientSourceColor",     ColorToString(mGradientSourceColor)).toString());
        mGradientTargetColor    = StringToColor(settings->value("gradientTargetColor",     ColorToString(mGradientTargetColor)).toString());
        mWireframeColor         = StringToColor(settings->value("wireframeColor",          ColorToString(mWireframeColor)).toString());
        mVertexNormalsColor     = StringToColor(settings->value("vertexNormalsColor",      ColorToString(mVertexNormalsColor)).toString());
        mFaceNormalsColor       = StringToColor(settings->value("faceNormalsColor",        ColorToString(mFaceNormalsColor)).toString());
        mTangentsColor          = StringToColor(settings->value("tangentsColor",           ColorToString(mTangentsColor)).toString());
        mMirroredBinormalsColor = StringToColor(settings->value("mirroredBinormalsColor",  ColorToString(mMirroredBinormalsColor)).toString());
        mBinormalsColor         = StringToColor(settings->value("binormalsColor",          ColorToString(mBinormalsColor)).toString());
        mNodeAABBColor          = StringToColor(settings->value("nodeAABBColor",           ColorToString(mNodeAABBColor)).toString());
        mStaticAABBColor        = StringToColor(settings->value("staticAABBColor",         ColorToString(mStaticAABBColor)).toString());
        mMeshAABBColor          = StringToColor(settings->value("meshAABBColor",           ColorToString(mMeshAABBColor)).toString());
        mCollisionMeshAABBColor = StringToColor(settings->value("collisionMeshAABBColor",  ColorToString(mCollisionMeshAABBColor)).toString());
        mCollisionMeshColor     = StringToColor(settings->value("collisionMeshColor",      ColorToString(mCollisionMeshColor)).toString());
        mOBBsColor              = StringToColor(settings->value("OBBsColor",               ColorToString(mOBBsColor)).toString());
        mLineSkeletonColor      = StringToColor(settings->value("lineSkeletonColor",       ColorToString(mLineSkeletonColor)).toString());
        mSkeletonColor          = StringToColor(settings->value("skeletonColor",           ColorToString(mSkeletonColor)).toString());
        mSelectionColor         = StringToColor(settings->value("selectionColor",          ColorToString(mSelectionColor)).toString());
        mNodeNameColor          = StringToColor(settings->value("nodeNameColor",           ColorToString(mNodeNameColor)).toString());
        mRimColor               = StringToColor(settings->value("rimColor",                ColorToString(mRimColor)).toString());

        mTrajectoryArrowInnerColor      = StringToColor(settings->value("trajectoryArrowInnerColor",           ColorToString(mTrajectoryArrowInnerColor)).toString());
        mTrajectoryArrowBorderColor     = StringToColor(settings->value("trajectoryArrowBorderColor",          ColorToString(mTrajectoryArrowBorderColor)).toString());

        mGridColor              = StringToColor(settings->value("gridColor",               ColorToString(mGridColor)).toString());
        mMainAxisColor          = StringToColor(settings->value("gridMainAxisColor",       ColorToString(mMainAxisColor)).toString());
        mSubStepColor           = StringToColor(settings->value("gridSubStepColor",        ColorToString(mSubStepColor)).toString());

        mLightSkyColor          = StringToColor(settings->value("lightSkyColor",           ColorToString(mLightSkyColor)).toString());
        mLightGroundColor       = StringToColor(settings->value("lightGroundColor",        ColorToString(mLightGroundColor)).toString());

        mTexturePath            = FromQtString(settings->value("texturePath",              mTexturePath.AsChar()).toString());
        mCreateMipMaps          = settings->value("autoCreateMipMaps",                      mCreateMipMaps).toBool();

        mAspiredRenderFPS       = settings->value("aspiredRenderFPS",                       mAspiredRenderFPS).toInt();
        mSkipLoadingTextures    = settings->value("skipLoadingTextures",                    mSkipLoadingTextures).toBool();

        mShowFPS                = settings->value("showFPS",                                mShowFPS).toBool();

        mGridUnitSize           = (float)settings->value("gridUnitSize",                    (double)mGridUnitSize).toDouble();
        mFaceNormalsScale       = (float)settings->value("faceNormalsScale",                (double)mFaceNormalsScale).toDouble();
        mVertexNormalsScale     = (float)settings->value("vertexNormalsScale",              (double)mVertexNormalsScale).toDouble();
        mTangentsScale          = (float)settings->value("tangentsScale",                   (double)mTangentsScale).toDouble();

        mNearClipPlaneDistance  = (float)settings->value("nearClipPlaneDistance",           (double)mNearClipPlaneDistance).toDouble();
        mFarClipPlaneDistance   = (float)settings->value("farClipPlaneDistance",            (double)mFarClipPlaneDistance).toDouble();
        mFOV                    = (float)settings->value("fieldOfView",                     (double)mFOV).toDouble();

        mMainLightIntensity     = (float)settings->value("mainLightIntensity",              (double)mMainLightIntensity).toDouble();
        mMainLightAngleA        = (float)settings->value("mainLightAngleA",                 (double)mMainLightAngleA).toDouble();
        mMainLightAngleB        = (float)settings->value("mainLightAngleB",                 (double)mMainLightAngleB).toDouble();
        mSpecularIntensity      = (float)settings->value("specularIntensity",               (double)mSpecularIntensity).toDouble();

        mNodeOrientationScale   = (float)settings->value("nodeOrientationScale",            (double)mNodeOrientationScale).toDouble();
        mScaleBonesOnLength     = settings->value("scaleBonesOnLength",                     mScaleBonesOnLength).toBool();
        mRenderBonesOnly        = settings->value("renderBonesOnly",                        mRenderBonesOnly).toBool();
        /*
            mEnableAdvancedRendering= settings->value("enableAdvancedRendering",                mEnableAdvancedRendering).toBool();
            mBloomEnabled           = settings->value("bloomEnabled",                           mBloomEnabled).toBool();
            mBloomIntensity         = (float)settings->value("bloomIntensity",                  (double)mBloomIntensity).toDouble();
            mBloomThreshold         = (float)settings->value("bloomThreshold",                  (double)mBloomThreshold).toDouble();
            mBloomRadius            = (float)settings->value("bloomRadius",                     (double)mBloomRadius).toDouble();
        */

        /*  mDOFEnabled             = settings->value("dofEnabled",                             mDOFEnabled).toBool();
            mDOFFocalPoint          = (float)settings->value("dofFocusDist",                    (double)mDOFFocalPoint).toDouble();
            mDOFNear                = (float)settings->value("dofNear",                         (double)mDOFNear).toDouble();
            mDOFFar                 = (float)settings->value("dofFar",                          (double)mDOFFar).toDouble();
            mDOFBlurRadius          = (float)settings->value("dofBlurRadius",                   (double)mDOFBlurRadius).toDouble();*/

        mRimIntensity           = (float)settings->value("rimIntensity",                    (double)mRimIntensity).toDouble();
        mRimAngle               = (float)settings->value("rimAngle",                        (double)mRimAngle).toDouble();
        mRimWidth               = (float)settings->value("rimWidth",                        (double)mRimWidth).toDouble();

        mRenderSelectionBox = settings->value("renderSelectionBox",                         mRenderSelectionBox).toBool();
    }
} // namespace EMStudio
