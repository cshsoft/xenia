/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_DXBC_SHADER_TRANSLATOR_H_
#define XENIA_GPU_DXBC_SHADER_TRANSLATOR_H_

#include <cstring>
#include <string>
#include <vector>

#include "xenia/base/math.h"
#include "xenia/gpu/shader_translator.h"

namespace xe {
namespace gpu {

// Generates shader model 5_1 byte code (for Direct3D 12).
class DxbcShaderTranslator : public ShaderTranslator {
 public:
  DxbcShaderTranslator(bool edram_rov_used);
  ~DxbcShaderTranslator() override;

  // Constant buffer bindings in space 0.
  enum class CbufferRegister {
    kSystemConstants,
    kFloatConstants,
    kBoolLoopConstants,
    kFetchConstants,
  };

  enum : uint32_t {
    kSysFlag_XYDividedByW = 1,
    kSysFlag_ZDividedByW = kSysFlag_XYDividedByW << 1,
    kSysFlag_WNotReciprocal = kSysFlag_ZDividedByW << 1,
    kSysFlag_ReverseZ = kSysFlag_WNotReciprocal << 1,
    kSysFlag_Color0Gamma = kSysFlag_ReverseZ << 1,
    kSysFlag_Color1Gamma = kSysFlag_Color0Gamma << 1,
    kSysFlag_Color2Gamma = kSysFlag_Color1Gamma << 1,
    kSysFlag_Color3Gamma = kSysFlag_Color2Gamma << 1,
  };

  enum : uint32_t {
    // Whether the write mask is non-zero.
    kRTFlag_Used = 1,
    // Whether the render target needs to be merged with another (if the write
    // mask is not 1111, or 11 for 16_16, or 1 for 32_FLOAT, or blending is
    // enabled and it's not no-op).
    kRTFlag_Load = kRTFlag_Used << 1,
    kRTFlag_Blend = kRTFlag_Load << 1,
    // Whether the format is represented by 2 dwords.
    kRTFlag_Format64bpp = kRTFlag_Blend << 1,
    // Whether the format is fixed-point and needs to be converted to integer
    // (k_8_8_8_8, k_2_10_10_10, k_16_16, k_16_16_16_16).
    kRTFlag_FormatFixed = kRTFlag_Format64bpp << 1,
    // Whether the format is k_2_10_10_10_FLOAT and 7e3 conversion is needed.
    kRTFlag_FormatFloat10 = kRTFlag_FormatFixed << 1,
    // Whether the format is k_16_16_FLOAT or k_16_16_16_16_FLOAT and
    // f16tof32/f32tof16 is needed.
    kRTFlag_FormatFloat16 = kRTFlag_FormatFloat10 << 1,
  };

  enum : uint32_t {
    // First blend constant of the render target.

    kBlend1_Src_SrcColor_Shift = 0,
    kBlend1_Src_SrcColor_Pos = 1u << kBlend1_Src_SrcColor_Shift,
    kBlend1_Src_SrcColor_Neg = 3u << kBlend1_Src_SrcColor_Shift,
    kBlend1_Src_SrcAlpha_Shift = 2,
    kBlend1_Src_SrcAlpha_Pos = 1u << kBlend1_Src_SrcAlpha_Shift,
    kBlend1_Src_SrcAlpha_Neg = 3u << kBlend1_Src_SrcAlpha_Shift,
    kBlend1_Src_DestColor_Shift = 4,
    kBlend1_Src_DestColor_Pos = 1u << kBlend1_Src_DestColor_Shift,
    kBlend1_Src_DestColor_Neg = 3u << kBlend1_Src_DestColor_Shift,
    kBlend1_Src_DestAlpha_Shift = 6,
    kBlend1_Src_DestAlpha_Pos = 1u << kBlend1_Src_DestAlpha_Shift,
    kBlend1_Src_DestAlpha_Neg = 3u << kBlend1_Src_DestAlpha_Shift,
    // For ONE_MINUS modes, enable both One and the needed factor with _Neg.
    kBlend1_Src_One_Shift = 8,
    kBlend1_Src_One = 1u << kBlend1_Src_One_Shift,

    kBlend1_SrcAlpha_SrcAlpha_Shift = 9,
    kBlend1_SrcAlpha_SrcAlpha_Pos = 1u << kBlend1_SrcAlpha_SrcAlpha_Shift,
    kBlend1_SrcAlpha_SrcAlpha_Neg = 3u << kBlend1_SrcAlpha_SrcAlpha_Shift,
    kBlend1_SrcAlpha_DestAlpha_Shift = 11,
    kBlend1_SrcAlpha_DestAlpha_Pos = 1u << kBlend1_SrcAlpha_DestAlpha_Shift,
    kBlend1_SrcAlpha_DestAlpha_Neg = 3u << kBlend1_SrcAlpha_DestAlpha_Shift,
    kBlend1_SrcAlpha_One_Shift = 13,
    kBlend1_SrcAlpha_One = 1u << kBlend1_SrcAlpha_One_Shift,

