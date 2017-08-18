// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TO_GLSL_OPERAND_H
#define TO_GLSL_OPERAND_H

#include "internal_includes/structs.h"

#define TO_FLAG_NONE              0x0
#define TO_FLAG_INTEGER           0x1
#define TO_FLAG_NAME_ONLY         0x2
#define TO_FLAG_DECLARATION_NAME  0x4
#define TO_FLAG_DESTINATION       0x8 //Operand is being written to by assignment.
#define TO_FLAG_UNSIGNED_INTEGER  0x10
#define TO_FLAG_DOUBLE            0x20
#define TO_FLAG_FLOAT             0x40
#define TO_FLAG_COPY              0x80

void TranslateOperand(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag);

int GetMaxComponentFromComponentMask(const Operand* psOperand);
void TranslateOperandIndex(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index);
void TranslateOperandIndexMAD(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index, uint32_t multiply, uint32_t add);
void TranslateVariableName(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle);
void TranslateOperandSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand);

uint32_t GetNumSwizzleElements(const Operand* psOperand);
void AddSwizzleUsingElementCount(HLSLCrossCompilerContext* psContext, uint32_t count);
int GetFirstOperandSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
uint32_t IsSwizzleReplacated(const Operand* psOperand);

void TextureName(bstring output, Shader* psShader, const uint32_t ui32TextureRegister, const uint32_t ui32SamplerRegister, const int bCompare);
void UAVName(bstring output, Shader* psShader, const uint32_t ui32RegisterNumber);
void UniformBufferName(bstring output, Shader* psShader, const uint32_t ui32RegisterNumber);

void ConvertToTextureName(bstring output, Shader* psShader, const char* szName, const char* szSamplerName, const int bCompare);
void ConvertToUAVName(bstring output, Shader* psShader, const char* szOriginalUAVName);
void ConvertToUniformBufferName(bstring output, Shader* psShader, const char* szConstantBufferName);

void ShaderVarName(bstring output, Shader* psShader, const char* OriginalName);
void ShaderVarFullName(bstring output, Shader* psShader, const ShaderVarType* psShaderVar);

uint32_t ConvertOperandSwizzleToComponentMask(const Operand* psOperand);
//Non-zero means the components overlap
int CompareOperandSwizzles(const Operand* psOperandA, const Operand* psOperandB);

SHADER_VARIABLE_TYPE GetOperandDataType(HLSLCrossCompilerContext* psContext, const Operand* psOperand);


// NOTE: CODE DUPLICATION FROM HLSLcc METAL ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TranslateGmemOperandSwizzleWithMask(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask, uint32_t gmemNumElements);
uint32_t GetGmemInputResourceSlot(uint32_t const slotIn);
uint32_t GetGmemInputResourceNumElements(uint32_t const slotIn);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
