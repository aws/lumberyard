set(THIS_PROJECT CryRenderNULL)

project( ${THIS_PROJECT} )


# 
set (SourceGroup_root
)

# Source Files
set (SourceGroup_Source_Files
	../Common/Renderer_SPU.cpp
	NULL_DevBuffer.cpp
	NULL_Font.cpp
	NULL_PostProcess.cpp
	NULL_Renderer.cpp
	NULL_RendPipeline.cpp
	NULL_RERender.cpp
	NULL_Shaders.cpp
	NULL_Shadows.cpp
	NULL_System.cpp
	NULL_Textures.cpp
	NULL_TexturesStreaming.cpp
	NULLRenderAuxGeom.cpp
	../StdAfx.cpp
)

# Header Files
set (SourceGroup_Header_Files
	NULL_Renderer.h
	NULLRenderAuxGeom.h
	../StdAfx.h
)

# Common
set (SourceGroup_Common
	../Common/CommonRender.cpp
	../../CryCommon/EngineSettingsManager.cpp
	../Common/IntroMovieRenderer.cpp
	../Common/lzss.cpp
	../Common/MatrixStack.cpp
	../Common/Renderer.cpp
	../Common/RenderPipeline.cpp
	../Common/RenderThread.cpp
	../Common/ResFile.cpp
	../Common/ResFileLookupDataMan.cpp
	../Common/ShadowUtils.cpp
	../Common/TextMessages.cpp
	../Common/WaterUtils.cpp
	../../CryCommon/EngineSettingsManager.h
	../Common/IntroMovieRenderer.h
	../Common/ResFileLookupDataMan.h
	../Common/TextMessages.h
)

# Common -> RendElements
set (SourceGroup_Common_RendElements
	../Common/RendElements/CREBreakableGlass.cpp
	CRELensOpticsNULL.cpp
	../Common/RendElements/CREBeam.cpp
	../Common/RendElements/CREClientPoly.cpp
	../Common/RendElements/CRECloud.cpp
	../Common/RendElements/CREDeferredShading.cpp
	../Common/RendElements/CREFarTreeSprites.cpp
	../Common/RendElements/CREFogVolume.cpp
	../Common/RendElements/CREHDRProcess.cpp
	../Common/RendElements/CREImposter.cpp
	../Common/RendElements/CREMesh.cpp
	../Common/RendElements/CREOclusionQuery.cpp
	../Common/RendElements/CREParticle.cpp
	../Common/RendElements/CREPostProcess.cpp
	../Common/RendElements/CRESky.cpp
	../Common/RendElements/CREVolumeObject.cpp
	../Common/RendElements/CREWaterOcean.cpp
	../Common/RendElements/CREWaterVolume.cpp
	../Common/RendElements/RendElement.cpp
)

# Common -> RendElements -> RendElements_h
set (SourceGroup_Common_RendElements_RendElements_h
	../Common/RendElements/CRELensOptics.h
	../Common/RendElements/CREBeam.h
	../Common/RendElements/CREClientPoly.h
	../Common/RendElements/CRECloud.h
	../Common/RendElements/CREDeferredShading.h
	../Common/RendElements/CREHDRProcess.h
	../Common/RendElements/Stars.h
)

# Common -> RendElements -> Utils
set (SourceGroup_Common_RendElements_Utils
	../Common/RendElements/Utils/PolygonMath2D.cpp
	../Common/RendElements/Utils/PolygonMath2D.h
	../Common/RendElements/Utils/SpatialHashGrid.h
)

# Common -> Common_h
set (SourceGroup_Common_Common_h
	../Common/CommonRender.h
	../Common/FrameProfiler.h
	../Common/IColorGradingControllerInt.h
	../Common/LZSS.H
	../Common/OcclQuery.h
	../Common/Renderer.h
	../Common/RenderPipeline.h
	../Common/RenderThread.h
	../Common/ResFile.h
	../Common/ResFile_info.h
	../Common/Shadow_Renderer.h
	../Common/ShadowUtils.h
	../Common/WaterUtils.h
)

# Common -> PostProcess
set (SourceGroup_Common_PostProcess
	../Common/PostProcess/PostEffects.cpp
	../Common/PostProcess/PostProcess.cpp
	../Common/PostProcess/PostProcessUtils.cpp
	../Common/PostProcess/PostEffects.h
	../Common/PostProcess/PostProcess.h
	../Common/PostProcess/PostProcessUtils.h
)

# Common -> RenderMesh
set (SourceGroup_Common_RenderMesh
	../Common/RenderMesh_Render.cpp
	../Common/RenderMesh_SetMesh.cpp
	../Common/RenderMesh.cpp
	../Common/RenderMesh.h
)

# Common -> Textures
set (SourceGroup_Common_Textures
	../../CryCommon/SettingsManagerHelpers.cpp
	../Common/Textures/ITextureStreamer.cpp
	../Common/Textures/PlanningTextureStreamer.cpp
	../Common/Textures/PlanningTextureStreamer_SPU.cpp
	../Common/Textures/StatoscopeTextureStreaming.cpp
	../Common/Textures/TextureCompiler.cpp
	../Common/Textures/DynTexture.cpp
	../Common/Textures/PowerOf2BlockPacker.cpp
	../../CryCommon/ResourceCompilerHelper.cpp
	../Common/Textures/Texture.cpp
	../Common/Textures/TextureManager.cpp
	../Common/Textures/TextureStreaming.cpp
	../../CryCommon/SettingsManagerHelpers.h
	../Common/Textures/PlanningTextureStreamer.h
	../Common/Textures/StatoscopeTextureStreaming.h
	../Common/Textures/TextureCompiler.h
	../Common/Textures/PowerOf2BlockPacker.h
	../Common/Textures/Texture.h
	../Common/Textures/TextureManager.h
)