    kBlend1_Dest_SrcColor_Shift = 14,
    kBlend1_Dest_SrcColor_Pos = 1u << kBlend1_Dest_SrcColor_Shift,
    kBlend1_Dest_SrcColor_Neg = 3u << kBlend1_Dest_SrcColor_Shift,
    kBlend1_Dest_SrcAlpha_Shift = 16,
    kBlend1_Dest_SrcAlpha_Pos = 1u << kBlend1_Dest_SrcAlpha_Shift,
    kBlend1_Dest_SrcAlpha_Neg = 3u << kBlend1_Dest_SrcAlpha_Shift,
    kBlend1_Dest_DestColor_Shift = 18,
    kBlend1_Dest_DestColor_Pos = 1u << kBlend1_Dest_DestColor_Shift,
    kBlend1_Dest_DestColor_Neg = 3u << kBlend1_Dest_DestColor_Shift,
    kBlend1_Dest_DestAlpha_Shift = 20,
    kBlend1_Dest_DestAlpha_Pos = 1u << kBlend1_Dest_DestAlpha_Shift,
    kBlend1_Dest_DestAlpha_Neg = 3u << kBlend1_Dest_DestAlpha_Shift,
    // For ONE_MINUS modes, enable both One and the needed factor with _Neg.
    kBlend1_Dest_One_Shift = 22,
    kBlend1_Dest_One = 1u << kBlend1_Dest_One_Shift,

    kBlend1_DestAlpha_SrcAlpha_Shift = 23,
    kBlend1_DestAlpha_SrcAlpha_Pos = 1u << kBlend1_DestAlpha_SrcAlpha_Shift,
    kBlend1_DestAlpha_SrcAlpha_Neg = 3u << kBlend1_DestAlpha_SrcAlpha_Shift,
    kBlend1_DestAlpha_DestAlpha_Shift = 25,
    kBlend1_DestAlpha_DestAlpha_Pos = 1u << kBlend1_DestAlpha_DestAlpha_Shift,
    kBlend1_DestAlpha_DestAlpha_Neg = 3u << kBlend1_DestAlpha_DestAlpha_Shift,
    kBlend1_DestAlpha_One_Shift = 27,
    kBlend1_DestAlpha_One = 1u << kBlend1_DestAlpha_One_Shift,

    // Second blend constant of the render target.

    kBlend2_Src_ConstantColor_Shift = 0,
    kBlend2_Src_ConstantColor_Pos = 1u << kBlend2_Src_ConstantColor_Shift,
    kBlend2_Src_ConstantColor_Neg = 3u << kBlend2_Src_ConstantColor_Shift,
    kBlend2_Src_ConstantAlpha_Shift = 2,
    kBlend2_Src_ConstantAlpha_Pos = 1u << kBlend2_Src_ConstantAlpha_Shift,
    kBlend2_Src_ConstantAlpha_Neg = 3u << kBlend2_Src_ConstantAlpha_Shift,

    kBlend2_SrcAlpha_ConstantAlpha_Shift = 4,
    kBlend2_SrcAlpha_ConstantAlpha_Pos =
        1u << kBlend2_SrcAlpha_ConstantAlpha_Shift,
    kBlend2_SrcAlpha_ConstantAlpha_Neg =
        3u << kBlend2_SrcAlpha_ConstantAlpha_Shift,

    kBlend2_Dest_ConstantColor_Shift = 6,
    kBlend2_Dest_ConstantColor_Pos = 1u << kBlend2_Dest_ConstantColor_Shift,
    kBlend2_Dest_ConstantColor_Neg = 3u << kBlend2_Dest_ConstantColor_Shift,
    kBlend2_Dest_ConstantAlpha_Shift = 8,
    kBlend2_Dest_ConstantAlpha_Pos = 1u << kBlend2_Dest_ConstantAlpha_Shift,
    kBlend2_Dest_ConstantAlpha_Neg = 3u << kBlend2_Dest_ConstantAlpha_Shift,

    kBlend2_DestAlpha_ConstantAlpha_Shift = 10,
    kBlend2_DestAlpha_ConstantAlpha_Pos =
        1u << kBlend2_DestAlpha_ConstantAlpha_Shift,
    kBlend2_DestAlpha_ConstantAlpha_Neg =
        3u << kBlend2_DestAlpha_ConstantAlpha_Shift,

    kBlend2_Src_AlphaSaturate_Shift = 12,
    kBlend2_Src_AlphaSaturate = 1u << kBlend2_Src_AlphaSaturate_Shift,
    kBlend2_SrcAlpha_AlphaSaturate_Shift = 13,
    kBlend2_SrcAlpha_AlphaSaturate = 1u << kBlend2_SrcAlpha_AlphaSaturate_Shift,
    kBlend2_Dest_AlphaSaturate_Shift = 14,
    kBlend2_Dest_AlphaSaturate = 1u << kBlend2_Dest_AlphaSaturate_Shift,
    kBlend2_DestAlpha_AlphaSaturate_Shift = 15,
    kBlend2_DestAlpha_AlphaSaturate = 1u
                                      << kBlend2_DestAlpha_AlphaSaturate_Shift,

    // For addition/subtraction/inverse subtraction.
    kBlend2_Src_OpSign_Shift = 16,
    kBlend2_Src_OpSign_Pos = 1u << kBlend2_Src_OpSign_Shift,
    kBlend2_Src_OpSign_Neg = 3u << kBlend2_Src_OpSign_Shift,
    kBlend2_SrcAlpha_OpSign_Shift = 18,
    kBlend2_SrcAlpha_OpSign_Pos = 1u << kBlend2_SrcAlpha_OpSign_Shift,
    kBlend2_SrcAlpha_OpSign_Neg = 3u << kBlend2_SrcAlpha_OpSign_Shift,
    kBlend2_Dest_OpSign_Shift = 20,
    kBlend2_Dest_OpSign_Pos = 1u << kBlend2_Dest_OpSign_Shift,
    kBlend2_Dest_OpSign_Neg = 3u << kBlend2_Dest_OpSign_Shift,
    kBlend2_DestAlpha_OpSign_Shift = 22,
    kBlend2_DestAlpha_OpSign_Pos = 1u << kBlend2_DestAlpha_OpSign_Shift,
    kBlend2_DestAlpha_OpSign_Neg = 3u << kBlend2_DestAlpha_OpSign_Shift,

