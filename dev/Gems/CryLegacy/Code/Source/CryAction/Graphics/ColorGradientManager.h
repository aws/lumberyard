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

#ifndef COLOR_GRADIENT_MANAGER_H_INCLUDED
#define COLOR_GRADIENT_MANAGER_H_INCLUDED

#include <IColorGradingController.h>
#include <ISerialize.h>
#include <ColorGradingBus.h>

namespace Graphics
{
    class CColorGradientManager : public AZ::ColorGradingRequestBus::Handler
    {
    public:
        CColorGradientManager();
        virtual ~CColorGradientManager();

        void TriggerFadingColorGradient(const string& filePath, const float fadeInTimeInSeconds);

        void UpdateForThisFrame(const float frameTimeInSeconds);
        void Serialize(TSerialize serializer);

        //////////////////////////////////////////////////////////////////////////
        // ColorGradingRequestBus::Handler implementation
        void FadeInColorChart(const AZStd::string& colorChartTextureName, float fadeTime) override;
        void Reset() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        void FadeInLastLayer(const float frameTimeInSeconds);
        void FadeOutCurrentLayers();
        void RemoveZeroWeightedLayers();
        void SetLayersForThisFrame();
        void LoadGradients();

        IColorGradingController& GetColorGradingController();

    private:

        class LoadedColorGradient
        {
        public:
            LoadedColorGradient(const string& filePath, const SColorChartLayer& layer, const float fadeInTimeInSeconds);

        public:
            void FadeIn(const float frameTimeInSeconds);
            void FadeOut(const float blendAmountOfFadingInGradient);

            void FreezeMaximumBlendAmount();

            SColorChartLayer m_layer;
            string m_filePath;
            float m_fadeInTimeInSeconds;
            float m_elapsedTime;
            float m_maximumBlendAmount;
        };

        class LoadingColorGradient
        {
        public:
            LoadingColorGradient(const string& filePath, const float fadeInTimeInSeconds);

            LoadedColorGradient Load(IColorGradingController& colorGradingController) const;

        public:
            string m_filePath;
            float m_fadeInTimeInSeconds;
        };

    private:

        std::vector<LoadingColorGradient> m_colorGradientsToLoad;
        std::vector<LoadedColorGradient> m_currentGradients;
    };
}

#endif //COLOR_GRADIENT_MANAGER_H_INCLUDED