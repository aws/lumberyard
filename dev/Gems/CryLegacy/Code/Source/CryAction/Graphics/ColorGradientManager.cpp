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

#include "CryLegacy_precompiled.h"

#include "ColorGradientManager.h"

namespace Graphics
{
    CColorGradientManager::CColorGradientManager()
    {
        AZ::ColorGradingRequestBus::Handler::BusConnect();
    }

    CColorGradientManager::~CColorGradientManager()
    {
        AZ::ColorGradingRequestBus::Handler::BusDisconnect();
    }

    void CColorGradientManager::Reset()
    {
        stl::free_container(m_colorGradientsToLoad);

        IColorGradingController& colorGradingController = GetColorGradingController();
        for (std::vector<LoadedColorGradient>::iterator it = m_currentGradients.begin(), itEnd = m_currentGradients.end(); it != itEnd; ++it)
        {
            LoadedColorGradient& cg = *it;
            if (cg.m_layer.m_texID >= 0)
            {
                colorGradingController.UnloadColorChart(cg.m_layer.m_texID);
            }
        }

        stl::free_container(m_currentGradients);

        // Reset to identity color grading settings
        colorGradingController.SetLayers(nullptr, 0);
    }

    void CColorGradientManager::Serialize(TSerialize serializer)
    {
        if (serializer.IsReading())
        {
            Reset();
        }

        serializer.BeginGroup("ColorGradientManager");
        {
            int numToLoad = (int)m_colorGradientsToLoad.size();
            int numLoaded = (int)m_currentGradients.size();
            int numGradients = numToLoad + numLoaded;
            serializer.Value("ColorGradientCount", numGradients);
            if (serializer.IsWriting())
            {
                for (int i = 0; i < numToLoad; ++i)
                {
                    LoadingColorGradient& gradient = m_colorGradientsToLoad[i];
                    serializer.BeginGroup("ColorGradient");
                    serializer.Value("FilePath", gradient.m_filePath);
                    serializer.Value("FadeInTime", gradient.m_fadeInTimeInSeconds);
                    serializer.EndGroup();
                }
                for (int i = 0; i < numLoaded; ++i)
                {
                    LoadedColorGradient& gradient = m_currentGradients[i];
                    serializer.BeginGroup("ColorGradient");
                    serializer.Value("FilePath", gradient.m_filePath);
                    serializer.Value("BlendAmount", gradient.m_layer.m_blendAmount);
                    serializer.Value("FadeInTime", gradient.m_fadeInTimeInSeconds);
                    serializer.Value("ElapsedTime", gradient.m_elapsedTime);
                    serializer.Value("MaximumBlendAmount", gradient.m_maximumBlendAmount);
                    serializer.EndGroup();
                }
            }
            else
            {
                m_currentGradients.reserve(numGradients);
                for (int i = 0; i < numGradients; ++i)
                {
                    serializer.BeginGroup("ColorGradient");
                    string filePath;
                    float blendAmount = 1.0f;
                    float fadeInTimeInSeconds = 0.0f;
                    serializer.Value("FilePath", filePath);
                    serializer.Value("BlendAmount", blendAmount);
                    serializer.Value("FadeInTime", fadeInTimeInSeconds);
                    const int textureID = GetColorGradingController().LoadColorChart(filePath);
                    LoadedColorGradient gradient(filePath, SColorChartLayer(textureID, blendAmount), fadeInTimeInSeconds);

                    // Optional
                    serializer.ValueWithDefault("ElapsedTime", gradient.m_elapsedTime, 0.0f);
                    serializer.ValueWithDefault("MaximumBlendAmount", gradient.m_maximumBlendAmount, 1.0f);

                    m_currentGradients.push_back(gradient);
                    serializer.EndGroup();
                }
            }
            serializer.EndGroup();
        }
    }

    void CColorGradientManager::TriggerFadingColorGradient(const string& filePath, const float fadeInTimeInSeconds)
    {
        const unsigned int numGradients = (int) m_currentGradients.size();
        for (unsigned int currentGradientIndex = 0; currentGradientIndex < numGradients; ++currentGradientIndex)
        {
            m_currentGradients[currentGradientIndex].FreezeMaximumBlendAmount();
        }

        m_colorGradientsToLoad.push_back(LoadingColorGradient(filePath, fadeInTimeInSeconds));
    }

    void CColorGradientManager::FadeInColorChart(const AZStd::string& colorChartTextureName, float fadeTime)
    {
        TriggerFadingColorGradient(colorChartTextureName.c_str(), fadeTime);
    }

    void CColorGradientManager::UpdateForThisFrame(const float frameTimeInSeconds)
    {
        RemoveZeroWeightedLayers();

        LoadGradients();

        FadeInLastLayer(frameTimeInSeconds);
        FadeOutCurrentLayers();

        SetLayersForThisFrame();
    }