    kBlend2_Color_OpMin_Shift = 24,
    kBlend2_Color_OpMin = 1u << kBlend2_Color_OpMin_Shift,
    kBlend2_Color_OpMax_Shift = 25,
    kBlend2_Color_OpMax = 1u << kBlend2_Color_OpMax_Shift,
  };

  // IF SYSTEM CONSTANTS ARE CHANGED OR ADDED, THE FOLLOWING MUST BE UPDATED:
  // - kSysConst enum (indices, registers and first components).
  // - system_constant_rdef_.
  // - d3d12/shaders/xenos_draw.hlsli (for geometry shaders).
  struct SystemConstants {
    // vec4 0
    uint32_t flags;
    uint32_t vertex_index_endian;
    uint32_t vertex_base_index;
    uint32_t pixel_pos_reg;

    // vec4 1
    float ndc_scale[3];
    float pixel_half_pixel_offset;

    // vec4 2
    float ndc_offset[3];
    // 0 - disabled, 1 - passes if in range, -1 - fails if in range.
    int32_t alpha_test;

    // vec4 3
    float point_size[2];
    float point_size_min_max[2];

    // vec3 4
    // Inverse scale of the host viewport (but not supersampled), with signs
    // pre-applied.
    float point_screen_to_ndc[2];
    float ssaa_inv_scale[2];

    // vec4 5
    // The range is floats as uints so it's easier to pass infinity.
    uint32_t alpha_test_range[2];
    uint32_t edram_pitch_tiles;
    uint32_t padding_5;

    // vec4 6
    float color_exp_bias[4];

    // vec4 7
    uint32_t color_output_map[4];

    // vec4 8
    uint32_t edram_base_dwords[4];

    // vec4 9
    // Binding and format info flags.
    uint32_t edram_rt_flags[4];

    // vec4 10:13
    // Format info - widths of components in the lower 32 bits (for ibfe/bfi).
    uint32_t edram_rt_pack_width_low[4][4];

    // vec4 14:17
    // Format info - offsets of components in the lower 32 bits (for ibfe/bfi),
    // each in 8 bits.
    uint32_t edram_rt_pack_offset_low[4][4];

    // vec4 18:19
    // Format info - mask of color and alpha after unpacking, but before float
    // conversion. Primarily to differentiate between signed and unsigned
    // formats because ibfe is used for both since k_16_16 and k_16_16_16_16 are
    // signed.
    uint32_t edram_load_mask_rt01_rt23[2][4];

    // vec4 20
    // First part of the blending options.
    uint32_t edram_blend1[4];

    // vec4 21
    // Second part of the blending options.
    uint32_t edram_blend2[4];

    // vec4 22
    // The constant blend factor for the respective modes.
    float edram_blend_constant[4];

    // vec4 23:24
    // Format info - minimum color and alpha values (as float, before
    // conversion) writable to the each render target. Integer so it's easier to
    // write infinity.
    uint32_t edram_store_min_rt01_rt23[2][4];

    // vec4 25:26
    // Format info - maximum color and alpha values (as float, before
    // conversion) writable to the each render target. Integer so it's easier to
    // write infinity.
    uint32_t edram_store_max_rt01_rt23[2][4];

    // vec4 27:28
    // Format info - scale to apply to the color and the alpha of each render
    // target before packing.
    float edram_store_scale_rt01_rt23[2][4];
  };

  // 192 textures at most because there are 32 fetch constants, and textures can
  // be 2D array, 3D or cube, and also signed and unsigned.
  static constexpr uint32_t kMaxTextureSRVIndexBits = 8;
  static constexpr uint32_t kMaxTextureSRVs =
      (1 << kMaxTextureSRVIndexBits) - 1;
  struct TextureSRV {
    uint32_t fetch_constant;
    TextureDimension dimension;
    bool is_signed;
    // Whether this SRV must be bound even if it's signed and all components are
    // unsigned and vice versa (for kGetTextureComputedLod).
    bool is_sign_required;
    std::string name;
  };
  // The first binding returned is at t1 because t0 is shared memory.
  const TextureSRV* GetTextureSRVs(uint32_t& count_out) const {
    count_out = uint32_t(texture_srvs_.size());
    return texture_srvs_.data();
  }

  // Arbitrary limit - there can't be more than 2048 in a shader-visible
  // descriptor heap, though some older hardware (tier 1 resource binding -
  // Nvidia Fermi) doesn't support more than 16 samplers bound at once (we can't
  // really do anything if a game uses more than 16), but just to have some
  // limit so sampler count can easily be packed into 32-bit map keys (for
  // instance, for root signatures). But shaders can specify overrides for
  // filtering modes, and the number of possible combinations is huge - let's
  // limit it to something sane.
  static constexpr uint32_t kMaxSamplerBindingIndexBits = 7;
  static constexpr uint32_t kMaxSamplerBindings =
      (1 << kMaxSamplerBindingIndexBits) - 1;
  struct SamplerBinding {
    uint32_t fetch_constant;
    TextureFilter mag_filter;
    TextureFilter min_filter;
    TextureFilter mip_filter;
    AnisoFilter aniso_filter;
    std::string name;
  };
  const SamplerBinding* GetSamplerBindings(uint32_t& count_out) const {
    count_out = uint32_t(sampler_bindings_.size());
    return sampler_bindings_.data();
  }

 protected:
  void Reset() override;

  void StartTranslation() override;

  std::vector<uint8_t> CompleteTranslation() override;

  void ProcessLabel(uint32_t cf_index) override;

