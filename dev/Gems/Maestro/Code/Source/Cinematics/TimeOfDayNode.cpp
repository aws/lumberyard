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

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "TimeOfDayNode.h"
#include "TimeOfDayNodeDescription.h"
#include "AnimSplineTrack.h"
#include "CompoundSplineTrack.h"
#include "BoolTrack.h"
#include "IPostEffectGroup.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimValueType.h"
#include "IGameFramework.h"
#include "ILevelSystem.h"
#include <I3DEngine.h>
#include <ITimeOfDay.h>
#include "Maestro/MaestroBus.h"

//////////////////////////////////////////////////////////////////////////
CTODNodeDescription::CControlParamBase::CControlParamBase() {}

CTODNodeDescription::CControlParamBase::~CControlParamBase() {}

//////////////////////////////////////////////////////////////////////////
inline void AssertMissingSetDefault()
{
    AZ_Assert(false, "Missing SetDefault template specialization")
}

inline void AssertMissingGetDefault()
{
    AZ_Assert(false, "Missing GetDefault template specialization")
}


template<typename T>
void CTODNodeDescription::TControlParam<T>::SetDefault(float val)
{
    AssertMissingSetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::SetDefault(bool val)
{
    AssertMissingSetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::SetDefault(Vec4 val)
{
    AssertMissingSetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::SetDefault(Vec3 val)
{
    AssertMissingSetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::GetDefault(float& val) const
{
    AssertMissingGetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::GetDefault(bool& val) const
{
    AssertMissingGetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::GetDefault(Vec4& val) const
{
    AssertMissingGetDefault();
}

template<typename T>
void CTODNodeDescription::TControlParam<T>::GetDefault(Vec3& val) const
{
    AssertMissingGetDefault();
}

template<typename T>
CTODNodeDescription::TControlParam<T>::~TControlParam() {}

//////////////////////////////////////////////////////////////////////////
CTODNodeDescription::CTODNodeDescription() {}

template<typename T>
void CTODNodeDescription::AddSupportedParam(const char* name, AnimValueType valueType, ITimeOfDay::ETimeOfDayParamID paramId, T defaultValue)
{
    CAnimNode::SParamInfo param;
    param.name = name;
    param.paramType = static_cast<AnimParamType>(static_cast<int>(AnimParamType::User) + static_cast<int>(m_nodeParams.size()));
    param.valueType = valueType;
    m_nodeParams.push_back(param);

    TControlParam<T>* control = new TControlParam<T>;
    control->m_paramId = paramId;
    control->SetDefault(defaultValue);

    m_controlParams.push_back(control);
}

//-----------------------------------------------------------------------------
template<>
void CTODNodeDescription::TControlParam<float>::SetDefault(float val)
{
    m_defaultValue = val;
}
template<>
void CTODNodeDescription::TControlParam<bool>::SetDefault(bool val)
{
    m_defaultValue = val;
}
template<>
void CTODNodeDescription::TControlParam<Vec4>::SetDefault(Vec4 val)
{
    m_defaultValue = val;
}
template<>
void CTODNodeDescription::TControlParam<Vec3>::SetDefault(Vec3 val)
{
    m_defaultValue = val;
}
template<>
void CTODNodeDescription::TControlParam<float>::GetDefault(float& val) const
{
    val = m_defaultValue;
}
template<>
void CTODNodeDescription::TControlParam<bool>::GetDefault(bool& val) const
{
    val = m_defaultValue;
}
template<>
void CTODNodeDescription::TControlParam<Vec4>::GetDefault(Vec4& val) const
{
    val = m_defaultValue;
}
template<>
void CTODNodeDescription::TControlParam<Vec3>::GetDefault(Vec3& val) const
{
    val = m_defaultValue;
}

//-----------------------------------------------------------------------------
StaticInstance<CAnimTODNode::TODNodeDescriptionMap> CAnimTODNode::s_TODNodeDescriptions;
bool CAnimTODNode::s_initialized = false;

//-----------------------------------------------------------------------------
CAnimTODNode::CAnimTODNode()
    : CAnimTODNode(0, AnimNodeType::Invalid, nullptr)
{
}

//-----------------------------------------------------------------------------
CAnimTODNode::CAnimTODNode(const int id, AnimNodeType nodeType, CTODNodeDescription* desc)
    : CAnimNode(id, nodeType)
    , m_description(desc)
{
}

//-----------------------------------------------------------------------------
void CAnimTODNode::Initialize()
{
    if (!s_initialized)
    {
        s_initialized = true;
        const float recip255 = 1.0f / 255.0f;
        //////////////////////////////////////////////////////////////////////////
        //! Sun
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_Sun] = desc;
            desc->m_nodeParams.reserve(3);
            desc->m_controlParams.reserve(3);
            desc->AddSupportedParam<Vec3>("Sun Color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_COLOR, Vec3(255.0f * recip255, 248.0f * recip255, 248.0f * recip255));
            desc->AddSupportedParam<float>("Sun Intensity (lux)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_INTENSITY, 119000.0f);
            desc->AddSupportedParam<float>("Sun Specular Multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_SPECULAR_MULTIPLIER, 1.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Fog
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_Fog] = desc;
            desc->m_nodeParams.reserve(20);
            desc->m_controlParams.reserve(20);
            desc->AddSupportedParam<Vec3>("Color (bottom)", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_FOG_COLOR, Vec3());
            desc->AddSupportedParam<float>("Color (bottom) multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_FOG_COLOR_MULTIPLIER, 0.f);
            desc->AddSupportedParam<float>("Height (bottom)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_HEIGHT, 0.0f);
            desc->AddSupportedParam<float>("Density (bottom)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_DENSITY, 1.0f);
            desc->AddSupportedParam<Vec3>("Color (top)", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_FOG_COLOR2, Vec3());
            desc->AddSupportedParam<float>("Color (top) multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_FOG_COLOR2_MULTIPLIER, 0.0f);
            desc->AddSupportedParam<float>("Height (top)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_HEIGHT2, 4000.0f);
            desc->AddSupportedParam<float>("Density (top)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_DENSITY2, 0.0f);
            desc->AddSupportedParam<float>("Color height offset", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_HEIGHT_OFFSET, 0.0f);
            desc->AddSupportedParam<Vec3>("Color (radial)", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_FOG_RADIAL_COLOR, Vec3());
            desc->AddSupportedParam<float>("Color (radial) multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_FOG_RADIAL_COLOR_MULTIPLIER, 0.0f);
            desc->AddSupportedParam<float>("Radial size", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_RADIAL_SIZE, 0.75f);
            desc->AddSupportedParam<float>("Radial lobe", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_RADIAL_LOBE, 0.5f);
            desc->AddSupportedParam<float>("Final density clamp", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_FINAL_DENSITY_CLAMP, 0.02f);
            desc->AddSupportedParam<float>("Global density", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_GLOBAL_DENSITY, 1.0f);
            desc->AddSupportedParam<float>("Ramp start", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_RAMP_START, 0.0f);
            desc->AddSupportedParam<float>("Ramp end", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_RAMP_END, 100.0f);
            desc->AddSupportedParam<float>("Ramp influence", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_RAMP_INFLUENCE, 0.0f);
            desc->AddSupportedParam<float>("Shadow darkening", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_SHADOW_DARKENING, 0.25f);
            desc->AddSupportedParam<float>("Shadow darkening sun", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG_SHADOW_DARKENING_SUN, 1.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Volumetric Fog
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_VolumetricFog] = desc;
            desc->m_nodeParams.reserve(20);
            desc->m_controlParams.reserve(20);
            desc->AddSupportedParam<float>("Height (bottom)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_HEIGHT, 0.0f);
            desc->AddSupportedParam<float>("Density (bottom)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_DENSITY, 1.0f);
            desc->AddSupportedParam<float>("Height (top)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_HEIGHT2, 4000.0f);
            desc->AddSupportedParam<float>("Density (top)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_DENSITY2, 0.0001f);
            desc->AddSupportedParam<float>("Global density", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_GLOBAL_DENSITY, 0.1f);
            desc->AddSupportedParam<float>("Ramp start", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_RAMP_START, 0.0f);
            desc->AddSupportedParam<float>("Ramp end", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_RAMP_END, 0.0f);
            desc->AddSupportedParam<Vec3>("Color (atmosphere)", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_COLOR1, Vec3(1.0f, 1.0f, 1.0f));
            desc->AddSupportedParam<float>("Anisotropy (atmosphere)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_ANISOTROPIC1, 0.2f);
            desc->AddSupportedParam<Vec3>("Color (sun radial)", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_COLOR2, Vec3(1.0f, 1.0f, 1.0f));
            desc->AddSupportedParam<float>("Anisotropy (sun radial)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_ANISOTROPIC2, 0.95f);
            desc->AddSupportedParam<float>("Radial blend factor", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_BLEND_FACTOR, 1.0f);
            desc->AddSupportedParam<float>("Radial blend mode", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_BLEND_MODE, 0.0f);
            desc->AddSupportedParam<Vec3>("Color (entities)", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_COLOR, Vec3(1.0f, 1.0f, 1.0f));
            desc->AddSupportedParam<float>("Anisotropy (entities)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_ANISOTROPIC, 0.6f);
            desc->AddSupportedParam<float>("Range", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_RANGE, 64.0f);
            desc->AddSupportedParam<float>("In-scattering", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_INSCATTER, 1.0f);
            desc->AddSupportedParam<float>("Extinction", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_EXTINCTION, 0.3f);
            desc->AddSupportedParam<float>("Analitical fog visibility", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_GLOBAL_FOG_VISIBILITY, 0.5f);
            desc->AddSupportedParam<float>("Final density clamp", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_VOLFOG2_FINAL_DENSITY_CLAMP, 1.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Sky Light
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_SkyLight] = desc;
            desc->m_nodeParams.reserve(8);
            desc->m_controlParams.reserve(8);
            desc->AddSupportedParam<Vec3>("Sun intensity", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_SUN_INTENSITY, Vec3(1.0f, 1.0f, 1.0f));
            desc->AddSupportedParam<float>("Sun intensity multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_SUN_INTENSITY_MULTIPLIER, 50.0f);
            desc->AddSupportedParam<float>("Mie scattering", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_KM, 4.8f);
            desc->AddSupportedParam<float>("Rayleigh scattering", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_KR, 2.0f);
            desc->AddSupportedParam<float>("Sun anisotropy factor", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_G, -0.997f);
            desc->AddSupportedParam<float>("Wavelength (R)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_WAVELENGTH_R, 694.0f);
            desc->AddSupportedParam<float>("Wavelength (G)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_WAVELENGTH_G, 597.0f);
            desc->AddSupportedParam<float>("Wavelength (B)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYLIGHT_WAVELENGTH_B, 488.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Night Sky
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_NightSky] = desc;
            desc->m_nodeParams.reserve(9);
            desc->m_controlParams.reserve(9);
            desc->AddSupportedParam<Vec3>("Horizon color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_HORIZON_COLOR, Vec3(222.0f * recip255, 148.0f * recip255, 47.0f * recip255));
            desc->AddSupportedParam<Vec3>("Zenith color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_ZENITH_COLOR, Vec3(17.0f * recip255, 38.0f * recip255, 78.0f * recip255));
            desc->AddSupportedParam<float>("Zenith shift", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_ZENITH_SHIFT, 0.25f);
            desc->AddSupportedParam<float>("Star intensity", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_START_INTENSITY, 0.01f);
            desc->AddSupportedParam<Vec3>("Moon color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_COLOR, Vec3(255.0f * recip255, 255.0f * recip255, 255.0f * recip255));
            desc->AddSupportedParam<Vec3>("Moon inner corona color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_INNERCORONA_COLOR, Vec3(230.0f * recip255, 255.0f * recip255, 255.0f * recip255));
            desc->AddSupportedParam<float>("Moon inner corona scale", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_INNERCORONA_SCALE, 0.499f);
            desc->AddSupportedParam<Vec3>("Moon outer corona color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR, Vec3(128.0f * recip255, 200.0f * recip255, 255.0f * recip255));
            desc->AddSupportedParam<float>("Moon outer corona scale", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_OUTERCORONA_SCALE, 0.006f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Night Sky Multiplier
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_NightSkyMultiplier] = desc;
            desc->m_nodeParams.reserve(5);
            desc->m_controlParams.reserve(5);
            desc->AddSupportedParam<float>("Horizon color", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_HORIZON_COLOR_MULTIPLIER, 0.0001f);
            desc->AddSupportedParam<float>("Zenith color", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_ZENITH_COLOR_MULTIPLIER, 0.00002f);
            desc->AddSupportedParam<float>("Moon color", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_COLOR_MULTIPLIER, 0.01f);
            desc->AddSupportedParam<float>("Moon inner corona color", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_INNERCORONA_COLOR_MULTIPLIER, 0.0001f);
            desc->AddSupportedParam<float>("Moon outer corona color", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_NIGHSKY_MOON_OUTERCORONA_COLOR_MULTIPLIER, 0.00005f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Cloud Shading
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_CloudShading] = desc;
            desc->m_nodeParams.reserve(4);
            desc->m_controlParams.reserve(4);
            desc->AddSupportedParam<float>("Sun contribution", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_CLOUDSHADING_SUNLIGHT_MULTIPLIER, 1.96f);
            desc->AddSupportedParam<Vec3>("Sun custom color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR, Vec3(215.0f * recip255, 200.0f * recip255, 170.0f * recip255));
            desc->AddSupportedParam<float>("Sun custom color multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR_MULTIPLIER, 1.0f);
            desc->AddSupportedParam<float>("Sun custom color influence", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_CLOUDSHADING_SUNLIGHT_CUSTOM_COLOR_INFLUENCE, 0.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Sun Rays Effect
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_SunRaysEffect] = desc;
            desc->m_nodeParams.reserve(5);
            desc->m_controlParams.reserve(5);
            desc->AddSupportedParam<float>("Sun shafts visibility", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_SHAFTS_VISIBILITY, 0.25f);
            desc->AddSupportedParam<float>("Sun rays visibility", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_RAYS_VISIBILITY, 1.0f);
            desc->AddSupportedParam<float>("Sun rays attenuation", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_RAYS_ATTENUATION, 5.0f);
            desc->AddSupportedParam<float>("Sun rays suncolor influence", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_RAYS_SUNCOLORINFLUENCE, 1.0f);
            desc->AddSupportedParam<Vec3>("Sun rays custom color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_RAYS_CUSTOMCOLOR, Vec3(1.0f, 1.0f, 1.0f));
        }
        //////////////////////////////////////////////////////////////////////////
        //! Advanced
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_AdvancedTOD] = desc;
            desc->m_nodeParams.reserve(4);
            desc->m_controlParams.reserve(4);
            desc->AddSupportedParam<Vec3>("Ocean fog color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_OCEANFOG_COLOR, Vec3(29.0f * recip255, 102.0f * recip255, 141.0f * recip255));
            desc->AddSupportedParam<float>("Ocean fog color multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_OCEANFOG_COLOR_MULTIPLIER, 1.0f);
            desc->AddSupportedParam<float>("Ocean fog density", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_OCEANFOG_DENSITY, 0.2f);
            desc->AddSupportedParam<float>("Skybox multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SKYBOX_MULTIPLIER, 1.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Filters
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_Filters] = desc;
            desc->m_nodeParams.reserve(3);
            desc->m_controlParams.reserve(3);
            desc->AddSupportedParam<float>("Grain", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_COLORGRADING_FILTERS_GRAIN, 0.0f);
            desc->AddSupportedParam<Vec3>("Photofilter color", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR, Vec3(0.952f, 0.517f, 0.09f));
            desc->AddSupportedParam<float>("Photofilter density", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY, 0.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Depth of Field
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_DepthOfField] = desc;
            desc->m_nodeParams.reserve(2);
            desc->m_controlParams.reserve(2);
            desc->AddSupportedParam<float>("Focus range", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_COLORGRADING_DOF_FOCUSRANGE, 1000.0f);
            desc->AddSupportedParam<float>("Blur amount", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_COLORGRADING_DOF_BLURAMOUNT, 0.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Shadows
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_Shadows] = desc;
            desc->m_nodeParams.reserve(17);
            desc->m_controlParams.reserve(17);
            desc->AddSupportedParam<float>("Cascade 0: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC0_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 0: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC0_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 1: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC1_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 1: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC1_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 2: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC2_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 2: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC2_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 3: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC3_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 3: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC3_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 4: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC4_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 4: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC4_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 5: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC5_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 5: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC5_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 6: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC6_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 6: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC6_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 7: Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC7_BIAS, 0.f);
            desc->AddSupportedParam<float>("Cascade 7: Slope Bias", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOWSC7_SLOPE_BIAS, 0.f);
            desc->AddSupportedParam<float>("Shadow jittering", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SHADOW_JITTERING, 2.5f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! Obsolete
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_Obsolete] = desc;
            desc->m_nodeParams.reserve(3);
            desc->m_controlParams.reserve(3);
            desc->AddSupportedParam<float>("HDR dynamic power factor", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_DYNAMIC_POWER_FACTOR, 0.0f);
            desc->AddSupportedParam<float>("Sky brightening (terrain occlusion)", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_TERRAIN_OCCL_MULTIPLIER, 0.3f);
            desc->AddSupportedParam<float>("Sun color multiplier", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_SUN_COLOR_MULTIPLIER, 1.0f);
        }
        //////////////////////////////////////////////////////////////////////////
        //! HDR
        {
            CTODNodeDescription* desc = new CTODNodeDescription();
            s_TODNodeDescriptions[AnimNodeType::TOD_HDR] = desc;
            desc->m_nodeParams.reserve(13);
            desc->m_controlParams.reserve(13);
            desc->AddSupportedParam<float>("Film curve shoulder scale", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_FILMCURVE_SHOULDER_SCALE, 1.0f);
            desc->AddSupportedParam<float>("Film curve midtones scale", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_FILMCURVE_LINEAR_SCALE, 1.0f);
            desc->AddSupportedParam<float>("Film curve toe scale", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_FILMCURVE_TOE_SCALE, 1.0f);
            desc->AddSupportedParam<float>("Film curve whitepoint", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_FILMCURVE_WHITEPOINT, 1.0f);
            desc->AddSupportedParam<float>("Saturation", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_COLORGRADING_COLOR_SATURATION, 1.0f);
            desc->AddSupportedParam<Vec3>("Color balance", AnimValueType::RGB, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_COLORGRADING_COLOR_BALANCE, Vec3(1.0f, 1.0f, 1.0f));
            desc->AddSupportedParam<float>("(Dep) Scene key", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_EYEADAPTATION_SCENEKEY, 0.18f);
            desc->AddSupportedParam<float>("(Dep) Min exposure", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_EYEADAPTATION_MIN_EXPOSURE, 0.36f);
            desc->AddSupportedParam<float>("(Dep) Max exposure", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_EYEADAPTATION_MAX_EXPOSURE, 2.8f);
            desc->AddSupportedParam<float>("EV Min", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_EYEADAPTATION_EV_MIN, 4.5f);
            desc->AddSupportedParam<float>("EV Max", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_EYEADAPTATION_EV_MAX, 17.0f);
            desc->AddSupportedParam<float>("EV Auto compensation", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_EYEADAPTATION_EV_AUTO_COMPENSATION, 1.5f);
            desc->AddSupportedParam<float>("Bloom amount", AnimValueType::Float, ITimeOfDay::ETimeOfDayParamID::PARAM_HDR_BLOOM_AMOUNT, 0.11f);
        }
    }
}

//-----------------------------------------------------------------------------
CTODNodeDescription* CAnimTODNode::GetTODNodeDescription(AnimNodeType nodeType)
{
    CTODNodeDescription* retDescription = nullptr;

    CAnimTODNode::Initialize();

    TODNodeDescriptionMap::iterator itr = s_TODNodeDescriptions.find(nodeType);

    if (itr != s_TODNodeDescriptions.end())
    {
        retDescription = itr->second;
    }

    return retDescription;
}

//-----------------------------------------------------------------------------
CAnimNode* CAnimTODNode::CreateNode(const int id, AnimNodeType nodeType)
{
    CAnimNode* retNode = nullptr;

    CTODNodeDescription* desc = GetTODNodeDescription(nodeType);

    if (desc)
    {
        retNode = aznew CAnimTODNode(id, nodeType, desc);
        static_cast<CAnimTODNode*>(retNode)->m_nodeType = nodeType;
    }

    return retNode;
}

//-----------------------------------------------------------------------------
void CAnimTODNode::InitPostLoad(IAnimSequence* sequence)
{
    CAnimNode::InitPostLoad(sequence);

    // For AZ::Serialization, m_nodeType will have be reflected and deserialized. Find the appropriate FXNodeDescription for it
    // and store the pointer
    m_description = GetTODNodeDescription(m_nodeType);
    if (!m_description)
    {
        // This is not ideal - we should never get here unless someone is tampering with data. We can't remove the node at this point,
        // we can't use a defatult description without crashing later, so we simply assert.
        AZ_Assert(false, "Unrecognized Time of Day nodeType in Track View node %s. Please remove this node from the sequence.", m_name.c_str());
    }
}

//-----------------------------------------------------------------------------
void CAnimTODNode::SerializeAnims(XmlNodeRef& xmlNode, bool loading, bool loadEmptyTracks)
{
    if (loading)
    {
        int paramIdVersion = 0;
        xmlNode->getAttr("ParamIdVersion", paramIdVersion);

        // Fix old param types
        if (paramIdVersion <= 2)
        {
            int numChildren = xmlNode->getChildCount();
            for (int childIndex = 0; childIndex < numChildren; ++childIndex)
            {
                XmlNodeRef trackNode = xmlNode->getChild(childIndex);

                CAnimParamType paramType;
                paramType.Serialize(trackNode, true);
                // Don't use APARAM_USER because it could change in newer versions
                // CAnimNode::SerializeAnims will then take care of that
                static const unsigned int OLD_APARAM_USER = 100;
                paramType = static_cast<AnimParamType>(static_cast<int>(paramType.GetType()) + OLD_APARAM_USER);
                paramType.Serialize(trackNode, false);
            }
        }
    }

    CAnimNode::SerializeAnims(xmlNode, loading, loadEmptyTracks);
}

//-----------------------------------------------------------------------------
unsigned int CAnimTODNode::GetParamCount() const
{
    return m_description->m_nodeParams.size();
}

//-----------------------------------------------------------------------------
CAnimParamType CAnimTODNode::GetParamType(unsigned int paramIndex) const
{
    if (paramIndex >= 0 && paramIndex < (unsigned int)m_description->m_nodeParams.size())
    {
        return m_description->m_nodeParams[paramIndex].paramType;
    }

    return AnimParamType::Invalid;
}

//-----------------------------------------------------------------------------
bool CAnimTODNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (size_t paramIndex = 0; paramIndex < m_description->m_nodeParams.size(); ++paramIndex)
    {
        if (m_description->m_nodeParams[paramIndex].paramType == paramId)
        {
            info = m_description->m_nodeParams[paramIndex];
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void CAnimTODNode::CreateDefaultTracks()
{
    for (size_t paramIndex = 0; paramIndex < m_description->m_nodeParams.size(); ++paramIndex)
    {
        IAnimTrack* track = CreateTrackInternal(m_description->m_nodeParams[paramIndex].paramType,
            eAnimCurveType_BezierFloat, m_description->m_nodeParams[paramIndex].valueType);

        //Setup default value
        AnimValueType valueType = m_description->m_nodeParams[paramIndex].valueType;
        if (valueType == AnimValueType::Float)
        {
            C2DSplineTrack* floatTrack = static_cast<C2DSplineTrack*>(track);
            float val(0);
            m_description->m_controlParams[paramIndex]->GetDefault(val);
            floatTrack->SetDefaultValue(Vec2(0, val));
        }
        else if (valueType == AnimValueType::Bool)
        {
            CBoolTrack* boolTrack = static_cast<CBoolTrack*>(track);
            bool val = false;
            m_description->m_controlParams[paramIndex]->GetDefault(val);
            boolTrack->SetDefaultValue(val);
        }
        else if (valueType == AnimValueType::Vector4)
        {
            CCompoundSplineTrack* compoundTrack = static_cast<CCompoundSplineTrack*>(track);
            Vec4 val(0.0f, 0.0f, 0.0f, 0.0f);
            m_description->m_controlParams[paramIndex]->GetDefault(val);
            compoundTrack->SetValue(0.0f, val, true);
        }
        else if (valueType == AnimValueType::RGB)
        {
            CCompoundSplineTrack* compoundTrack = static_cast<CCompoundSplineTrack*>(track);
            Vec3 val(0.0f, 0.0f, 0.0f);
            m_description->m_controlParams[paramIndex]->GetDefault(val);
            compoundTrack->SetValue(0.0f, val, true);
        }
        else
        {
            AZ_Assert(false, "Unknown value type track in Time of Day node creation");
        }
    }
}

//-----------------------------------------------------------------------------
void CAnimTODNode::Animate(SAnimContext& ac)
{
    ITimeOfDay* timeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    for (size_t trackIndex = 0; trackIndex < m_tracks.size(); ++trackIndex)
    {
        IAnimTrack* track = m_tracks[trackIndex].get();
        AZ_Assert(track, "Track is nullptr");
        size_t paramIndex = (size_t)static_cast<int>(m_tracks[trackIndex]->GetParameterType().GetType()) - static_cast<int>(AnimParamType::User);
        AZ_Assert(paramIndex < m_description->m_nodeParams.size(), "Too many parameters");

        if (track->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
        {
            continue;
        }

        if (track->IsMasked(ac.trackMask))
        {
            continue;
        }

        AnimValueType valueType = m_description->m_nodeParams[paramIndex].valueType;
        if (valueType == AnimValueType::Float)
        {
            ITimeOfDay::SVariableInfo varInfo;
            if (timeOfDay->GetVariableInfo(m_description->m_controlParams[paramIndex]->m_paramId, varInfo))
            {
                // val[1] and val[2] are treated as min/max values that are clamped. Ensure that we don't change those bounds
                float val[3] = { varInfo.fValue[0], varInfo.fValue[1], varInfo.fValue[2] };
                track->GetValue(ac.time, val[0]);
                timeOfDay->SetVariableValue(m_description->m_controlParams[paramIndex]->m_paramId, val);
            }
        }
        else if (valueType == AnimValueType::RGB)
        {
            Vec3 val(0.0f, 0.0f, 0.0f);
            static_cast<CCompoundSplineTrack*>(track)->GetValue(ac.time, val);

            float arrayVal[3]{ val.x, val.y, val.z };
            timeOfDay->SetVariableValue(m_description->m_controlParams[paramIndex]->m_paramId, arrayVal);
        }
    }
}

//-----------------------------------------------------------------------------
void CAnimTODNode::Activate(bool bActivate)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Movie);

    if (!bActivate)
    {
        OnReset();
    }
}

//-----------------------------------------------------------------------------
void CAnimTODNode::OnReset()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Movie);

    CAnimNode::OnReset();
    ILevel* currentLevel = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel();

    if (gEnv->IsEditor())
    {
        AZStd::string path = currentLevel ? currentLevel->GetLevelInfo()->GetPath() : "";

        if (path.length() > 0)
        {
            path.append("/leveldata/TimeOfDay.xml");
            m_xmlData = gEnv->pSystem->LoadXmlFromFile(path.c_str());
        }
    }
    else
    {
        if (m_xmlData == nullptr)
        {
            const char* missionName = currentLevel ? currentLevel->GetLevelInfo()->GetDefaultGameType()->name : nullptr;
            if (missionName && missionName[0])
            {
                char fileName[256];
                azsnprintf(fileName, sizeof(fileName), "mission_%s.xml", missionName);
                Maestro::TimeOfDayCacheRequestBus::BroadcastResult(m_xmlData, &Maestro::TimeOfDayCacheRequests::Get, fileName);
                if (!m_xmlData)
                {
                    AZ_Warning("CAnimTODNode::OnReset", false, "Mission file not found: %s", fileName);
                }
            }
        }
    }

    if (m_xmlData != nullptr)
    {
        if (!_stricmp(m_xmlData->getTag(), "TimeOfDay"))
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Movie, "CAnimTODNode::OnReset - Init and update TOD");

            ITimeOfDay* timeOfDay = gEnv->p3DEngine->GetTimeOfDay();

            timeOfDay->ResetVariables();
            if (gEnv->IsEditor())
            {
                timeOfDay->Serialize(m_xmlData, true);
            }
            else
            {
                Maestro::TimeOfDayCacheRequestBus::Broadcast(&Maestro::TimeOfDayCacheRequests::ResetTimeOfDayParams);
            }

            timeOfDay->UpdateEnvLighting(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimTODNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimTODNode, CAnimNode>()
        ->Version(1);
}
