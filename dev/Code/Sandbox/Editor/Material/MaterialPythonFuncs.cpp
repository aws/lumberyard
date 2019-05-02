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

// Description : Material support for Python


#include "StdAfx.h"
#include "Material/MaterialManager.h"
#include "Util/BoostPythonHelpers.h"
#include "ShaderEnum.h"

namespace
{
    void PyMaterialCreate()
    {
        GetIEditor()->GetMaterialManager()->Command_Create();
    }

    void PyMaterialCreateMulti()
    {
        GetIEditor()->GetMaterialManager()->Command_CreateMulti();
    }

    void PyMaterialConvertToMulti()
    {
        GetIEditor()->GetMaterialManager()->Command_ConvertToMulti();
    }

    void PyMaterialDuplicateCurrent()
    {
        GetIEditor()->GetMaterialManager()->Command_Duplicate();
    }

    void PyMaterialMergeSelection()
    {
        GetIEditor()->GetMaterialManager()->Command_Merge();
    }

    void PyMaterialDeleteCurrent()
    {
        GetIEditor()->GetMaterialManager()->Command_Delete();
    }

    void PyMaterialAssignCurrentToSelection()
    {
        CUndo undo("Assign Material To Selection");
        GetIEditor()->GetMaterialManager()->Command_AssignToSelection();
    }

    void PyMaterialResetSelection()
    {
        GetIEditor()->GetMaterialManager()->Command_ResetSelection();
    }

    void PyMaterialSelectObjectsWithCurrent()
    {
        CUndo undo("Select Objects With Current Material");
        GetIEditor()->GetMaterialManager()->Command_SelectAssignedObjects();
    }

    void PyMaterialSetCurrentFromObject()
    {
        GetIEditor()->GetMaterialManager()->Command_SelectFromObject();
    }

    std::vector<std::string> PyGetSubMaterial(const char* pMaterialPath)
    {
        QString materialPath = pMaterialPath;
        CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(pMaterialPath, false);
        if (!pMaterial)
        {
            throw std::runtime_error("Invalid multi material.");
        }

        std::vector<std::string> result;
        for (int i = 0; i < pMaterial->GetSubMaterialCount(); i++)
        {
            if (pMaterial->GetSubMaterial(i))
            {
                result.push_back((materialPath + "\\" + pMaterial->GetSubMaterial(i)->GetName()).toUtf8().data());
            }
        }
        return result;
    }

    CMaterial* TryLoadingMaterial(const char* pPathAndMaterialName)
    {
        QString varMaterialPath = pPathAndMaterialName;
        std::deque<QString> splittedMaterialPath;
        for (auto token : varMaterialPath.split(QRegularExpression(R"([\\/])"), QString::SkipEmptyParts))
        {
            splittedMaterialPath.push_back(token);
        }

        CMaterial* pMaterial  = GetIEditor()->GetMaterialManager()->LoadMaterial(varMaterialPath, false);
        if (!pMaterial)
        {
            QString subMaterialName = splittedMaterialPath.back();
            bool isSubMaterialExist(false);

            varMaterialPath.remove((varMaterialPath.length() - subMaterialName.length() - 1), varMaterialPath.length());
            QString test = varMaterialPath;
            pMaterial  = GetIEditor()->GetMaterialManager()->LoadMaterial(varMaterialPath, false);

            if (!pMaterial)
            {
                throw std::runtime_error("Invalid multi material.");
            }

            for (int i = 0; i < pMaterial->GetSubMaterialCount(); i++)
            {
                if (pMaterial->GetSubMaterial(i)->GetName() == subMaterialName)
                {
                    pMaterial = pMaterial->GetSubMaterial(i);
                    isSubMaterialExist = true;
                }
            }

            if (!pMaterial || !isSubMaterialExist)
            {
                throw std::runtime_error((QString("\"") + subMaterialName + "\" is an invalid sub material.").toUtf8().data());
            }
        }
        GetIEditor()->GetMaterialManager()->SetCurrentMaterial(pMaterial);
        return pMaterial;
    }

    std::deque<QString> PreparePropertyPath(const char* pPathAndPropertyName)
    {
        QString varPathProperty = pPathAndPropertyName;
        std::deque<QString> splittedPropertyPath;
        for (auto token : varPathProperty.split(QRegularExpression(R"([\\/])"), QString::SkipEmptyParts))
        {
            splittedPropertyPath.push_back(token);
        }

        return splittedPropertyPath;
    }

    //////////////////////////////////////////////////////////////////////////
    // Converter: Enum -> CString (human readable)
    //////////////////////////////////////////////////////////////////////////

    QString TryConvertingSEFResTextureToCString(SEfResTexture* pResTexture)
    {
        if (pResTexture)
        {
            return pResTexture->m_Name.c_str();
        }
        return "";
    }

    QString TryConvertingETEX_TypeToCString(const uint8& texTypeName)
    {
        switch (texTypeName)
        {
        case eTT_2D:
            return "2D";
        case eTT_Cube:
            return "Cube-Map";
        case eTT_NearestCube:
            return "Nearest Cube-Map probe for alpha blended";
        case eTT_Auto2D:
            return "Auto 2D-Map";
        case eTT_Dyn2D:
            return "Dynamic 2D-Map";
        case eTT_User:
            return "From User Params";
        default:
            throw std::runtime_error("Invalid tex type.");
        }
    }

    QString TryConvertingTexFilterToCString(const int& iFilterName)
    {
        switch (iFilterName)
        {
        case FILTER_NONE:
            return "Default";
        case FILTER_POINT:
            return "Point";
        case FILTER_LINEAR:
            return "Linear";
        case FILTER_BILINEAR:
            return "Bilinear";
        case FILTER_TRILINEAR:
            return "Trilinear";
        case FILTER_ANISO2X:
            return "Anisotropic 2x";
        case FILTER_ANISO4X:
            return "Anisotropic 4x";
        case FILTER_ANISO8X:
            return "Anisotropic 8x";
        case FILTER_ANISO16X:
            return "Anisotropic 16x";
        default:
            throw std::runtime_error("Invalid tex filter.");
        }
    }

    QString TryConvertingETexGenTypeToCString(const uint8& texGenType)
    {
        switch (texGenType)
        {
        case ETG_Stream:
            return "Stream";
        case ETG_World:
            return "World";
        case ETG_Camera:
            return "Camera";
        default:
            throw std::runtime_error("Invalid tex gen type.");
        }
    }

    QString TryConvertingETexModRotateTypeToCString(const uint8& rotateType)
    {
        switch (rotateType)
        {
        case ETMR_NoChange:
            return "No Change";
        case ETMR_Fixed:
            return "Fixed Rotation";
        case ETMR_Constant:
            return "Constant Rotation";
        case ETMR_Oscillated:
            return "Oscillated Rotation";
        default:
            throw std::runtime_error("Invalid rotate type.");
        }
    }

    QString TryConvertingETexModMoveTypeToCString(const uint8& oscillatorType)
    {
        switch (oscillatorType)
        {
        case ETMM_NoChange:
            return "No Change";
        case ETMM_Fixed:
            return "Fixed Moving";
        case ETMM_Constant:
            return "Constant Moving";
        case ETMM_Jitter:
            return "Jitter Moving";
        case ETMM_Pan:
            return "Pan Moving";
        case ETMM_Stretch:
            return "Stretch Moving";
        case ETMM_StretchRepeat:
            return "Stretch-Repeat Moving";
        default:
            throw std::runtime_error("Invalid oscillator type.");
        }
    }

    QString TryConvertingEDeformTypeToCString(const EDeformType& deformType)
    {
        switch (deformType)
        {
        case eDT_Unknown:
            return "None";
        case eDT_SinWave:
            return "Sin Wave";
        case eDT_SinWaveUsingVtxColor:
            return "Sin Wave using vertex color";
        case eDT_Bulge:
            return "Bulge";
        case eDT_Squeeze:
            return "Squeeze";
        case eDT_Perlin2D:
            return "Perlin 2D";
        case eDT_Perlin3D:
            return "Perlin 3D";
        case eDT_FromCenter:
            return "From Center";
        case eDT_Bending:
            return "Bending";
        case eDT_ProcFlare:
            return "Proc. Flare";
        case eDT_AutoSprite:
            return "Auto sprite";
        case eDT_Beam:
            return "Beam";
        case eDT_FixedOffset:
            return "FixedOffset";
        default:
            throw std::runtime_error("Invalid deform type.");
        }
    }