    void CColorGradientManager::FadeInLastLayer(const float frameTimeInSeconds)
    {
        if (m_currentGradients.empty())
        {
            return;
        }

        m_currentGradients.back().FadeIn(frameTimeInSeconds);
    }

    void CColorGradientManager::FadeOutCurrentLayers()
    {
        if (m_currentGradients.size() <= 1u)
        {
            return;
        }

        const unsigned int numberofFadingOutGradients = (int) m_currentGradients.size() - 1;
        const float fadingInLayerBlendAmount = m_currentGradients[numberofFadingOutGradients].m_layer.m_blendAmount;
        for (unsigned int index = 0; index < numberofFadingOutGradients; ++index)
        {
            m_currentGradients[index].FadeOut(fadingInLayerBlendAmount);
        }
    }

    void CColorGradientManager::RemoveZeroWeightedLayers()
    {
        std::vector<LoadedColorGradient>::iterator currentGradient = m_currentGradients.begin();

        while (currentGradient != m_currentGradients.end())
        {
            if (currentGradient->m_layer.m_blendAmount == 0.0f)
            {
                GetColorGradingController().UnloadColorChart(currentGradient->m_layer.m_texID);

                currentGradient = m_currentGradients.erase(currentGradient);
            }
            else
            {
                ++currentGradient;
            }
        }
    }

    void CColorGradientManager::SetLayersForThisFrame()
    {
        std::vector<SColorChartLayer> thisFrameLayers;

        const unsigned int numberOfFadingInGradients = (int) m_currentGradients.size();
        thisFrameLayers.reserve(numberOfFadingInGradients + thisFrameLayers.size());
        for (unsigned int index = 0; index < numberOfFadingInGradients; ++index)
        {
            thisFrameLayers.push_back(m_currentGradients[index].m_layer);
        }

        const uint32 numLayers = (uint32) thisFrameLayers.size();
        const SColorChartLayer* pLayers = numLayers ? &thisFrameLayers.front() : 0;
        GetColorGradingController().SetLayers(pLayers, numLayers);
    }

    void CColorGradientManager::LoadGradients()
    {
        const unsigned int numGradientsToLoad = (int) m_colorGradientsToLoad.size();
        m_currentGradients.reserve(numGradientsToLoad +  m_currentGradients.size());
        for (unsigned int index = 0; index < numGradientsToLoad; ++index)
        {
            LoadedColorGradient loadedGradient = m_colorGradientsToLoad[index].Load(GetColorGradingController());

            m_currentGradients.push_back(loadedGradient);
        }

        m_colorGradientsToLoad.clear();
    }

    IColorGradingController& CColorGradientManager::GetColorGradingController()
    {
        return *gEnv->pRenderer->GetIColorGradingController();
    }

    CColorGradientManager::LoadedColorGradient::LoadedColorGradient(const string& filePath, const SColorChartLayer& layer, const float fadeInTimeInSeconds)
        : m_filePath(filePath)
        , m_layer(layer)
        , m_fadeInTimeInSeconds(fadeInTimeInSeconds)
        , m_elapsedTime(0.0f)
        , m_maximumBlendAmount(1.0f)
    {
    }

    void CColorGradientManager::LoadedColorGradient::FadeIn(const float frameTimeInSeconds)
    {
        if (m_fadeInTimeInSeconds <= 0.0f)
        {
            m_layer.m_blendAmount = 1.0f;

            return;
        }

        m_elapsedTime += frameTimeInSeconds;

        const float blendAmount = m_elapsedTime / m_fadeInTimeInSeconds;

        m_layer.m_blendAmount = min(blendAmount, 1.0f);
    }

    void CColorGradientManager::LoadedColorGradient::FadeOut(const float blendAmountOfFadingInGradient)
    {
        m_layer.m_blendAmount = m_maximumBlendAmount * (1.0f - blendAmountOfFadingInGradient);
    }

    void CColorGradientManager::LoadedColorGradient::FreezeMaximumBlendAmount()
    {
        m_maximumBlendAmount = m_layer.m_blendAmount;
    }

    CColorGradientManager::LoadingColorGradient::LoadingColorGradient(const string& filePath, const float fadeInTimeInSeconds)
        : m_filePath(filePath)
        , m_fadeInTimeInSeconds(fadeInTimeInSeconds)
    {
    }

    Graphics::CColorGradientManager::LoadedColorGradient CColorGradientManager::LoadingColorGradient::Load(IColorGradingController& colorGradingController) const
    {
        const int textureID = colorGradingController.LoadColorChart(m_filePath);

        return LoadedColorGradient(m_filePath, SColorChartLayer(textureID, 1.0f), m_fadeInTimeInSeconds);
    }
}
