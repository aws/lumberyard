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


#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <MathConversion.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>


namespace AZ
{
	class ReflectContext;
}

struct IMaterial;

namespace StarterGameGem
{

	/*!
	* Wrapper for material utility functions exposed to Lua for StarterGame.
	*/
	class StarterGameMaterialUtility
	{
	public:
		AZ_TYPE_INFO(StarterGameMaterialUtility, "{2C135D40-3323-4BCB-8B34-3D33A86E7E35}");
		AZ_CLASS_ALLOCATOR(StarterGameMaterialUtility, AZ::SystemAllocator, 0);

		static void Reflect(AZ::ReflectContext* reflection);

		static bool SetShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName, float var);
        static bool SetSubMtlShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex, float var);
        static bool SetShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName, const AZ::Vector3& var);
        static bool SetSubMtlShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex, const AZ::Vector3& var);
        static float GetShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName);
        static float GetSubMtlShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex);
        static AZ::Vector3 GetShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName);
        static AZ::Vector3 GetSubMtlShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex);

		static bool ReplaceMaterialWithClone(AZ::EntityId entityId);
		static void RestoreOriginalMaterial(AZ::EntityId entityId);

    private:
        static _smart_ptr<IMaterial> GetMaterial(AZ::EntityId entityId);
        static bool SetMaterialParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, UParamVal var, EParamType type);
        static bool GetMaterialParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, float& value);
        static bool GetMaterialParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, AZ::Vector3& value);
        static bool SetShaderParam(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, UParamVal var);
        static bool GetShaderParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, float& value);
        static bool GetShaderParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, AZ::Vector3& value);
        static bool SetShaderMatFloat(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, float var);
        static bool SetShaderMatVec3(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, const AZ::Vector3& var);
        static bool GetShaderMatFloat(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, float& var);
        static bool GetShaderMatVec3(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, AZ::Vector3& var);

    };

} // namespace StarterGameGem