  void ProcessExecInstructionBegin(const ParsedExecInstruction& instr) override;
  void ProcessExecInstructionEnd(const ParsedExecInstruction& instr) override;
  void ProcessLoopStartInstruction(
      const ParsedLoopStartInstruction& instr) override;
  void ProcessLoopEndInstruction(
      const ParsedLoopEndInstruction& instr) override;
  void ProcessJumpInstruction(const ParsedJumpInstruction& instr) override;

  void ProcessVertexFetchInstruction(
      const ParsedVertexFetchInstruction& instr) override;
  void ProcessTextureFetchInstruction(
      const ParsedTextureFetchInstruction& instr) override;
  void ProcessAluInstruction(const ParsedAluInstruction& instr) override;

 private:
  enum : uint32_t {
    kSysConst_Flags_Index = 0,
    kSysConst_Flags_Vec = 0,
    kSysConst_Flags_Comp = 0,
    kSysConst_VertexIndexEndian_Index = kSysConst_Flags_Index + 1,
    kSysConst_VertexIndexEndian_Vec = kSysConst_Flags_Vec,
    kSysConst_VertexIndexEndian_Comp = 1,
    kSysConst_VertexBaseIndex_Index = kSysConst_VertexIndexEndian_Index + 1,
    kSysConst_VertexBaseIndex_Vec = kSysConst_Flags_Vec,
    kSysConst_VertexBaseIndex_Comp = 2,
    kSysConst_PixelPosReg_Index = kSysConst_VertexBaseIndex_Index + 1,
    kSysConst_PixelPosReg_Vec = kSysConst_Flags_Vec,
    kSysConst_PixelPosReg_Comp = 3,

    kSysConst_NDCScale_Index = kSysConst_PixelPosReg_Index + 1,
    kSysConst_NDCScale_Vec = kSysConst_Flags_Vec + 1,
    kSysConst_NDCScale_Comp = 0,
    kSysConst_PixelHalfPixelOffset_Index = kSysConst_NDCScale_Index + 1,
    kSysConst_PixelHalfPixelOffset_Vec = kSysConst_NDCScale_Vec,
    kSysConst_PixelHalfPixelOffset_Comp = 3,

    kSysConst_NDCOffset_Index = kSysConst_PixelHalfPixelOffset_Index + 1,
    kSysConst_NDCOffset_Vec = kSysConst_NDCScale_Vec + 1,
    kSysConst_NDCOffset_Comp = 0,
    kSysConst_AlphaTest_Index = kSysConst_NDCOffset_Index + 1,
    kSysConst_AlphaTest_Vec = kSysConst_NDCOffset_Vec,
    kSysConst_AlphaTest_Comp = 3,

    kSysConst_PointSize_Index = kSysConst_AlphaTest_Index + 1,
    kSysConst_PointSize_Vec = kSysConst_NDCOffset_Vec + 1,
    kSysConst_PointSize_Comp = 0,
    kSysConst_PointSizeMinMax_Index = kSysConst_PointSize_Index + 1,
    kSysConst_PointSizeMinMax_Vec = kSysConst_PointSize_Vec,
    kSysConst_PointSizeMinMax_Comp = 2,

    kSysConst_PointScreenToNDC_Index = kSysConst_PointSizeMinMax_Index + 1,
    kSysConst_PointScreenToNDC_Vec = kSysConst_PointSize_Vec + 1,
    kSysConst_PointScreenToNDC_Comp = 0,
    kSysConst_SSAAInvScale_Index = kSysConst_PointScreenToNDC_Index + 1,
    kSysConst_SSAAInvScale_Vec = kSysConst_PointScreenToNDC_Vec,
    kSysConst_SSAAInvScale_Comp = 2,

    kSysConst_AlphaTestRange_Index = kSysConst_SSAAInvScale_Index + 1,
    kSysConst_AlphaTestRange_Vec = kSysConst_PointScreenToNDC_Vec + 1,
    kSysConst_AlphaTestRange_Comp = 0,
    kSysConst_EDRAMPitchTiles_Index = kSysConst_AlphaTestRange_Index + 1,
    kSysConst_EDRAMPitchTiles_Vec = kSysConst_AlphaTestRange_Vec,
    kSysConst_EDRAMPitchTiles_Comp = 2,

    kSysConst_ColorExpBias_Index = kSysConst_EDRAMPitchTiles_Index + 1,
    kSysConst_ColorExpBias_Vec = kSysConst_AlphaTestRange_Vec + 1,

    kSysConst_ColorOutputMap_Index = kSysConst_ColorExpBias_Index + 1,
    kSysConst_ColorOutputMap_Vec = kSysConst_ColorExpBias_Vec + 1,

    kSysConst_EDRAMBaseDwords_Index = kSysConst_ColorOutputMap_Index + 1,
    kSysConst_EDRAMBaseDwords_Vec = kSysConst_ColorOutputMap_Vec + 1,

    kSysConst_EDRAMRTFlags_Index = kSysConst_EDRAMBaseDwords_Index + 1,
    kSysConst_EDRAMRTFlags_Vec = kSysConst_EDRAMBaseDwords_Vec + 1,

    kSysConst_EDRAMRTPackWidthLowRT0_Index = kSysConst_EDRAMRTFlags_Index + 1,
    kSysConst_EDRAMRTPackWidthLowRT0_Vec = kSysConst_EDRAMRTFlags_Vec + 1,

    kSysConst_EDRAMRTPackWidthLowRT1_Index =
        kSysConst_EDRAMRTPackWidthLowRT0_Index + 1,
    kSysConst_EDRAMRTPackWidthLowRT1_Vec =
        kSysConst_EDRAMRTPackWidthLowRT0_Vec + 1,