# Common -> Textures -> Image
set (SourceGroup_Common_Textures_Image
	../Common/Textures/Image/CImage.cpp
	../Common/Textures/Image/DDSImage.cpp
	../Common/Textures/Image/JpgImage.cpp
	../Common/Textures/Image/TgaImage.cpp
)

# Common -> Textures -> Image -> Image_h
set (SourceGroup_Common_Textures_Image_Image_h
	../Common/Textures/Image/CImage.h
	../Common/Textures/Image/DDSImage.h
	../Common/Textures/Image/SHendian.h
)

# Common -> Shaders
set (SourceGroup_Common_Shaders
	../Common/Shaders/Parser.cpp
	../Common/Shaders/ParserBin.cpp
	../Common/Shaders/ShaderCache.cpp
	../Common/Shaders/ShaderComponents.cpp
	../Common/Shaders/ShaderCore.cpp
	../Common/Shaders/ShaderFXParseBin.cpp
	../Common/Shaders/ShaderParse.cpp
	../Common/Shaders/ShaderScript.cpp
	../Common/Shaders/ShaderTemplate.cpp
	../Common/Shaders/CShader.h
	../Common/Shaders/CShaderBin.h
	../Common/Shaders/Parser.h
	../Common/Shaders/ParserBin.h
	../Common/Shaders/Shader.h
	../Common/Shaders/ShaderCache.h
	../Common/Shaders/ShaderComponents.h
)

# Common -> Lzma
set (SourceGroup_Common_Lzma
	../Common/Lzma/Alloc.c
	../Common/Lzma/Bra.c
	../Common/Lzma/Bra86.c
	../Common/Lzma/LzFind.c
	../Common/Lzma/Lzma2Dec.c
	../Common/Lzma/Lzma2Enc.c
	../Common/Lzma/Lzma86Dec.c
	../Common/Lzma/Lzma86Enc.c
	../Common/Lzma/LzmaDec.c
	../Common/Lzma/LzmaEnc.c
	../Common/Lzma/Alloc.h
	../Common/Lzma/Bra.h
	../Common/Lzma/LzFind.h
	../Common/Lzma/LzHash.h
	../Common/Lzma/Lzma2Dec.h
	../Common/Lzma/Lzma2Enc.h
	../Common/Lzma/Lzma86.h
	../Common/Lzma/LzmaDec.h
	../Common/Lzma/LzmaEnc.h
	../Common/Lzma/Types.h
)


source_group("Source Files" FILES ${SourceGroup_Source_Files})
source_group("Header Files" FILES ${SourceGroup_Header_Files})
source_group("Common" FILES ${SourceGroup_Common})
source_group("Common\\RendElements" FILES ${SourceGroup_Common_RendElements})
source_group("Common\\RendElements\\RendElements_h" FILES ${SourceGroup_Common_RendElements_RendElements_h})
source_group("Common\\RendElements\\Utils" FILES ${SourceGroup_Common_RendElements_Utils})
source_group("Common\\Common_h" FILES ${SourceGroup_Common_Common_h})
source_group("Common\\PostProcess" FILES ${SourceGroup_Common_PostProcess})
source_group("Common\\RenderMesh" FILES ${SourceGroup_Common_RenderMesh})
source_group("Common\\Textures" FILES ${SourceGroup_Common_Textures})
source_group("Common\\Textures\\Image" FILES ${SourceGroup_Common_Textures_Image})
source_group("Common\\Textures\\Image\\Image_h" FILES ${SourceGroup_Common_Textures_Image_Image_h})
source_group("Common\\Shaders" FILES ${SourceGroup_Common_Shaders})
source_group("Common\\Lzma" FILES ${SourceGroup_Common_Lzma})

set (SOURCES
	 ${SourceGroup_root}
	 ${SourceGroup_Source_Files}
	 ${SourceGroup_Header_Files}
	 ${SourceGroup_Common}
	 ${SourceGroup_Common_RendElements}
	 ${SourceGroup_Common_RendElements_RendElements_h}
	 ${SourceGroup_Common_RendElements_Utils}
	 ${SourceGroup_Common_Common_h}
	 ${SourceGroup_Common_PostProcess}
	 ${SourceGroup_Common_RenderMesh}
	 ${SourceGroup_Common_Textures}
	 ${SourceGroup_Common_Textures_Image}
	 ${SourceGroup_Common_Textures_Image_Image_h}
	 ${SourceGroup_Common_Shaders}
	 ${SourceGroup_Common_Lzma}
)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

if(WIN32)
	include_directories( ${WINSDK_SDK_INCLUDE_DIR} )
endif(WIN32)

add_definitions(-D_RENDERER)
add_definitions(-DNULL_RENDERER)

add_library( ${THIS_PROJECT} ${SOURCES})

if(WIN32)
	target_link_libraries( ${THIS_PROJECT} opengl32 glu32 )
endif(WIN32)

USE_MSVC_PRECOMPILED_HEADER( ${THIS_PROJECT} "StdAfx.h" "../StdAfx.cpp" )
EXCLUDE_FILE_FROM_MSVC_PRECOMPILED_HEADER( "../Common/lzss.cpp" )

SET_PLATFORM_TARGET_PROPERTIES( ${THIS_PROJECT} )