    QString TryConvertingEWaveFormToCString(const EWaveForm& waveForm)
    {
        switch (waveForm)
        {
        case eWF_None:
            return "None";
        case eWF_Sin:
            return "Sin";
        case eWF_HalfSin:
            return "Half Sin";
        case eWF_Square:
            return "Square";
        case eWF_Triangle:
            return "Triangle";
        case eWF_SawTooth:
            return "Saw Tooth";
        case eWF_InvSawTooth:
            return "Inverse Saw Tooth";
        case eWF_Hill:
            return "Hill";
        case eWF_InvHill:
            return "Inverse Hill";
        default:
            throw std::runtime_error("Invalid wave form.");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Converter: CString -> Enum
    //////////////////////////////////////////////////////////////////////////

    template<typename STRING>
    // [Shader System TO DO] remove once dynamic slots assingment is in place
    EEfResTextures TryConvertingCStringToEEfResTextures(const STRING& resTextureName)
    {
        if (resTextureName == "Diffuse")
        {
            return EFTT_DIFFUSE;
        }
        else if (resTextureName == "Specular")
        {
            return EFTT_SPECULAR;
        }
        else if (resTextureName == "Bumpmap")
        {
            return EFTT_NORMALS;
        }
        else if (resTextureName == "Heightmap")
        {
            return EFTT_HEIGHT;
        }
        else if (resTextureName == "Environment")
        {
            return EFTT_ENV;
        }
        else if (resTextureName == "Detail")
        {
            return EFTT_DETAIL_OVERLAY;
        }
        else if (resTextureName == "Opacity")
        {
            return EFTT_OPACITY;
        }
        else if (resTextureName == "Decal")
        {
            return EFTT_DECAL_OVERLAY;
        }
        else if (resTextureName == "SubSurface")
        {
            return EFTT_SUBSURFACE;
        }
        else if (resTextureName == "Custom")
        {
            return EFTT_CUSTOM;
        }
        else if (resTextureName == "[1] Custom")
        {
            return EFTT_DIFFUSE;
        }
        else if (resTextureName == "Emittance")
        {
            return EFTT_EMITTANCE;
        }
        else if (resTextureName == "Occlusion")
        {
            return EFTT_OCCLUSION;
        }
        else if (resTextureName == "Specular2")
        {
            return EFTT_SPECULAR_2;
        }
        throw std::runtime_error("Invalid texture name.");

        return EFTT_MAX;
    }

    template<typename STRING>
    ETEX_Type TryConvertingCStringToETEX_Type(const STRING& texTypeName)
    {
        if (texTypeName == "2D")
        {
            return eTT_2D;
        }
        else if (texTypeName == "Cube-Map")
        {
            return eTT_Cube;
        }
        else if (texTypeName == "Nearest Cube-Map probe for alpha blended")
        {
            return eTT_NearestCube;
        }
        else if (texTypeName == "Auto 2D-Map")
        {
            return eTT_Auto2D;
        }
        else if (texTypeName == "Dynamic 2D-Map")
        {
            return eTT_Dyn2D;
        }
        else if (texTypeName == "From User Params")
        {
            return eTT_User;
        }
        throw std::runtime_error("Invalid tex type name.");
    }

    template<typename STRING>
    signed char TryConvertingCStringToTexFilter(const STRING& filterName)
    {
        if (filterName == "Default")
        {
            return FILTER_NONE;
        }
        else if (filterName == "Point")
        {
            return FILTER_POINT;
        }
        else if (filterName == "Linear")
        {
            return FILTER_LINEAR;
        }
        else if (filterName == "Bilinear")
        {
            return FILTER_BILINEAR;
        }
        else if (filterName == "Trilinear")
        {
            return FILTER_TRILINEAR;
        }
        else if (filterName == "Anisotropic 2x")
        {
            return FILTER_ANISO2X;
        }
        else if (filterName == "Anisotropic 4x")
        {
            return FILTER_ANISO4X;
        }
        else if (filterName == "Anisotropic 8x")
        {
            return FILTER_ANISO8X;
        }
        else if (filterName == "Anisotropic 16x")
        {
            return FILTER_ANISO16X;
        }
        throw std::runtime_error("Invalid filter name.");
    }

    template<typename STRING>
    ETexGenType TryConvertingCStringToETexGenType(const STRING& texGenType)
    {
        if (texGenType == "Stream")
        {
            return ETG_Stream;
        }
        else if (texGenType == "World")
        {
            return ETG_World;
        }
        else if (texGenType == "Camera")
        {
            return ETG_Camera;
        }
        throw std::runtime_error("Invalid tex gen type name.");
    }

    template<typename STRING>
    ETexModRotateType TryConvertingCStringToETexModRotateType(const STRING& rotateType)
    {
        if (rotateType == "No Change")
        {
            return ETMR_NoChange;
        }
        else if (rotateType == "Fixed Rotation")
        {
            return ETMR_Fixed;
        }
        else if (rotateType == "Constant Rotation")
        {
            return ETMR_Constant;
        }
        else if (rotateType == "Oscillated Rotation")
        {
            return ETMR_Oscillated;
        }
        throw std::runtime_error("Invalid rotate type name.");
    }

    template<typename STRING>
    ETexModMoveType TryConvertingCStringToETexModMoveType(const STRING& oscillatorType)
    {
        if (oscillatorType == "No Change")
        {
            return ETMM_NoChange;
        }
        else if (oscillatorType == "Fixed Moving")
        {
            return ETMM_Fixed;
        }
        else if (oscillatorType == "Constant Moving")
        {
            return ETMM_Constant;
        }
        else if (oscillatorType == "Jitter Moving")
        {
            return ETMM_Jitter;
        }
        else if (oscillatorType == "Pan Moving")
        {
            return ETMM_Pan;
        }
        else if (oscillatorType == "Stretch Moving")
        {
            return ETMM_Stretch;
        }
        else if (oscillatorType == "Stretch-Repeat Moving")
        {
            return ETMM_StretchRepeat;
        }
        throw std::runtime_error("Invalid oscillator type.");
    }

    template<typename STRING>
    EDeformType TryConvertingCStringToEDeformType(const STRING& deformType)
    {
        if (deformType == "None")
        {
            return eDT_Unknown;
        }
        else if (deformType == "Sin Wave")
        {
            return eDT_SinWave;
        }
        else if (deformType == "Sin Wave using vertex color")
        {
            return eDT_SinWaveUsingVtxColor;
        }
        else if (deformType == "Bulge")
        {
            return eDT_Bulge;
        }
        else if (deformType == "Squeeze")
        {
            return eDT_Squeeze;
        }
        else if (deformType == "Perlin 2D")
        {
            return eDT_Perlin2D;
        }
        else if (deformType == "Perlin 3D")
        {
            return eDT_Perlin3D;
        }
        else if (deformType == "From Center")
        {
            return eDT_FromCenter;
        }
        else if (deformType == "Bending")
        {
            return eDT_Bending;
        }
        else if (deformType == "Proc. Flare")
        {
            return eDT_ProcFlare;
        }
        else if (deformType == "Auto sprite")
        {
            return eDT_AutoSprite;
        }
        else if (deformType == "Beam")
        {
            return eDT_Beam;
        }
        else if (deformType == "FixedOffset")
        {
            return eDT_FixedOffset;
        }
        throw std::runtime_error("Invalid deform type.");
    }

    template <typename STRING>
    EWaveForm TryConvertingCStringToEWaveForm(const STRING& waveForm)
    {
        if (waveForm == "None")
        {
            return eWF_None;
        }
        else if (waveForm == "Sin")
        {
            return eWF_Sin;
        }
        else if (waveForm == "Half Sin")
        {
            return eWF_HalfSin;
        }
        else if (waveForm == "Square")
        {
            return eWF_Square;
        }
        else if (waveForm == "Triangle")
        {
            return eWF_Triangle;
        }
        else if (waveForm == "Saw Tooth")
        {
            return eWF_SawTooth;
        }
        else if (waveForm == "Inverse Saw Tooth")
        {
            return eWF_InvSawTooth;
        }
        else if (waveForm == "Hill")
        {
            return eWF_Hill;
        }
        else if (waveForm == "Inverse Hill")
        {
            return eWF_InvHill;
        }
        throw std::runtime_error("Invalid wave form.");
    }

    //////////////////////////////////////////////////////////////////////////
    // Script parser
    //////////////////////////////////////////////////////////////////////////

    QString ParseUINameFromPublicParamsScript(const char* sUIScript)
    {
        string uiscript = sUIScript;
        string element[3];
        int p1 = 0;
        string itemToken = uiscript.Tokenize(";", p1);
        while (!itemToken.empty())
        {
            int nElements = 0;
            int p2 = 0;
            string token = itemToken.Tokenize(" \t\r\n=", p2);
            while (!token.empty())
            {
                element[nElements++] = token;
                if (nElements == 2)
                {
                    element[nElements] = itemToken.substr(p2);
                    element[nElements].Trim(" =\t\"");
                    break;
                }
                token = itemToken.Tokenize(" \t\r\n=", p2);
            }

            if (_stricmp(element[1], "UIName") == 0)
            {
                return element[2].c_str();
            }
            itemToken = uiscript.Tokenize(";", p1);
        }
        return "";
    }

    std::map<QString, float> ParseValidRangeFromPublicParamsScript(const char* sUIScript)
    {
        string uiscript = sUIScript;
        std::map<QString, float> result;
        bool isUIMinExist(false);
        bool isUIMaxExist(false);
        string element[3];
        int p1 = 0;
        string itemToken = uiscript.Tokenize(";", p1);
        while (!itemToken.empty())
        {
            int nElements = 0;
            int p2 = 0;
            string token = itemToken.Tokenize(" \t\r\n=", p2);
            while (!token.empty())
            {
                element[nElements++] = token;
                if (nElements == 2)
                {
                    element[nElements] = itemToken.substr(p2);
                    element[nElements].Trim(" =\t\"");
                    break;
                }
                token = itemToken.Tokenize(" \t\r\n=", p2);
            }

            if (_stricmp(element[1], "UIMin") == 0)
            {
                result["UIMin"] = atof(element[2]);
                isUIMinExist = true;
            }
            if (_stricmp(element[1], "UIMax") == 0)
            {
                result["UIMax"] = atof(element[2]);
                isUIMaxExist = true;
            }
            itemToken = uiscript.Tokenize(";", p1);
        }
        if (!isUIMinExist || !isUIMaxExist)
        {
            throw std::runtime_error("Invalid range for shader param.");
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    // Set Flags
    //////////////////////////////////////////////////////////////////////////

    void SetMaterialFlag(CMaterial* pMaterial, const EMaterialFlags& eFlag, const bool& bFlag)
    {
        if (pMaterial->GetFlags() & eFlag && bFlag == false)
        {
            pMaterial->SetFlags(pMaterial->GetFlags() - eFlag);
        }
        else if (!(pMaterial->GetFlags() & eFlag) && bFlag == true)
        {
            pMaterial->SetFlags(pMaterial->GetFlags() | eFlag);
        }
    }

    void SetPropagationFlag(CMaterial* pMaterial, const eMTL_PROPAGATION& eFlag, const bool& bFlag)
    {
        if (pMaterial->GetPropagationFlags() & eFlag && bFlag == false)
        {
            pMaterial->SetPropagationFlags(pMaterial->GetPropagationFlags() - eFlag);
        }
        else if (!(pMaterial->GetPropagationFlags() & eFlag) && bFlag == true)
        {
            pMaterial->SetPropagationFlags(pMaterial->GetPropagationFlags() | eFlag);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    bool ValidateProperty(CMaterial* pMaterial, const std::deque<QString>& splittedPropertyPathParam, const SPyWrappedProperty& value)
    {
        std::deque<QString> splittedPropertyPath = splittedPropertyPathParam;
        std::deque<QString> splittedPropertyPathSubCategory = splittedPropertyPathParam;
        QString categoryName = splittedPropertyPath.front();
        QString subCategoryName = "";
        QString subSubCategoryName = "";
        QString currentPath = "";
        QString propertyName = splittedPropertyPath.back();

        QString errorMsgInvalidValue = QString("Invalid value for property \"") + propertyName + "\"";
        QString errorMsgInvalidDataType = QString("Invalid data type for property \"") + propertyName + "\"";
        QString errorMsgInvalidPropertyPath = QString("Invalid property path");


        const int iMinColorValue = 0;
        const int iMaxColorValue = 255;

        if (splittedPropertyPathSubCategory.size() == 3)
        {
            splittedPropertyPathSubCategory.pop_back();
            subCategoryName = splittedPropertyPathSubCategory.back();
            currentPath = QString("") + categoryName + "/" + subCategoryName;

            if (
                subCategoryName != "TexType" &&
                subCategoryName != "Filter" &&
                subCategoryName != "IsProjectedTexGen" &&
                subCategoryName != "TexGenType" &&
                subCategoryName != "Wave X" &&
                subCategoryName != "Wave Y" &&
                subCategoryName != "Wave Z" &&
                subCategoryName != "Wave W" &&
                subCategoryName != "Shader1" &&
                subCategoryName != "Shader2" &&
                subCategoryName != "Shader3" &&
                subCategoryName != "Tiling" &&
                subCategoryName != "Rotator" &&
                subCategoryName != "Oscillator" &&
                subCategoryName != "Diffuse" &&
                subCategoryName != "Specular" &&
                subCategoryName != "Bumpmap" &&
                subCategoryName != "Heightmap" &&
                subCategoryName != "Environment" &&
                subCategoryName != "Detail" &&
                subCategoryName != "Opacity" &&
                subCategoryName != "Decal" &&
                subCategoryName != "SubSurface" &&
                subCategoryName != "Custom" &&
                subCategoryName != "[1] Custom"
                )
            {
                throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
            }
        }
        else if (splittedPropertyPathSubCategory.size() == 4)
        {
            splittedPropertyPathSubCategory.pop_back();
            subCategoryName = splittedPropertyPathSubCategory.back();
            splittedPropertyPathSubCategory.pop_back();
            subSubCategoryName = splittedPropertyPathSubCategory.back();
            currentPath = categoryName + "/" + subSubCategoryName + "/" + subCategoryName;

            if (
                subSubCategoryName != "Diffuse" &&
                subSubCategoryName != "Specular" &&
                subSubCategoryName != "Bumpmap" &&
                subSubCategoryName != "Heightmap" &&
                subSubCategoryName != "Environment" &&
                subSubCategoryName != "Detail" &&
                subSubCategoryName != "Opacity" &&
                subSubCategoryName != "Decal" &&
                subSubCategoryName != "SubSurface" &&
                subSubCategoryName != "Custom" &&
                subSubCategoryName != "[1] Custom"
                )
            {
                throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
            }
            else if (subCategoryName != "Tiling" && subCategoryName != "Rotator" && subCategoryName != "Oscillator")
            {
                throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
            }
        }
        else
        {
            currentPath = categoryName;
        }

        if (
            categoryName == "Material Settings" ||
            categoryName == "Opacity Settings" ||
            categoryName == "Lighting Settings" ||
            categoryName == "Advanced" ||
            categoryName == "Texture Maps" ||
            categoryName == "Vertex Deformation" ||
            categoryName == "Layer Presets")
        {
            if (propertyName == "Opacity" || propertyName == "AlphaTest" || propertyName == "Glow Amount")
            {
                // int: 0 < x < 100
                if (value.type != SPyWrappedProperty::eType_Int)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                if (value.property.intValue < 0 || value.property.intValue > 100)
                {
                    throw std::runtime_error(errorMsgInvalidValue.toUtf8().data());
                }
                return true;
            }
            else if (propertyName == "Glossiness")
            {
                // int: 0 < x < 255
                if (value.type != SPyWrappedProperty::eType_Int)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                if (value.property.intValue < iMinColorValue || value.property.intValue > iMaxColorValue)
                {
                    throw std::runtime_error(errorMsgInvalidValue.toUtf8().data());
                }
                return true;
            }
            else if (propertyName == "Specular Level")
            {
                // float: 0.0 < x < 4.0
                if (value.type != SPyWrappedProperty::eType_Float)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                if (value.property.floatValue < 0.0f || value.property.floatValue > 4.0f)
                {
                    throw std::runtime_error(errorMsgInvalidValue.toUtf8().data());
                }
                return true;
            }
            else if (
                propertyName == "TileU" ||
                propertyName == "TileV" ||
                propertyName == "OffsetU" ||
                propertyName == "OffsetV" ||
                propertyName == "RotateU" ||
                propertyName == "RotateV" ||
                propertyName == "RotateW" ||
                propertyName == "Rate" ||
                propertyName == "Phase" ||
                propertyName == "Amplitude" ||
                propertyName == "CenterU" ||
                propertyName == "CenterV" ||
                propertyName == "RateU" ||
                propertyName == "RateV" ||
                propertyName == "PhaseU" ||
                propertyName == "PhaseV" ||
                propertyName == "AmplitudeU" ||
                propertyName == "AmplitudeV" ||
                propertyName == "Wave Length X" ||
                propertyName == "Wave Length Y" ||
                propertyName == "Wave Length Z" ||
                propertyName == "Wave Length W" ||
                propertyName == "Level" ||
                propertyName == "Amplitude" ||
                propertyName == "Phase" ||
                propertyName == "Frequency")
            {
                // float: 0.0 < x < 100.0
                if (value.type != SPyWrappedProperty::eType_Float)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                if (value.property.floatValue < 0.0f || value.property.floatValue > 100.0f)
                {
                    throw std::runtime_error(errorMsgInvalidValue.toUtf8().data());
                }
                return true;
            }
            else if (propertyName == "Diffuse Color" || propertyName == "Specular Color" || propertyName == "Emissive Color")
            {
                // intVector(RGB): 0 < x < 255
                if (value.type != SPyWrappedProperty::eType_Color)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                if (value.property.colorValue.r < iMinColorValue || value.property.colorValue.r > iMaxColorValue)
                {
                    throw std::runtime_error((errorMsgInvalidValue + " (red)").toUtf8().data());
                }
                else if (value.property.colorValue.g < iMinColorValue || value.property.colorValue.g > iMaxColorValue)
                {
                    throw std::runtime_error((errorMsgInvalidValue + " (green)").toUtf8().data());
                }
                else if (value.property.colorValue.b < iMinColorValue || value.property.colorValue.b > iMaxColorValue)
                {
                    throw std::runtime_error((errorMsgInvalidValue + " (blue)").toUtf8().data());
                }
                return true;
            }
            else if (
                propertyName == "Link to Material" ||
                propertyName == "Surface Type" ||
                propertyName == "Diffuse" ||
                propertyName == "Specular" ||
                propertyName == "Bumpmap" ||
                propertyName == "Heightmap" ||
                propertyName == "Environment" ||
                propertyName == "Detail" ||
                propertyName == "Opacity" ||
                propertyName == "Decal" ||
                propertyName == "SubSurface" ||
                propertyName == "Custom" ||
                propertyName == "[1] Custom" ||
                propertyName == "TexType" ||
                propertyName == "Filter" ||
                propertyName == "TexGenType" ||
                propertyName == "Type" ||
                propertyName == "TypeU" ||
                propertyName == "TypeV")
            {
                // string
                if (value.type != SPyWrappedProperty::eType_String)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }
                return true;
            }
            else if (
                propertyName == "Additive" ||
                propertyName == "Allow layer activation" ||
                propertyName == "2 Sided" ||
                propertyName == "No Shadow" ||
                propertyName == "Use Scattering" ||
                propertyName == "Hide After Breaking" ||
                propertyName == "Fog Volume Shading Quality High" ||
                propertyName == "Blend Terrain Color" ||
                propertyName == "Propagate Material Settings" ||
                propertyName == "Propagate Opacity Settings" ||
                propertyName == "Propagate Lighting Settings" ||
                propertyName == "Propagate Advanced Settings" ||
                propertyName == "Propagate Texture Maps" ||
                propertyName == "Propagate Shader Params" ||
                propertyName == "Propagate Shader Generation" ||
                propertyName == "Propagate Vertex Deformation" ||
                propertyName == "Propagate Layer Presets" ||
                propertyName == "IsProjectedTexGen" ||
                propertyName == "IsTileU" ||
                propertyName == "IsTileV" ||
                propertyName == "No Draw")
            {
                // bool
                if (value.type != SPyWrappedProperty::eType_Bool)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }
                return true;
            }
            else if (propertyName == "Shader" || propertyName == "Shader1" || propertyName == "Shader2" || propertyName == "Shader3")
            {
                // string && valid shader
                if (value.type != SPyWrappedProperty::eType_String)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }

                CShaderEnum* pShaderEnum = GetIEditor()->GetShaderEnum();
                if (!pShaderEnum)
                {
                    throw std::runtime_error("Shader enumerator corrupted.");
                }
                pShaderEnum->EnumShaders();
                for (int i = 0; i < pShaderEnum->GetShaderCount(); i++)
                {
                    if (pShaderEnum->GetShader(i) == value.stringValue)
                    {
                        return true;
                    }
                }
            }
            else if (propertyName == "Noise Scale")
            {
                // FloatVec: undefined < x < undefined
                if (value.type != SPyWrappedProperty::eType_Vec3)
                {
                    throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                }
                return true;
            }
        }
        else if (categoryName == "Shader Params")
        {
            DynArray<SShaderParam>& shaderParams = pMaterial->GetShaderResources().m_ShaderParams;
            for (int i = 0; i < shaderParams.size(); i++)
            {
                if (propertyName == ParseUINameFromPublicParamsScript(shaderParams[i].m_Script.c_str()))
                {
                    if (shaderParams[i].m_Type == eType_FLOAT)
                    {
                        // float: valid range (from script)
                        if (value.type != SPyWrappedProperty::eType_Float)
                        {
                            throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                        }
                        std::map<QString, float> range = ParseValidRangeFromPublicParamsScript(shaderParams[i].m_Script.c_str());
                        if (value.property.floatValue < range["UIMin"] ||  value.property.floatValue > range["UIMax"])
                        {
                            QString errorMsg;
                            errorMsg = QStringLiteral("Invalid value for shader param \"%1\" (min: %2, max: %3)").arg(propertyName).arg(range["UIMin"]).arg(range["UIMax"]);
                            throw std::runtime_error(errorMsg.toUtf8().data());
                        }
                        return true;
                    }
                    else if (shaderParams[i].m_Type == eType_FCOLOR)
                    {
                        // intVector(RGB): 0 < x < 255
                        if (value.type != SPyWrappedProperty::eType_Color)
                        {
                            throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                        }

                        if (value.property.colorValue.r < iMinColorValue || value.property.colorValue.r > iMaxColorValue)
                        {
                            throw std::runtime_error((errorMsgInvalidValue + " (red)").toUtf8().data());
                        }
                        else if (value.property.colorValue.g < iMinColorValue || value.property.colorValue.g > iMaxColorValue)
                        {
                            throw std::runtime_error((errorMsgInvalidValue + " (green)").toUtf8().data());
                        }
                        else if (value.property.colorValue.b < iMinColorValue || value.property.colorValue.b > iMaxColorValue)
                        {
                            throw std::runtime_error((errorMsgInvalidValue + " (blue)").toUtf8().data());
                        }
                        return true;
                    }
                }
            }
        }
        else if (categoryName == "Shader Generation Params")
        {
            for (int i = 0; i < pMaterial->GetShaderGenParamsVars()->GetNumVariables(); i++)
            {
                if (propertyName == pMaterial->GetShaderGenParamsVars()->GetVariable(i)->GetHumanName())
                {
                    if (value.type != SPyWrappedProperty::eType_Bool)
                    {
                        throw std::runtime_error(errorMsgInvalidDataType.toUtf8().data());
                    }
                    return true;
                }
            }
        }
        else
        {
            throw std::runtime_error((errorMsgInvalidPropertyPath + " (" + currentPath + ")").toUtf8().data());
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////

    SPyWrappedProperty PyGetProperty(const char* pPathAndMaterialName, const char* pPathAndPropertyName)
    {
        CMaterial* pMaterial = TryLoadingMaterial(pPathAndMaterialName);
        std::deque<QString> splittedPropertyPath = PreparePropertyPath(pPathAndPropertyName);
        std::deque<QString> splittedPropertyPathCategory = splittedPropertyPath;
        QString categoryName = splittedPropertyPath.front();
        QString subCategoryName = "None";
        QString subSubCategoryName = "None";
        QString propertyName = splittedPropertyPath.back();
        QString errorMsgInvalidPropertyPath = "Invalid property path.";
        SPyWrappedProperty value;

        if (splittedPropertyPathCategory.size() == 3)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
        }
        else if (splittedPropertyPathCategory.size() == 4)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
            splittedPropertyPathCategory.pop_back();
            subSubCategoryName = splittedPropertyPathCategory.back();
        }

        // ########## Material Settings ##########
        if (categoryName == "Material Settings")
        {
            if (propertyName == "Shader")
            {
                value.type = SPyWrappedProperty::eType_String;
                value.stringValue = pMaterial->GetShaderName();
            }
            else if (propertyName == "Surface Type")
            {
                value.type = SPyWrappedProperty::eType_String;
                value.stringValue = pMaterial->GetSurfaceTypeName();
                if (value.stringValue.startsWith("mat_"))
                {
                    value.stringValue.remove(0, 4);
                }
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid material setting.").toUtf8().data());
            }
        }
        // ########## Opacity Settings ##########
        else if (categoryName == "Opacity Settings")
        {
            if (propertyName == "Opacity")
            {
                value.type = SPyWrappedProperty::eType_Int;
                value.property.floatValue = pMaterial->GetShaderResources().m_LMaterial.m_Opacity;
                value.property.intValue = value.property.floatValue * 100.0f;
            }
            else if (propertyName == "AlphaTest")
            {
                value.type = SPyWrappedProperty::eType_Int;
                value.property.floatValue = pMaterial->GetShaderResources().m_AlphaRef;
                value.property.intValue = value.property.floatValue * 100.0f;
            }
            else if (propertyName == "Additive")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_ADDITIVE;
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid opacity setting.").toUtf8().data());
            }
        }
        // ########## Lighting Settings ##########
        else if (categoryName == "Lighting Settings")
        {
            if (propertyName == "Diffuse Color")
            {
                value.type = SPyWrappedProperty::eType_Color;
                QColor col = ColorLinearToGamma(ColorF(
                            pMaterial->GetShaderResources().m_LMaterial.m_Diffuse.r,
                            pMaterial->GetShaderResources().m_LMaterial.m_Diffuse.g,
                            pMaterial->GetShaderResources().m_LMaterial.m_Diffuse.b));
                value.property.colorValue.r = col.red();
                value.property.colorValue.g = col.green();
                value.property.colorValue.b = col.blue();
            }
            else if (propertyName == "Specular Color")
            {
                value.type = SPyWrappedProperty::eType_Color;
                QColor col = ColorLinearToGamma(ColorF(
                            pMaterial->GetShaderResources().m_LMaterial.m_Specular.r / pMaterial->GetShaderResources().m_LMaterial.m_Specular.a,
                            pMaterial->GetShaderResources().m_LMaterial.m_Specular.g / pMaterial->GetShaderResources().m_LMaterial.m_Specular.a,
                            pMaterial->GetShaderResources().m_LMaterial.m_Specular.b / pMaterial->GetShaderResources().m_LMaterial.m_Specular.a));
                value.property.colorValue.r = col.red();
                value.property.colorValue.g = col.green();
                value.property.colorValue.b = col.green();
            }
            else if (propertyName == "Glossiness")
            {
                value.type = SPyWrappedProperty::eType_Int;
                value.property.floatValue = pMaterial->GetShaderResources().m_LMaterial.m_Smoothness;
                value.property.intValue = (int)value.property.floatValue;
            }
            else if (propertyName == "Specular Level")
            {
                value.type = SPyWrappedProperty::eType_Float;
                value.property.floatValue = pMaterial->GetShaderResources().m_LMaterial.m_Specular.a;
            }
            else if (propertyName == "Emissive Color")
            {
                value.type = SPyWrappedProperty::eType_Color;
                QColor col = ColorLinearToGamma(ColorF(
                            pMaterial->GetShaderResources().m_LMaterial.m_Emittance.r,
                            pMaterial->GetShaderResources().m_LMaterial.m_Emittance.g,
                            pMaterial->GetShaderResources().m_LMaterial.m_Emittance.b));
                value.property.colorValue.r = col.red();
                value.property.colorValue.g = col.green();
                value.property.colorValue.b = col.blue();
            }
            else if (propertyName == "Emissive Intensity")
            {
                value.type = SPyWrappedProperty::eType_Float;
                value.property.floatValue = pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a;
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid lighting setting.").toUtf8().data());
            }
        }
        // ########## Advanced ##########
        else if (categoryName == "Advanced")
        {
            if (propertyName == "Allow layer activation")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->LayerActivationAllowed();
            }
            else if (propertyName == "2 Sided")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_2SIDED;
            }
            else if (propertyName ==  "No Shadow")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_NOSHADOW;
            }
            else if (propertyName == "Use Scattering")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_SCATTER;
            }
            else if (propertyName == "Hide After Breaking")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_HIDEONBREAK;
            }
            else if (propertyName == "Fog Volume Shading Quality High")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH;
            }
            else if (propertyName == "Blend Terrain Color")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetFlags() & MTL_FLAG_BLEND_TERRAIN;
            }
            else if (propertyName == "Voxel Coverage")
            {
                value.type = SPyWrappedProperty::eType_Float;
                value.property.floatValue = (float)pMaterial->GetShaderResources().m_VoxelCoverage / 255.0f;
            }
            else if (propertyName == "Link to Material")
            {
                value.type = SPyWrappedProperty::eType_String;
                value.stringValue = pMaterial->GetMatInfo()->GetMaterialLinkName();
            }
            else if (propertyName == "Propagate Material Settings")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_MATERIAL_SETTINGS;
            }
            else if (propertyName == "Propagate Opacity Settings")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_OPACITY;
            }
            else if (propertyName == "Propagate Lighting Settings")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_LIGHTING;
            }
            else if (propertyName == "Propagate Advanced Settings")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_ADVANCED;
            }
            else if (propertyName == "Propagate Texture Maps")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_TEXTURES;
            }
            else if (propertyName == "Propagate Shader Params")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_SHADER_PARAMS;
            }
            else if (propertyName == "Propagate Shader Generation")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_SHADER_GEN;
            }
            else if (propertyName == "Propagate Vertex Deformation")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_VERTEX_DEF;
            }
            else if (propertyName == "Propagate Layer Presets")
            {
                value.type = SPyWrappedProperty::eType_Bool;
                value.property.boolValue = pMaterial->GetPropagationFlags() & MTL_PROPAGATE_LAYER_PRESETS;
            }
            else
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid advanced setting.").toUtf8().data());
            }
        }
        // ########## Texture Maps ##########
        else if (categoryName == "Texture Maps")
        {
            SInputShaderResources&      shaderResources = pMaterial->GetShaderResources();
            // ########## Texture Maps / [name] ##########
            if (splittedPropertyPath.size() == 2)
            {
                value.type = SPyWrappedProperty::eType_String;

                uint16          nSlot = (uint16)TryConvertingCStringToEEfResTextures(propertyName);
                SEfResTexture*  pTextureRes = shaderResources.GetTextureResource(nSlot);
                if (!pTextureRes || pTextureRes->m_Name.empty())
                {
                    AZ_Warning("ShadersSystem", false, "PyGetProperty - Error: empty texture slot [%d] (or missing name) for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                    value.stringValue = "";
                }
                else
                {
                    value.stringValue = pTextureRes->m_Name.c_str();
                }
            }
            // ########## Texture Maps / [TexType | Filter | IsProjectedTexGen | TexGenType ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                SEfResTexture*      pTextureRes = shaderResources.GetTextureResource(TryConvertingCStringToEEfResTextures(subCategoryName));
                if (pTextureRes)
                {
                    if (propertyName == "TexType")
                    {
                        value.type = SPyWrappedProperty::eType_String;
                        value.stringValue = TryConvertingETEX_TypeToCString(pTextureRes->m_Sampler.m_eTexType);
                    }
                    else if (propertyName == "Filter")
                    {
                        value.type = SPyWrappedProperty::eType_String;
                        value.stringValue = TryConvertingTexFilterToCString(pTextureRes->m_Filter);
                    }
                    else if (propertyName == "IsProjectedTexGen")
                    {
                        value.type = SPyWrappedProperty::eType_Bool;
                        value.property.boolValue = pTextureRes->AddModificator()->m_bTexGenProjected;
                    }
                    else if (propertyName == "TexGenType")
                    {
                        value.type = SPyWrappedProperty::eType_String;
                        value.stringValue = TryConvertingETexGenTypeToCString(pTextureRes->AddModificator()->m_eTGType);
                    }
                    else
                    {
                        throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                    }
                }
                else
                {
                    uint16      nSlot = (uint16) TryConvertingCStringToEEfResTextures(subCategoryName);
                    AZ_Warning("ShadersSystem", false, "PyGetProperty - Error: empty 'subCategoryName' texture slot [%d] for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                }
            }
            // ########## Texture Maps / [Tiling | Rotator | Oscillator] ##########
            else if (splittedPropertyPath.size() == 4)
            {
                SEfResTexture*  pTextureRes = shaderResources.GetTextureResource(TryConvertingCStringToEEfResTextures(subSubCategoryName));
                if (pTextureRes)
                {
                    if (subCategoryName == "Tiling")
                    {
                        if (propertyName == "IsTileU")
                        {
                            value.type = SPyWrappedProperty::eType_Bool;
                            value.property.boolValue = pTextureRes->m_bUTile;
                        }
                        else if (propertyName == "IsTileV")
                        {
                            value.type = SPyWrappedProperty::eType_Bool;
                            value.property.boolValue = pTextureRes->m_bVTile;
                        }
                        else if (propertyName == "TileU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_Tiling[0];
                        }
                        else if (propertyName == "TileV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_Tiling[1];
                        }
                        else if (propertyName == "OffsetU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_Offs[0];
                        }
                        else if (propertyName == "OffsetV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_Offs[1];
                        }
                        else if (propertyName == "RotateU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = Word2Degr(pTextureRes->AddModificator()->m_Rot[0]);
                        }
                        else if (propertyName == "RotateV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = Word2Degr(pTextureRes->AddModificator()->m_Rot[1]);
                        }
                        else if (propertyName == "RotateW")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = Word2Degr(pTextureRes->AddModificator()->m_Rot[2]);
                        }
                        else
                        {
                            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                        }
                    }
                    else if (subCategoryName == "Rotator")
                    {
                        if (propertyName == "Type")
                        {
                            value.type = SPyWrappedProperty::eType_String;
                            value.stringValue = TryConvertingETexModRotateTypeToCString(pTextureRes->AddModificator()->m_eRotType);
                        }
                        else if (propertyName == "Rate")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = Word2Degr(pTextureRes->AddModificator()->m_RotOscRate[2]);
                        }
                        else if (propertyName == "Phase")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = Word2Degr(pTextureRes->AddModificator()->m_RotOscPhase[2]);
                        }
                        else if (propertyName == "Amplitude")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = Word2Degr(pTextureRes->AddModificator()->m_RotOscAmplitude[2]);
                        }
                        else if (propertyName == "CenterU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_RotOscCenter[0];
                        }
                        else if (propertyName == "CenterV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_RotOscCenter[1];
                        }
                        else
                        {
                            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                        }
                    }
                    else if (subCategoryName == "Oscillator")
                    {
                        if (propertyName == "TypeU")
                        {
                            value.type = SPyWrappedProperty::eType_String;
                            value.stringValue = TryConvertingETexModMoveTypeToCString(pTextureRes->AddModificator()->m_eMoveType[0]);
                        }
                        else if (propertyName == "TypeV")
                        {
                            value.type = SPyWrappedProperty::eType_String;
                            value.stringValue = TryConvertingETexModMoveTypeToCString(pTextureRes->AddModificator()->m_eMoveType[1]);
                        }
                        else if (propertyName == "RateU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_OscRate[0];
                        }
                        else if (propertyName == "RateV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_OscRate[1];
                        }
                        else if (propertyName == "PhaseU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_OscPhase[0];
                        }
                        else if (propertyName == "PhaseV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_OscPhase[1];
                        }
                        else if (propertyName == "AmplitudeU")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_OscAmplitude[0];
                        }
                        else if (propertyName == "AmplitudeV")
                        {
                            value.type = SPyWrappedProperty::eType_Float;
                            value.property.floatValue = pTextureRes->AddModificator()->m_OscAmplitude[1];
                        }
                        else
                        {
                            throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                        }
                    }
                    else
                    {
                        throw std::runtime_error((QString("\"") + subCategoryName + "\" is an invalid sub category.").toUtf8().data());
                    }
                }
                else
                {
                    uint16      nSlot = (uint16)TryConvertingCStringToEEfResTextures(subSubCategoryName);
                    AZ_Warning("ShadersSystem", false, "PyGetProperty - Error: empty 'subSubCategoryName' texture slot [%d] for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                }
            }
            else
            {
                throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
            }
        }
        // ########## Shader Params ##########
        else if (categoryName == "Shader Params")
        {
            DynArray<SShaderParam>& shaderParams = pMaterial->GetShaderResources().m_ShaderParams;
            bool isPropertyFound(false);

            for (int i = 0; i < shaderParams.size(); i++)
            {
                if (propertyName == ParseUINameFromPublicParamsScript(shaderParams[i].m_Script.c_str()))
                {
                    if (shaderParams[i].m_Type == eType_FLOAT)
                    {
                        value.type = SPyWrappedProperty::eType_Float;
                        value.property.floatValue = shaderParams[i].m_Value.m_Float;
                        isPropertyFound = true;
                        break;
                    }
                    else if (shaderParams[i].m_Type == eType_FCOLOR)
                    {
                        value.type = SPyWrappedProperty::eType_Color;
                        QColor col = ColorLinearToGamma(ColorF(
                                    shaderParams[i].m_Value.m_Vector[0],
                                    shaderParams[i].m_Value.m_Vector[1],
                                    shaderParams[i].m_Value.m_Vector[2]));
                        value.property.colorValue.r = col.red();
                        value.property.colorValue.g = col.green();
                        value.property.colorValue.b = col.blue();
                        isPropertyFound = true;
                        break;
                    }
                }
            }

            if (!isPropertyFound)
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid shader param.").toUtf8().data());
            }
        }
        // ########## Shader Generation Params ##########
        else if (categoryName == "Shader Generation Params")
        {
            value.type = SPyWrappedProperty::eType_Bool;
            bool isPropertyFound(false);
            for (int i = 0; i < pMaterial->GetShaderGenParamsVars()->GetNumVariables(); i++)
            {
                if (propertyName == pMaterial->GetShaderGenParamsVars()->GetVariable(i)->GetHumanName())
                {
                    isPropertyFound = true;
                    pMaterial->GetShaderGenParamsVars()->GetVariable(i)->Get(value.property.boolValue);
                    break;
                }
            }

            if (!isPropertyFound)
            {
                throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid shader generation param.").toUtf8().data());
            }
        }
        // ########## Vertex Deformation ##########
        else if (categoryName == "Vertex Deformation")
        {
            // ########## Vertex Deformation / [ Type | Wave Length X | Wave Length Y | Wave Length Z | Wave Length W | Noise Scale ] ##########
            if (splittedPropertyPath.size() == 2)
            {
                if (propertyName == "Type")
                {
                    value.type = SPyWrappedProperty::eType_String;
                    value.stringValue = TryConvertingEDeformTypeToCString(pMaterial->GetShaderResources().m_DeformInfo.m_eType);
                }
                else if (propertyName == "Wave Length X")
                {
                    value.type = SPyWrappedProperty::eType_Float;
                    value.property.floatValue = pMaterial->GetShaderResources().m_DeformInfo.m_fDividerX;
                }
                else if (propertyName == "Noise Scale")
                {
                    value.type = SPyWrappedProperty::eType_Vec3;
                    value.property.vecValue.x = pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[0];
                    value.property.vecValue.y = pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[1];
                    value.property.vecValue.z = pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[2];
                }
                else
                {
                    throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                }
            }
            // ########## Vertex Deformation / [ Wave X ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                if (subCategoryName == "Wave X")
                {
                    SWaveForm2 currentWaveForm;
                    if (subCategoryName == "Wave X")
                    {
                        currentWaveForm = pMaterial->GetShaderResources().m_DeformInfo.m_WaveX;
                    }

                    if (propertyName == "Type")
                    {
                        value.type = SPyWrappedProperty::eType_String;
                        value.stringValue = TryConvertingEWaveFormToCString(currentWaveForm.m_eWFType);
                    }
                    else if (propertyName == "Level")
                    {
                        value.type = SPyWrappedProperty::eType_Float;
                        value.property.floatValue = currentWaveForm.m_Level;
                    }
                    else if (propertyName == "Amplitude")
                    {
                        value.type = SPyWrappedProperty::eType_Float;
                        value.property.floatValue = currentWaveForm.m_Amp;
                    }
                    else if (propertyName == "Phase")
                    {
                        value.type = SPyWrappedProperty::eType_Float;
                        value.property.floatValue = currentWaveForm.m_Phase;
                    }
                    else if (propertyName == "Frequency")
                    {
                        value.type = SPyWrappedProperty::eType_Float;
                        value.property.floatValue = currentWaveForm.m_Freq;
                    }
                    else
                    {
                        throw std::runtime_error((QString("\"") + propertyName + "\" is an invalid property.").toUtf8().data());
                    }
                }
                else
                {
                    throw std::runtime_error((QString("\"") + categoryName + "\" is an invalid category.").toUtf8().data());
                }
            }
            else
            {
                throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
            }
        }
        // ########## Layer Presets ##########
        else if (categoryName == "Layer Presets")
        {
            // names are "Shader1", "Shader2" and "Shader3", bacause all have the name "Shader" in material editor
            if (splittedPropertyPath.size() == 2)
            {
                value.type = SPyWrappedProperty::eType_String;

                int shaderNumber = -1;
                if (propertyName == "Shader1")
                {
                    shaderNumber = 0;
                }
                else if (propertyName == "Shader2")
                {
                    shaderNumber = 1;
                }
                else if (propertyName == "Shader3")
                {
                    shaderNumber = 2;
                }
                else
                {
                    throw std::runtime_error("Invalid shader.");
                }

                value.stringValue = pMaterial->GetMtlLayerResources()[shaderNumber].m_shaderName;
            }
            else if (splittedPropertyPath.size() == 3)
            {
                if (propertyName == "No Draw")
                {
                    value.type = SPyWrappedProperty::eType_Bool;

                    int shaderNumber = -1;
                    if (subCategoryName == "Shader1")
                    {
                        shaderNumber = 0;
                    }
                    else if (subCategoryName == "Shader2")
                    {
                        shaderNumber = 1;
                    }
                    else if (subCategoryName == "Shader3")
                    {
                        shaderNumber = 2;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid shader.");
                    }
                    value.property.boolValue = pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags & MTL_LAYER_USAGE_NODRAW;
                }
            }
            else
            {
                throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
            }
        }
        else
        {
            throw std::runtime_error(errorMsgInvalidPropertyPath.toUtf8().data());
        }

        return value;
    }

    void PySetProperty(const char* pPathAndMaterialName, const char* pPathAndPropertyName, const SPyWrappedProperty& value)
    {
        CMaterial* pMaterial = TryLoadingMaterial(pPathAndMaterialName);
        std::deque<QString> splittedPropertyPath = PreparePropertyPath(pPathAndPropertyName);
        std::deque<QString> splittedPropertyPathCategory = splittedPropertyPath;
        QString categoryName = splittedPropertyPath.front();
        QString subCategoryName = "None";
        QString subSubCategoryName = "None";
        QString propertyName = splittedPropertyPath.back();
        QString errorMsgInvalidPropertyPath = "Invalid property path.";

        if (!ValidateProperty(pMaterial, splittedPropertyPath, value))
        {
            throw std::runtime_error("Invalid property.");
        }

        QString undoMsg = "Set Material Property";
        CUndo undo(undoMsg.toUtf8().data());
        pMaterial->RecordUndo(undoMsg.toUtf8().data(), true);

        if (splittedPropertyPathCategory.size() == 3)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
        }
        else if (splittedPropertyPathCategory.size() == 4)
        {
            splittedPropertyPathCategory.pop_back();
            subCategoryName = splittedPropertyPathCategory.back();
            splittedPropertyPathCategory.pop_back();
            subSubCategoryName = splittedPropertyPathCategory.back();
        }

        // ########## Material Settings ##########
        if (categoryName == "Material Settings")
        {
            if (propertyName == "Shader")
            {
                pMaterial->SetShaderName(value.stringValue);
            }
            else if (propertyName == "Surface Type")
            {
                bool isSurfaceExist(false);
                QString realSurfacename = "";
                ISurfaceTypeEnumerator* pSurfaceTypeEnum = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetEnumerator();
                if (pSurfaceTypeEnum)
                {
                    for (ISurfaceType* pSurfaceType = pSurfaceTypeEnum->GetFirst(); pSurfaceType; pSurfaceType = pSurfaceTypeEnum->GetNext())
                    {
                        QString surfaceName = pSurfaceType->GetName();
                        realSurfacename = surfaceName;
                        if (surfaceName.left(4) == "mat_")
                        {
                            surfaceName.remove(0, 4);
                        }
                        if (surfaceName == value.stringValue)
                        {
                            isSurfaceExist = true;
                            pMaterial->SetSurfaceTypeName(realSurfacename);
                        }
                    }

                    if (!isSurfaceExist)
                    {
                        throw std::runtime_error("Invalid surface type name.");
                    }
                }
                else
                {
                    throw std::runtime_error("Surface Type Enumerator corrupted.");
                }
            }
        }
        // ########## Opacity Settings ##########
        else if (categoryName == "Opacity Settings")
        {
            if (propertyName == "Opacity")
            {
                pMaterial->GetShaderResources().m_LMaterial.m_Opacity = static_cast<float>(value.property.intValue) / 100.0f;
            }
            else if (propertyName == "AlphaTest")
            {
                pMaterial->GetShaderResources().m_AlphaRef =  static_cast<float>(value.property.intValue) / 100.0f;
            }
            else if (propertyName == "Additive")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_ADDITIVE, value.property.boolValue);
            }
        }
        // ########## Lighting Settings ##########
        else if (categoryName == "Lighting Settings")
        {
            if (propertyName == "Diffuse Color")
            {
                ColorB color(value.property.colorValue.r, value.property.colorValue.g, value.property.colorValue.b);
                pMaterial->GetShaderResources().m_LMaterial.m_Diffuse = ColorGammaToLinear(QColor(color.r, color.g, color.b));
            }
            else if (propertyName == "Specular Color")
            {
                ColorB color(value.property.colorValue.r, value.property.colorValue.g, value.property.colorValue.b);
                ColorF colorFloat = ColorGammaToLinear(QColor(color.r, color.g, color.b));
                colorFloat.a = pMaterial->GetShaderResources().m_LMaterial.m_Specular.a;
                colorFloat.r *= colorFloat.a;
                colorFloat.g *= colorFloat.a;
                colorFloat.b *= colorFloat.a;
                pMaterial->GetShaderResources().m_LMaterial.m_Specular = colorFloat;
            }
            else if (propertyName == "Glossiness")
            {
                pMaterial->GetShaderResources().m_LMaterial.m_Smoothness = static_cast<float>(value.property.intValue);
            }
            else if (propertyName == "Specular Level")
            {
                float tempFloat = pMaterial->GetShaderResources().m_LMaterial.m_Specular.a;
                float tempFloat2 = value.property.floatValue;


                ColorF colorFloat = pMaterial->GetShaderResources().m_LMaterial.m_Specular;
                colorFloat.a = value.property.floatValue;
                colorFloat.r *= colorFloat.a;
                colorFloat.g *= colorFloat.a;
                colorFloat.b *= colorFloat.a;
                colorFloat.a = 1.0f;
                pMaterial->GetShaderResources().m_LMaterial.m_Specular = colorFloat;
            }
            else if (propertyName == "Emissive Color")
            {
                ColorB color(value.property.colorValue.r, value.property.colorValue.g, value.property.colorValue.b);
                float emissiveIntensity = pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a;
                pMaterial->GetShaderResources().m_LMaterial.m_Emittance = ColorGammaToLinear(QColor(color.r, color.g, color.b));
                pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a = emissiveIntensity;
            }
            else if (propertyName == "Emissive Intensity")
            {
                pMaterial->GetShaderResources().m_LMaterial.m_Emittance.a = value.property.floatValue;
            }
        }
        // ########## Advanced ##########
        else if (categoryName == "Advanced")
        {
            if (propertyName == "Allow layer activation")
            {
                pMaterial->SetLayerActivation(value.property.boolValue);
            }
            else if (propertyName == "2 Sided")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_2SIDED, value.property.boolValue);
            }
            else if (propertyName ==  "No Shadow")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_NOSHADOW, value.property.boolValue);
            }
            else if (propertyName == "Use Scattering")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_SCATTER, value.property.boolValue);
            }
            else if (propertyName == "Hide After Breaking")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_HIDEONBREAK, value.property.boolValue);
            }
            else if (propertyName == "Fog Volume Shading Quality High")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH, value.property.boolValue);
            }
            else if (propertyName == "Blend Terrain Color")
            {
                SetMaterialFlag(pMaterial, MTL_FLAG_BLEND_TERRAIN, value.property.boolValue);
            }
            else if (propertyName == "Voxel Coverage")
            {
                pMaterial->GetShaderResources().m_VoxelCoverage = static_cast<uint8>(value.property.floatValue * 255.0f);
            }
            else if (propertyName == "Link to Material")
            {
                pMaterial->GetMatInfo()->SetMaterialLinkName(value.stringValue.toUtf8().data());
            }
            else if (propertyName == "Propagate Material Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_MATERIAL_SETTINGS, value.property.boolValue);
            }
            else if (propertyName == "Propagate Opacity Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_OPACITY, value.property.boolValue);
            }
            else if (propertyName == "Propagate Lighting Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_LIGHTING, value.property.boolValue);
            }
            else if (propertyName == "Propagate Advanced Settings")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_ADVANCED, value.property.boolValue);
            }
            else if (propertyName == "Propagate Texture Maps")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_TEXTURES, value.property.boolValue);
            }
            else if (propertyName == "Propagate Shader Params")
            {
                if (value.property.boolValue == true)
                {
                    SetPropagationFlag(pMaterial, MTL_PROPAGATE_MATERIAL_SETTINGS, true);
                }
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_SHADER_PARAMS, value.property.boolValue);
            }
            else if (propertyName == "Propagate Shader Generation")
            {
                if (value.property.boolValue == true)
                {
                    SetPropagationFlag(pMaterial, MTL_PROPAGATE_MATERIAL_SETTINGS, true);
                }
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_SHADER_GEN, value.property.boolValue);
            }
            else if (propertyName == "Propagate Vertex Deformation")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_VERTEX_DEF, value.property.boolValue);
            }
            else if (propertyName == "Propagate Layer Presets")
            {
                SetPropagationFlag(pMaterial, MTL_PROPAGATE_LAYER_PRESETS, value.property.boolValue);
            }
        }
        // ########## Texture Maps ##########
        else if (categoryName == "Texture Maps")
        {
            // ########## Texture Maps / [name] ##########
            SInputShaderResources&      shaderResources = pMaterial->GetShaderResources();
            if (splittedPropertyPath.size() == 2)
            {
                uint16      nSlot = TryConvertingCStringToEEfResTextures(propertyName);
                if (value.stringValue.length() == 0)
                {
                    AZ_Warning("ShadersSystem", false, "PySetProperty - Error: empty texture [%d] name for material %s",
                        nSlot, pMaterial->GetName().toStdString().c_str());
                }
                // notice that the following is an insertion operation if the index did not exist in the map
                shaderResources.m_TexturesResourcesMap[nSlot].m_Name = value.stringValue.toUtf8().data();
            }
            // ########## Texture Maps / [TexType | Filter | IsProjectedTexGen | TexGenType ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                uint16              nSlot = TryConvertingCStringToEEfResTextures(subCategoryName);
                // notice that each of the following will add the texture slot if did not exist yet
                if (propertyName == "TexType")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].m_Sampler.m_eTexType = TryConvertingCStringToETEX_Type(value.stringValue);
                }
                else if (propertyName == "Filter")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].m_Filter = TryConvertingCStringToTexFilter(value.stringValue);
                }
                else if (propertyName == "IsProjectedTexGen")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_bTexGenProjected = value.property.boolValue;
                }
                else if (propertyName == "TexGenType")
                {
                    shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eTGType = TryConvertingCStringToETexGenType(value.stringValue);
                }
            }
            // ########## Texture Maps / [Tiling | Rotator | Oscillator] ##########
            else if (splittedPropertyPath.size() == 4)
            {
                uint16              nSlot = TryConvertingCStringToEEfResTextures(subSubCategoryName);
                if (subCategoryName == "Tiling")
                {
                    if (propertyName == "IsTileU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].m_bUTile = value.property.boolValue;
                    }
                    else if (propertyName == "IsTileV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].m_bVTile = value.property.boolValue;
                    }
                    else if (propertyName == "TileU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Tiling[0] = value.property.floatValue;
                    }
                    else if (propertyName == "TileV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Tiling[1] = value.property.floatValue;
                    }
                    else if (propertyName == "OffsetU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Offs[0] = value.property.floatValue;
                    }
                    else if (propertyName == "OffsetV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Offs[1] = value.property.floatValue;
                    }
                    else if (propertyName == "RotateU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Rot[0] = Degr2Word(value.property.floatValue);
                    }
                    else if (propertyName == "RotateV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Rot[1] = Degr2Word(value.property.floatValue);
                    }
                    else if (propertyName == "RotateW")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_Rot[2] = Degr2Word(value.property.floatValue);
                    }
                }
                else if (subCategoryName == "Rotator")
                {
                    if (propertyName == "Type")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eRotType = TryConvertingCStringToETexModRotateType(value.stringValue);
                    }
                    else if (propertyName == "Rate")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscRate[2] = Degr2Word(value.property.floatValue);
                    }
                    else if (propertyName == "Phase")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscPhase[2] = Degr2Word(value.property.floatValue);
                    }
                    else if (propertyName == "Amplitude")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscAmplitude[2] = Degr2Word(value.property.floatValue);
                    }
                    else if (propertyName == "CenterU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscCenter[0] = value.property.floatValue;
                    }
                    else if (propertyName == "CenterV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_RotOscCenter[1] = value.property.floatValue;
                    }
                }
                else if (subCategoryName == "Oscillator")
                {
                    if (propertyName == "TypeU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eMoveType[0] = TryConvertingCStringToETexModMoveType(value.stringValue);
                    }
                    else if (propertyName == "TypeV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_eMoveType[1] = TryConvertingCStringToETexModMoveType(value.stringValue);
                    }
                    else if (propertyName == "RateU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscRate[0] = value.property.floatValue;
                    }
                    else if (propertyName == "RateV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscRate[1] = value.property.floatValue;
                    }
                    else if (propertyName == "PhaseU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscPhase[0] = value.property.floatValue;
                    }
                    else if (propertyName == "PhaseV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscPhase[1] = value.property.floatValue;
                    }
                    else if (propertyName == "AmplitudeU")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscAmplitude[0] = value.property.floatValue;
                    }
                    else if (propertyName == "AmplitudeV")
                    {
                        shaderResources.m_TexturesResourcesMap[nSlot].AddModificator()->m_OscAmplitude[1] = value.property.floatValue;
                    }
                }
            }
        }
        // ########## Shader Params ##########
        else if (categoryName == "Shader Params")
        {
            DynArray<SShaderParam>& shaderParams = pMaterial->GetShaderResources().m_ShaderParams;

            for (int i = 0; i < shaderParams.size(); i++)
            {
                if (propertyName == ParseUINameFromPublicParamsScript(shaderParams[i].m_Script.c_str()))
                {
                    if (shaderParams[i].m_Type == eType_FLOAT)
                    {
                        shaderParams[i].m_Value.m_Float = value.property.floatValue;
                        break;
                    }
                    else if (shaderParams[i].m_Type == eType_FCOLOR)
                    {
                        ColorF colorLinear = ColorGammaToLinear(
                                QColor(
                                    (uint8)value.property.colorValue.r,
                                    (uint8)value.property.colorValue.g,
                                    (uint8)value.property.colorValue.b
                                    ));

                        shaderParams[i].m_Value.m_Vector[0] = colorLinear.r;
                        shaderParams[i].m_Value.m_Vector[1] = colorLinear.g;
                        shaderParams[i].m_Value.m_Vector[2] = colorLinear.b;
                        break;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid data type (Shader Params)");
                    }
                }
            }
        }
        // ########## Shader Generation Params ##########
        else if (categoryName == "Shader Generation Params")
        {
            for (int i = 0; i < pMaterial->GetShaderGenParamsVars()->GetNumVariables(); i++)
            {
                if (propertyName == pMaterial->GetShaderGenParamsVars()->GetVariable(i)->GetHumanName())
                {
                    CVarBlock* shaderGenBlock = pMaterial->GetShaderGenParamsVars();
                    shaderGenBlock->GetVariable(i)->Set(value.property.boolValue);
                    pMaterial->SetShaderGenParamsVars(shaderGenBlock);
                    break;
                }
            }
        }
        // ########## Vertex Deformation ##########
        else if (categoryName == "Vertex Deformation")
        {
            // ########## Vertex Deformation / [ Type | Wave Length X | Wave Length Y | Wave Length Z | Wave Length W | Noise Scale ] ##########
            if (splittedPropertyPath.size() == 2)
            {
                if (propertyName == "Type")
                {
                    pMaterial->GetShaderResources().m_DeformInfo.m_eType = TryConvertingCStringToEDeformType(value.stringValue);
                }
                else if (propertyName == "Wave Length X")
                {
                    pMaterial->GetShaderResources().m_DeformInfo.m_fDividerX = value.property.floatValue;
                }
                else if (propertyName == "Noise Scale")
                {
                    pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[0] = value.property.vecValue.x;
                    pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[1] = value.property.vecValue.y;
                    pMaterial->GetShaderResources().m_DeformInfo.m_vNoiseScale[2] = value.property.vecValue.z;
                }
            }
            // ########## Vertex Deformation / [ Wave X ] ##########
            else if (splittedPropertyPath.size() == 3)
            {
                if (subCategoryName == "Wave X")
                {
                    SWaveForm2& currentWaveForm = pMaterial->GetShaderResources().m_DeformInfo.m_WaveX;

                    if (propertyName == "Type")
                    {
                        currentWaveForm.m_eWFType = TryConvertingCStringToEWaveForm(value.stringValue);
                    }
                    else if (propertyName == "Level")
                    {
                        currentWaveForm.m_Level = value.property.floatValue;
                    }
                    else if (propertyName == "Amplitude")
                    {
                        currentWaveForm.m_Amp = value.property.floatValue;
                    }
                    else if (propertyName == "Phase")
                    {
                        currentWaveForm.m_Phase = value.property.floatValue;
                    }
                    else if (propertyName == "Frequency")
                    {
                        currentWaveForm.m_Freq = value.property.floatValue;
                    }
                }
            }
        }
        // ########## Layer Presets ##########
        else if (categoryName == "Layer Presets")
        {
            // names are "Shader1", "Shader2" and "Shader3", bacause all have the name "Shader" in material editor
            if (splittedPropertyPath.size() == 2)
            {
                int shaderNumber = -1;
                if (propertyName == "Shader1")
                {
                    shaderNumber = 0;
                }
                else if (propertyName == "Shader2")
                {
                    shaderNumber = 1;
                }
                else if (propertyName == "Shader3")
                {
                    shaderNumber = 2;
                }

                pMaterial->GetMtlLayerResources()[shaderNumber].m_shaderName = value.stringValue;
            }
            else if (splittedPropertyPath.size() == 3)
            {
                if (propertyName == "No Draw")
                {
                    int shaderNumber = -1;
                    if (subCategoryName == "Shader1")
                    {
                        shaderNumber = 0;
                    }
                    else if (subCategoryName == "Shader2")
                    {
                        shaderNumber = 1;
                    }
                    else if (subCategoryName == "Shader3")
                    {
                        shaderNumber = 2;
                    }

                    if (pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags & MTL_LAYER_USAGE_NODRAW && value.property.boolValue == false)
                    {
                        pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags = pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags - MTL_LAYER_USAGE_NODRAW;
                    }
                    else if (!(pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags & MTL_LAYER_USAGE_NODRAW) && value.property.boolValue == true)
                    {
                        pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags = pMaterial->GetMtlLayerResources()[shaderNumber].m_nFlags | MTL_LAYER_USAGE_NODRAW;
                    }
                }
            }
        }

        pMaterial->Update();
        pMaterial->Save();
        GetIEditor()->GetMaterialManager()->OnUpdateProperties(pMaterial, true);
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialCreate, material, create,
    "Creates a material.",
    "material.create()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialCreateMulti, material, create_multi,
    "Creates a multi-material.",
    "material.create_multi()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialConvertToMulti, material, convert_to_multi,
    "Converts the selected material to a multi-material.",
    "material.convert_to_multi()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialDuplicateCurrent, material, duplicate_current,
    "Duplicates the current material.",
    "material.duplicate_current()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialMergeSelection, material, merge_selection,
    "Merges the selected materials.",
    "material.merge_selection()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialDeleteCurrent, material, delete_current,
    "Deletes the current material.",
    "material.delete_current()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialAssignCurrentToSelection, material, assign_current_to_selection,
    "Assigns the current material to the selection.",
    "material.assign_current_to_selection()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialResetSelection, material, reset_selection,
    "Resets the materials of the selection.",
    "material.reset_selection()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialSelectObjectsWithCurrent, material, select_objects_with_current,
    "Selects the objects which have the current material.",
    "material.select_objects_with_current()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyMaterialSetCurrentFromObject, material, set_current_from_object,
    "Sets the current material to the material of a selected object.",
    "material.set_current_from_object()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetSubMaterial, material, get_submaterial,
    "Gets sub materials of an material.",
    "material.get_submaterial()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetProperty, material, get_property,
    "Gets a property of a material.",
    "material.get_property(str materialPath/materialName, str propertyPath/propertyName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetProperty, material, set_property,
    "Sets a property of a material.",
    "material.set_property(str materialPath/materialName, str propertyPath/propertyName, [ str | (int, int, int) | (float, float, float) | int | float | bool ] value)");