    kSysConst_EDRAMRTPackWidthLowRT2_Index =
        kSysConst_EDRAMRTPackWidthLowRT1_Index + 1,
    kSysConst_EDRAMRTPackWidthLowRT2_Vec =
        kSysConst_EDRAMRTPackWidthLowRT1_Vec + 1,

    kSysConst_EDRAMRTPackWidthLowRT3_Index =
        kSysConst_EDRAMRTPackWidthLowRT2_Index + 1,
    kSysConst_EDRAMRTPackWidthLowRT3_Vec =
        kSysConst_EDRAMRTPackWidthLowRT2_Vec + 1,

    kSysConst_EDRAMRTPackOffsetLowRT0_Index =
        kSysConst_EDRAMRTPackWidthLowRT3_Index + 1,
    kSysConst_EDRAMRTPackOffsetLowRT0_Vec =
        kSysConst_EDRAMRTPackWidthLowRT3_Vec + 1,

    kSysConst_EDRAMRTPackOffsetLowRT1_Index =
        kSysConst_EDRAMRTPackOffsetLowRT0_Index + 1,
    kSysConst_EDRAMRTPackOffsetLowRT1_Vec =
        kSysConst_EDRAMRTPackOffsetLowRT0_Vec + 1,

    kSysConst_EDRAMRTPackOffsetLowRT2_Index =
        kSysConst_EDRAMRTPackOffsetLowRT1_Index + 1,
    kSysConst_EDRAMRTPackOffsetLowRT2_Vec =
        kSysConst_EDRAMRTPackOffsetLowRT1_Vec + 1,

    kSysConst_EDRAMRTPackOffsetLowRT3_Index =
        kSysConst_EDRAMRTPackOffsetLowRT2_Index + 1,
    kSysConst_EDRAMRTPackOffsetLowRT3_Vec =
        kSysConst_EDRAMRTPackOffsetLowRT2_Vec + 1,

    kSysConst_EDRAMLoadMaskRT01_Index =
        kSysConst_EDRAMRTPackOffsetLowRT3_Index + 1,
    kSysConst_EDRAMLoadMaskRT01_Vec = kSysConst_EDRAMRTPackOffsetLowRT3_Vec + 1,

    kSysConst_EDRAMLoadMaskRT23_Index = kSysConst_EDRAMLoadMaskRT01_Index + 1,
    kSysConst_EDRAMLoadMaskRT23_Vec = kSysConst_EDRAMLoadMaskRT01_Vec + 1,

    kSysConst_EDRAMBlend1_Index = kSysConst_EDRAMLoadMaskRT23_Index + 1,
    kSysConst_EDRAMBlend1_Vec = kSysConst_EDRAMLoadMaskRT23_Vec + 1,

    kSysConst_EDRAMBlend2_Index = kSysConst_EDRAMBlend1_Index + 1,
    kSysConst_EDRAMBlend2_Vec = kSysConst_EDRAMBlend1_Vec + 1,

    kSysConst_EDRAMBlendConstant_Index = kSysConst_EDRAMBlend2_Index + 1,
    kSysConst_EDRAMBlendConstant_Vec = kSysConst_EDRAMBlend2_Vec + 1,

    kSysConst_EDRAMStoreMinRT01_Index = kSysConst_EDRAMBlendConstant_Index + 1,
    kSysConst_EDRAMStoreMinRT01_Vec = kSysConst_EDRAMBlendConstant_Vec + 1,

    kSysConst_EDRAMStoreMinRT23_Index = kSysConst_EDRAMStoreMinRT01_Index + 1,
    kSysConst_EDRAMStoreMinRT23_Vec = kSysConst_EDRAMStoreMinRT01_Vec + 1,

    kSysConst_EDRAMStoreMaxRT01_Index = kSysConst_EDRAMStoreMinRT23_Index + 1,
    kSysConst_EDRAMStoreMaxRT01_Vec = kSysConst_EDRAMStoreMinRT23_Vec + 1,

    kSysConst_EDRAMStoreMaxRT23_Index = kSysConst_EDRAMStoreMaxRT01_Index + 1,
    kSysConst_EDRAMStoreMaxRT23_Vec = kSysConst_EDRAMStoreMaxRT01_Vec + 1,

    kSysConst_EDRAMStoreScaleRT01_Index = kSysConst_EDRAMStoreMaxRT23_Index + 1,
    kSysConst_EDRAMStoreScaleRT01_Vec = kSysConst_EDRAMStoreMaxRT23_Vec + 1,

    kSysConst_EDRAMStoreScaleRT23_Index =
        kSysConst_EDRAMStoreScaleRT01_Index + 1,
    kSysConst_EDRAMStoreScaleRT23_Vec = kSysConst_EDRAMStoreScaleRT01_Vec + 1,

    kSysConst_Count = kSysConst_EDRAMStoreScaleRT23_Index + 1
  };

  static constexpr uint32_t kInterpolatorCount = 16;
  static constexpr uint32_t kPointParametersTexCoord = kInterpolatorCount;

  // IF ANY OF THESE ARE CHANGED, WriteInputSignature and WriteOutputSignature
  // MUST BE UPDATED!

  static constexpr uint32_t kVSInVertexIndexRegister = 0;
  static constexpr uint32_t kVSOutInterpolatorRegister = 0;
  static constexpr uint32_t kVSOutPointParametersRegister =
      kVSOutInterpolatorRegister + kInterpolatorCount;
  static constexpr uint32_t kVSOutPositionRegister =
      kVSOutPointParametersRegister + 1;

