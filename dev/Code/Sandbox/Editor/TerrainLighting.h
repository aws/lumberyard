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

#ifndef CRYINCLUDE_EDITOR_TERRAINLIGHTING_H
#define CRYINCLUDE_EDITOR_TERRAINLIGHTING_H

#pragma once
// TerrainLighting.h : header file
//

#include <QDialog>
#include <QLabel>
#include <QPixmap>
#include <QScopedPointer>

#define LIGHTING_TOOL_WINDOW_NAME "Sun Trajectory Tool"

/////////////////////////////////////////////////////////////////////////////
// CTerrainLighting dialog

enum eLightAlgorithm                            // correspond to the radio buttons in the lighting dialog
{
    //  eHemisphere = 0,
    eDP3 = 1,                                   // <will be removed soon>
    ePrecise = 2,                           // Sky Accessiblity, lambert sun lighting, soft shadows, baked together with diffuse color (DXT1)
    eDynamicSun = 3,                    // diffuse color in (DXT1) | sky accesiblity, lambert sun lighting, vegetation shadow (R5G6B5)
};

struct LightingSettings
{
    eLightAlgorithm eAlgo;          // Algorithm to use for the lighting calculations
    bool bTerrainShadows;               // Calculate shadows from hills (default false)
    bool bLighting;                         // Calculate lighting (default true)
    int iSunRotation;                       // Rotation of the sun (default 240 to get sun up in east ) 0..360
    int iSunHeight;                         // Height of the sun (default 25) 0..100
    int iShadowIntensity;               // 0=no shadow..100=full shadow, Intensity of the shadows on vegetation
    int iILApplySS;                         // 0 = apply no super sampling to terrain occl
    int iShadowBlur;                        // obsolete, keep it for backstep to sun acc.
    int iHemiSamplQuality;          // 0=no sampling (=ambient, might be brighter), 1=low (faster) .. 10=highest(slow)
    //  int iSkyBrightening;                // 0..100
    int iLongitude;                         // 0..360 m_sldLongitude (0..180 = north..equator..south pole)

    int iDawnTime;
    int iDawnDuration;
    int iDuskTime;
    int iDuskDuration;

    //! constructor
    LightingSettings()
    {
        eAlgo = eDynamicSun;
        bTerrainShadows = true;
        bLighting = true;
        iSunRotation = 240;
        iSunHeight = 80;
        iShadowIntensity = 100;
        iILApplySS = 1;
        iHemiSamplQuality = 5;
        //      iSkyBrightening=0;
        iLongitude = 90;        // equator
        iShadowBlur = 0;

        iDawnTime = 355;
        iDawnDuration = 10;
        iDuskTime = 365;
        iDuskDuration = 10;
    }

    Vec3 GetSunVector() const
    {
        Vec3 v;

        v.z = -((float)iSunHeight / 100.0f);

        float r = sqrtf(1.0f - v.z * v.z);

        v.x = sinf(DEG2RAD(iSunRotation)) * r;
        v.y = cosf(DEG2RAD(iSunRotation)) * r;

        // Normalize the light vector
        return (v).GetNormalized();
    }

    void Serialize(XmlNodeRef& node, bool bLoading)
    {
        ////////////////////////////////////////////////////////////////////////
        // Load or restore the layer settings from an XML
        ////////////////////////////////////////////////////////////////////////
        if (bLoading)
        {
            XmlNodeRef light = node->findChild("Lighting");
            if (!light)
            {
                return;
            }

            int algo = 0;
            light->getAttr("SunRotation", iSunRotation);
            light->getAttr("SunHeight", iSunHeight);
            light->getAttr("Algorithm", algo);
            light->getAttr("Lighting", bLighting);
            light->getAttr("Shadows", bTerrainShadows);
            light->getAttr("ShadowIntensity", iShadowIntensity);
            light->getAttr("ILQuality", iILApplySS);
            light->getAttr("HemiSamplQuality", iHemiSamplQuality);
            //          light->getAttr( "SkyBrightening",iSkyBrightening );
            light->getAttr("Longitude", iLongitude);

            light->getAttr("DawnTime", iDawnTime);
            light->getAttr("DawnDuration", iDawnDuration);
            light->getAttr("DuskTime", iDuskTime);
            light->getAttr("DuskDuration", iDuskDuration);

            eAlgo = eDynamicSun;            // only supported algorithm
        }
        else
        {
            int algo = (int)eAlgo;

            // Storing
            XmlNodeRef light = node->newChild("Lighting");
            light->setAttr("SunRotation", iSunRotation);
            light->setAttr("SunHeight", iSunHeight);
            light->setAttr("Algorithm", algo);
            light->setAttr("Lighting", bLighting);
            light->setAttr("Shadows", bTerrainShadows);
            light->setAttr("ShadowIntensity", iShadowIntensity);
            light->setAttr("ILQuality", iILApplySS);
            light->setAttr("HemiSamplQuality", iHemiSamplQuality);
            //          light->setAttr( "SkyBrightening",iSkyBrightening );
            light->setAttr("Longitude", iLongitude);

            light->setAttr("DawnTime", iDawnTime);
            light->setAttr("DawnDuration", iDawnDuration);
            light->setAttr("DuskTime", iDuskTime);
            light->setAttr("DuskDuration", iDuskDuration);

            Vec3 sunVector = GetSunVector();
            light->setAttr("SunVector", sunVector);
        }
    }
};

namespace Ui {
    class TerrainLighting;
}

class CTerrainLighting
    : public QWidget
    , IEditorNotifyListener
{
    Q_OBJECT
    // Construction
public:
    CTerrainLighting(QWidget* pParent = NULL);   // standard constructor
    ~CTerrainLighting();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

public:
    static const GUID& GetClassID();
    static void RegisterViewClass();
    // Implementation
protected:
    void InitTerrainTexGen();
    void showEvent(QShowEvent* event);

    QImage m_bmpLightmap;
    CImageEx m_lightmap;

    CImageEx m_moonSunTrans;

    QPixmap m_moonSunTransitionPreview;
    QPixmap m_sunPathPreview;

#ifdef LY_TERRAIN_EDITOR
    class CTerrainTexGen* m_pTexGen;
#endif // #ifdef LY_TERRAIN_EDITOR

protected slots:
    void OnHSlidersScroll();
    void OnApplyILSS();
    void OnFileImport();
    void OnFileExport();
    void OnDynamicSun();
    void OnPrecise();
    void UpdateScrollBarsFromEdits();
    void UpdateDocument();
    void OnForceSkyUpdate();
    void OnSkyQualitySlider();
    void OnLightingTimeOfDaySlider();
    void OnSunDirectionSlider();
    void OnSunMapLongitudeSlider();
    void OnDawnDurationSlider();
    void OnDuskTimeSlider();
    void OnDawnTimeSlider();
    void OnDuskDurationSlider();

    float GetTime() const;
    void SetTime(const float fTime, const bool bforceSkyUpate);

private:
    void UpdateControls();
    void GenerateLightmap(bool drawOntoDialog = true);
    void GenerateMoonSunTransition(bool drawOntoDialog = true);
    void UpdateMoonSunTransitionLabels();
    void Refresh();

    LightingSettings m_originalLightSettings;
    QScopedPointer<Ui::TerrainLighting> ui;
};


#endif // CRYINCLUDE_EDITOR_TERRAINLIGHTING_H