  static constexpr uint32_t kPSInInterpolatorRegister = 0;
  static constexpr uint32_t kPSInPointParametersRegister =
      kPSInInterpolatorRegister + kInterpolatorCount;
  static constexpr uint32_t kPSInPositionRegister =
      kPSInPointParametersRegister + 1;
  static constexpr uint32_t kPSInFrontFaceRegister = kPSInPositionRegister + 1;

  static constexpr uint32_t kSwizzleXYZW = 0b11100100;
  static constexpr uint32_t kSwizzleXXXX = 0b00000000;
  static constexpr uint32_t kSwizzleYYYY = 0b01010101;
  static constexpr uint32_t kSwizzleZZZZ = 0b10101010;
  static constexpr uint32_t kSwizzleWWWW = 0b11111111;

  // Operand encoding, with 32-bit immediate indices by default. None of the
  // arguments must be shifted when calling.
  static constexpr uint32_t EncodeScalarOperand(
      uint32_t type, uint32_t index_dimension,
      uint32_t index_representation_0 = 0, uint32_t index_representation_1 = 0,
      uint32_t index_representation_2 = 0) {
    // D3D10_SB_OPERAND_1_COMPONENT.
    return 1 | (type << 12) | (index_dimension << 20) |
           (index_representation_0 << 22) | (index_representation_1 << 25) |
           (index_representation_0 << 28);
  }
  // For writing to vectors. Mask literal can be written as 0bWZYX.
  static constexpr uint32_t EncodeVectorMaskedOperand(
      uint32_t type, uint32_t mask, uint32_t index_dimension,
      uint32_t index_representation_0 = 0, uint32_t index_representation_1 = 0,
      uint32_t index_representation_2 = 0) {
    // D3D10_SB_OPERAND_4_COMPONENT, D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE.
    return 2 | (0 << 2) | (mask << 4) | (type << 12) | (index_dimension << 20) |
           (index_representation_0 << 22) | (index_representation_1 << 25) |
           (index_representation_2 << 28);
  }
  // For reading from vectors. Swizzle can be written as 0bWWZZYYXX.
  static constexpr uint32_t EncodeVectorSwizzledOperand(
      uint32_t type, uint32_t swizzle, uint32_t index_dimension,
      uint32_t index_representation_0 = 0, uint32_t index_representation_1 = 0,
      uint32_t index_representation_2 = 0) {
    // D3D10_SB_OPERAND_4_COMPONENT, D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE.
    return 2 | (1 << 2) | (swizzle << 4) | (type << 12) |
           (index_dimension << 20) | (index_representation_0 << 22) |
           (index_representation_1 << 25) | (index_representation_2 << 28);
  }
  // For reading a single component of a vector as a 4-component vector.
  static constexpr uint32_t EncodeVectorReplicatedOperand(
      uint32_t type, uint32_t component, uint32_t index_dimension,
      uint32_t index_representation_0 = 0, uint32_t index_representation_1 = 0,
      uint32_t index_representation_2 = 0) {
    // D3D10_SB_OPERAND_4_COMPONENT, D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE.
    return 2 | (1 << 2) | (component << 4) | (component << 6) |
           (component << 8) | (component << 10) | (type << 12) |
           (index_dimension << 20) | (index_representation_0 << 22) |
           (index_representation_1 << 25) | (index_representation_2 << 28);
  }
  // For reading scalars from vectors.
  static constexpr uint32_t EncodeVectorSelectOperand(
      uint32_t type, uint32_t component, uint32_t index_dimension,
      uint32_t index_representation_0 = 0, uint32_t index_representation_1 = 0,
      uint32_t index_representation_2 = 0) {
    // D3D10_SB_OPERAND_4_COMPONENT, D3D10_SB_OPERAND_4_COMPONENT_SELECT_1_MODE.
    return 2 | (2 << 2) | (component << 4) | (type << 12) |
           (index_dimension << 20) | (index_representation_0 << 22) |
           (index_representation_1 << 25) | (index_representation_2 << 28);
  }

  // Allocates a new r# register for internal use and returns its index.
  uint32_t PushSystemTemp(bool zero = false);
  // Frees the last allocated internal r# registers for later reuse.
  void PopSystemTemp(uint32_t count = 1);

  // Whether general-purpose register values should be stored in x0 rather than
  // r# in this shader.
  bool IndexableGPRsUsed() const;

  // Writing the prologue.
  void StartVertexShader_LoadVertexIndex();
  void StartVertexShader();
  void StartPixelShader();

  // Writing the epilogue.
  void CompleteVertexShader();
  void CompletePixelShader_WriteToRTVs();
  void CompletePixelShader_WriteToROV_LoadColor(
      uint32_t edram_dword_offset_temp, uint32_t rt_index,
      uint32_t target_index);
  void CompletePixelShader_WriteToROV_StoreColor(
      uint32_t edram_dword_offset_temp, uint32_t rt_index,
      uint32_t source_and_scratch_temp);
  void CompletePixelShader_WriteToROV();
  void CompletePixelShader();
  void CompleteShaderCode();

  // Abstract 4-component vector source operand.
  struct DxbcSourceOperand {
    enum class Type {
      // GPR number in the index - used only when GPRs are not dynamically
      // indexed in the shader and there are no constant zeros and ones in the
      // swizzle.
      kRegister,
      // Immediate: float constant vector number in the index.
      // Dynamic: intermediate X contains page number, intermediate Y contains
      // vector number in the page.
      kConstantFloat,
      // The whole value preloaded to the intermediate register - used for GPRs
      // when they are indexable, for bool/loop constants pre-converted to
      // float, and for other operands if their swizzle contains 0 or 1.
      kIntermediateRegister,
      // Literal vector of zeros and positive or negative ones - when the
      // swizzle contains only them, or when the parsed operand is invalid (for
      // example, if it's a fetch constant in a non-tfetch texture instruction).
      // 0 or 1 specified in the index as bits, can be negated.
      kZerosOnes,
    };

    Type type;
    uint32_t index;
    // If the operand is dynamically indexed directly when it's used as an
    // operand in DXBC instructions.
    InstructionStorageAddressingMode addressing_mode;

    uint32_t swizzle;
    bool is_negated;
    bool is_absolute_value;

    // Temporary register containing data required to access the value if it has
    // to be accessed in multiple operations (allocated with PushSystemTemp).
    uint32_t intermediate_register;
    static constexpr uint32_t kIntermediateRegisterNone = UINT32_MAX;
  };
  // Each Load must be followed by Unload, otherwise there may be a temporary
  // register leak.
  void LoadDxbcSourceOperand(const InstructionOperand& operand,
                             DxbcSourceOperand& dxbc_operand);
  // Number of tokens this operand adds to the instruction length when used.
  uint32_t DxbcSourceOperandLength(const DxbcSourceOperand& operand,
                                   bool negate = false,
                                   bool absolute = false) const;
  // Writes the operand access tokens to the instruction (either for a scalar if
  // select_component is <= 3, or for a vector).
  void UseDxbcSourceOperand(const DxbcSourceOperand& operand,
                            uint32_t additional_swizzle = kSwizzleXYZW,
                            uint32_t select_component = 4, bool negate = false,
                            bool absolute = false);
  void UnloadDxbcSourceOperand(const DxbcSourceOperand& operand);

  // Writes xyzw or xxxx of the specified r# to the destination.
  void StoreResult(const InstructionResult& result, uint32_t reg,
                   bool replicate_x);

  // The nesting of `if` instructions is the following:
  // - pc checks (labels).
  // - Bool constant checks (can only be done by exec).
  // - Predicate checks (can be done both by exec and by instructions).
  // It's probably fine to place instruction predicate checks and exec predicate
  // on the same level rather than creating another level for instruction-level
  // predicates, because (at least in Halo 3), in a `(p0) exec`, all
  // instructions are `(p0)`, and `setp` isn't invoked in `(p0) exec`. Another
  // possible constraint making things easier is labels not appearing within
  // execs - so a label doesn't have to recheck the exec's condition.
  // TODO(Triang3l): Check if these control flow constrains are true for all
  // games.

  // Closes the current predicate `if` (but doesn't reset the current exec's
  // predicate).
  void ClosePredicate();
  // Updates the current predicate, placing if/endif when needed. This MUST be
  // called before emitting any instructions within an exec because the exec
  // implementation here doesn't place if/endif, only defers updating the
  // predicate.
  void CheckPredicate(bool instruction_predicated,
                      bool instruction_predicate_condition);
  // Opens or closes the `if` checking the value of a bool constant - call with
  // kCfExecBoolConstantNone to force close.
  void SetExecBoolConstant(uint32_t index, bool condition);
  void JumpToLabel(uint32_t address);

  // Emits copde for endian swapping of the data located in pv.
  void SwapVertexData(uint32_t vfetch_index, uint32_t write_mask);

  // Returns T#/t# index (they are the same in this translator).
  uint32_t FindOrAddTextureSRV(uint32_t fetch_constant,
                               TextureDimension dimension, bool is_signed,
                               bool is_sign_required = false);
  // Returns S#/s# index (they are the same in this translator).
  uint32_t FindOrAddSamplerBinding(uint32_t fetch_constant,
                                   TextureFilter mag_filter,
                                   TextureFilter min_filter,
                                   TextureFilter mip_filter,
                                   AnisoFilter aniso_filter);
  // Converts (S, T, face index) in the specified temporary register to a 3D
  // cubemap coordinate.
  void ArrayCoordToCubeDirection(uint32_t reg);

  void ProcessVectorAluInstruction(const ParsedAluInstruction& instr);
  void ProcessScalarAluInstruction(const ParsedAluInstruction& instr);

  // Appends a string to a DWORD stream, returns the DWORD-aligned length.
  static uint32_t AppendString(std::vector<uint32_t>& dest, const char* source);
  // Returns the length of a string as if it was appended to a DWORD stream, in
  // bytes.
  static inline uint32_t GetStringLength(const char* source) {
    return uint32_t(xe::align(std::strlen(source) + 1, sizeof(uint32_t)));
  }

  void WriteResourceDefinitions();
  void WriteInputSignature();
  void WriteOutputSignature();
  void WriteShaderCode();

  // Executable instructions - generated during translation.
  std::vector<uint32_t> shader_code_;
  // Complete shader object, with all the needed chunks and dcl_ instructions -
  // generated in the end of translation.
  std::vector<uint32_t> shader_object_;

  // Whether the output merger should be emulated in pixel shaders.
  bool edram_rov_used_;

  // Data types used in constants buffers. Listed in dependency order.
  enum class RdefTypeIndex {
    kFloat,
    kFloat2,
    kFloat3,
    kFloat4,
    kInt,
    kUint,
    kUint4,
    // Float constants - size written dynamically.
    kFloat4ConstantArray,
    // Bool constants.
    kUint4Array8,
    // Loop constants.
    kUint4Array32,
    // Fetch constants.
    kUint4Array48,

    kCount,
    kUnknown = kCount
  };

  struct RdefStructMember {
    const char* name;
    RdefTypeIndex type;
    uint32_t offset;
  };

  struct RdefType {
    // Name ignored for arrays.
    const char* name;
    // D3D10_SHADER_VARIABLE_CLASS.
    uint32_t type_class;
    // D3D10_SHADER_VARIABLE_TYPE.
    uint32_t type;
    uint32_t row_count;
    uint32_t column_count;
    // 0 for primitive types, 1 for structures, array size for arrays.
    uint32_t element_count;
    uint32_t struct_member_count;
    RdefTypeIndex array_element_type;
    const RdefStructMember* struct_members;
  };
  static const RdefType rdef_types_[size_t(RdefTypeIndex::kCount)];

  // Number of constant buffer bindings used in this shader - also used for
  // generation of indices of constant buffers that are optional.
  uint32_t cbuffer_count_;
  static constexpr uint32_t kCbufferIndexUnallocated = UINT32_MAX;
  uint32_t cbuffer_index_system_constants_;
  uint32_t cbuffer_index_float_constants_;
  uint32_t cbuffer_index_bool_loop_constants_;
  uint32_t cbuffer_index_fetch_constants_;

  struct SystemConstantRdef {
    const char* name;
    RdefTypeIndex type;
    uint32_t offset;
    uint32_t size;
  };
  static const SystemConstantRdef system_constant_rdef_[kSysConst_Count];
  // Mask of system constants (1 << kSysConst_#_Index) used in the shader, so
  // the remaining ones can be marked as unused in RDEF.
  uint64_t system_constants_used_;

  // Whether constants are dynamically indexed and need to be marked as such in
  // dcl_constantBuffer.
  bool float_constants_dynamic_indexed_;
  bool bool_loop_constants_dynamic_indexed_;

  // Offsets of float constant indices in shader_code_, for remapping in
  // CompleteTranslation (initially, at these offsets, guest float constant
  // indices are written).
  std::vector<uint32_t> float_constant_index_offsets_;

  // Number of currently allocated Xenia internal r# registers.
  uint32_t system_temp_count_current_;
  // Total maximum number of temporary registers ever used during this
  // translation (for the declaration).
  uint32_t system_temp_count_max_;

  // Vector ALU result/scratch (since Xenos write masks can contain swizzles).
  uint32_t system_temp_pv_;
  // Temporary register ID for previous scalar result, program counter,
  // predicate and absolute address register.
  uint32_t system_temp_ps_pc_p0_a0_;
  // Loop index stack - .x is the active loop, shifted right to .yzw on push.
  uint32_t system_temp_aL_;
  // Loop counter stack, .x is the active loop. Represents number of times
  // remaining to loop.
  uint32_t system_temp_loop_count_;
  // Explicitly set texture gradients and LOD.
  uint32_t system_temp_grad_h_lod_;
  uint32_t system_temp_grad_v_;

  // Position in vertex shaders (because viewport and W transformations can be
  // applied in the end of the shader).
  uint32_t system_temp_position_;

  // Color outputs in pixel shaders (because of exponent bias, alpha test and
  // remapping).
  uint32_t system_temp_color_[4];

  // Whether a predicate `if` is open.
  bool cf_currently_predicated_;
  // Currently expected predicate value.
  bool cf_current_predicate_condition_;
  // Whether the current `exec` is predicated.
  bool cf_exec_predicated_;
  // Predicate condition in the current `exec`.
  bool cf_exec_predicate_condition_;
  // The bool constant number containing the condition for the current `exec`.
  uint32_t cf_exec_bool_constant_;
  static constexpr uint32_t kCfExecBoolConstantNone = UINT32_MAX;
  // The expected value in the current conditional exec.
  bool cf_exec_bool_constant_condition_;

  bool writes_depth_;

  std::vector<TextureSRV> texture_srvs_;
  std::vector<SamplerBinding> sampler_bindings_;

  // The STAT chunk (based on Wine d3dcompiler_parse_stat).
  struct Statistics {
    uint32_t instruction_count;
    uint32_t temp_register_count;
    // Unknown in Wine.
    uint32_t def_count;
    // Only inputs and outputs.
    uint32_t dcl_count;
    uint32_t float_instruction_count;
    uint32_t int_instruction_count;
    uint32_t uint_instruction_count;
    // endif, ret.
    uint32_t static_flow_control_count;
    // if (but not else).
    uint32_t dynamic_flow_control_count;
    // Unknown in Wine.
    uint32_t macro_instruction_count;
    uint32_t temp_array_count;
    uint32_t array_instruction_count;
    uint32_t cut_instruction_count;
    uint32_t emit_instruction_count;
    uint32_t texture_normal_instructions;
    uint32_t texture_load_instructions;
    uint32_t texture_comp_instructions;
    uint32_t texture_bias_instructions;
    uint32_t texture_gradient_instructions;
    // Not including indexable temp load/store.
    uint32_t mov_instruction_count;
    // Unknown in Wine.
    uint32_t movc_instruction_count;
    uint32_t conversion_instruction_count;
    // Unknown in Wine.
    uint32_t unknown_22;
    uint32_t input_primitive;
    uint32_t gs_output_topology;
    uint32_t gs_max_output_vertex_count;
    uint32_t unknown_26;
    // Unknown in Wine, but confirmed by testing.
    uint32_t lod_instructions;
    uint32_t unknown_28;
    uint32_t unknown_29;
    uint32_t c_control_points;
    uint32_t hs_output_primitive;
    uint32_t hs_partitioning;
    uint32_t tessellator_domain;
    // Unknown in Wine.
    uint32_t c_barrier_instructions;
    // Unknown in Wine.
    uint32_t c_interlocked_instructions;
    // Unknown in Wine, but confirmed by testing.
    uint32_t c_texture_store_instructions;
  };
  Statistics stat_;
};

}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_DXBC_SHADER_TRANSLATOR_H_
