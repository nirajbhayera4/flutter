// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/toolkit/interop/impeller.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include "flutter/fml/mapping.h"
#include "impeller/base/validation.h"
#include "impeller/geometry/scalar.h"
#include "impeller/renderer/backend/gles/context_gles.h"
#include "impeller/renderer/backend/gles/texture_gles.h"
#include "impeller/renderer/context.h"
#include "impeller/toolkit/interop/color_filter.h"
#include "impeller/toolkit/interop/color_source.h"
#include "impeller/toolkit/interop/context.h"
#include "impeller/toolkit/interop/dl_builder.h"
#include "impeller/toolkit/interop/formats.h"
#include "impeller/toolkit/interop/fragment_program.h"
#include "impeller/toolkit/interop/glyph_info.h"
#include "impeller/toolkit/interop/image_filter.h"
#include "impeller/toolkit/interop/line_metrics.h"
#include "impeller/toolkit/interop/mask_filter.h"
#include "impeller/toolkit/interop/object.h"
#include "impeller/toolkit/interop/paint.h"
#include "impeller/toolkit/interop/paragraph.h"
#include "impeller/toolkit/interop/paragraph_builder.h"
#include "impeller/toolkit/interop/paragraph_style.h"
#include "impeller/toolkit/interop/path.h"
#include "impeller/toolkit/interop/path_builder.h"
#include "impeller/toolkit/interop/surface.h"
#include "impeller/toolkit/interop/texture.h"
#include "impeller/toolkit/interop/typography_context.h"

#if IMPELLER_ENABLE_OPENGLES
#include "impeller/toolkit/interop/backend/gles/context_gles.h"
#include "impeller/toolkit/interop/backend/gles/surface_gles.h"
#endif  // IMPELLER_ENABLE_OPENGLES

#if IMPELLER_ENABLE_METAL
#include "impeller/toolkit/interop/backend/metal/context_mtl.h"
#include "impeller/toolkit/interop/backend/metal/surface_mtl.h"
#endif  // IMPELLER_ENABLE_METAL

#if IMPELLER_ENABLE_VULKAN
#include "impeller/toolkit/interop/backend/vulkan/context_vk.h"
#include "impeller/toolkit/interop/backend/vulkan/surface_vk.h"
#include "impeller/toolkit/interop/backend/vulkan/swapchain_vk.h"
#endif  // IMPELLER_ENABLE_VULKAN

namespace impeller::interop {

#define DEFINE_PEER_GETTER(cxx_type, c_type)    \
  cxx_type* GetPeer(c_type object) {            \
    return reinterpret_cast<cxx_type*>(object); \
  }

DEFINE_PEER_GETTER(ColorFilter, ImpellerColorFilter);
DEFINE_PEER_GETTER(ColorSource, ImpellerColorSource);
DEFINE_PEER_GETTER(Context, ImpellerContext);
DEFINE_PEER_GETTER(DisplayList, ImpellerDisplayList);
DEFINE_PEER_GETTER(DisplayListBuilder, ImpellerDisplayListBuilder);
DEFINE_PEER_GETTER(FragmentProgram, ImpellerFragmentProgram);
DEFINE_PEER_GETTER(GlyphInfo, ImpellerGlyphInfo);
DEFINE_PEER_GETTER(ImageFilter, ImpellerImageFilter);
DEFINE_PEER_GETTER(LineMetrics, ImpellerLineMetrics);
DEFINE_PEER_GETTER(MaskFilter, ImpellerMaskFilter);
DEFINE_PEER_GETTER(Paint, ImpellerPaint);
DEFINE_PEER_GETTER(Paragraph, ImpellerParagraph);
DEFINE_PEER_GETTER(ParagraphBuilder, ImpellerParagraphBuilder);
DEFINE_PEER_GETTER(ParagraphStyle, ImpellerParagraphStyle);
DEFINE_PEER_GETTER(Path, ImpellerPath);
DEFINE_PEER_GETTER(PathBuilder, ImpellerPathBuilder);
DEFINE_PEER_GETTER(Surface, ImpellerSurface);
DEFINE_PEER_GETTER(SwapchainVK, ImpellerVulkanSwapchain);
DEFINE_PEER_GETTER(Texture, ImpellerTexture);
DEFINE_PEER_GETTER(TypographyContext, ImpellerTypographyContext);

static std::string GetVersionAsString(uint32_t version) {
  std::stringstream stream;
  stream << IMPELLER_VERSION_GET_VARIANT(version) << "."
         << IMPELLER_VERSION_GET_MAJOR(version) << "."
         << IMPELLER_VERSION_GET_MINOR(version) << "."
         << IMPELLER_VERSION_GET_PATCH(version);
  return stream.str();
}

IMPELLER_EXTERN_C
uint32_t ImpellerGetVersion() {
  return IMPELLER_VERSION;
}

static bool CheckVersion(uint32_t version) {
  if (version != IMPELLER_VERSION) {
    VALIDATION_LOG << "This version of Impeller ("
                   << GetVersionAsString(ImpellerGetVersion()) << ") "
                   << "doesn't match the version the user expects ("
                   << GetVersionAsString(version) << ").";
    return false;
  }
  return true;
}

IMPELLER_EXTERN_C
ImpellerContext ImpellerContextCreateOpenGLESNew(
    uint32_t version,
    ImpellerProcAddressCallback gl_proc_address_callback,
    void* gl_proc_address_callback_user_data) {
  if (!CheckVersion(version)) {
    return nullptr;
  }
#if IMPELLER_ENABLE_OPENGLES
  auto context = ContextGLES::Create(
      [gl_proc_address_callback,
       gl_proc_address_callback_user_data](const char* proc_name) -> void* {
        return gl_proc_address_callback(proc_name,
                                        gl_proc_address_callback_user_data);
      });
  if (!context || !context->IsValid()) {
    VALIDATION_LOG << "Could not create valid context.";
    return nullptr;
  }
  return context.Leak();
#else   // IMPELLER_ENABLE_OPENGLES
  VALIDATION_LOG << "OpenGLES not available.";
  return nullptr;
#endif  // IMPELLER_ENABLE_OPENGLES
}

IMPELLER_EXTERN_C ImpellerContext ImpellerContextCreateMetalNew(
    uint32_t version) {
  if (!CheckVersion(version)) {
    return nullptr;
  }
#if IMPELLER_ENABLE_METAL
  auto context = ContextMTL::Create();
  if (!context || !context->IsValid()) {
    VALIDATION_LOG << "Could not create valid context.";
    return nullptr;
  }
  return context.Leak();
#else   // IMPELLER_ENABLE_METAL
  VALIDATION_LOG << "Metal not available.";
  return nullptr;
#endif  // IMPELLER_ENABLE_METAL
}

IMPELLER_EXTERN_C ImpellerContext ImpellerContextCreateVulkanNew(
    uint32_t version,
    const ImpellerContextVulkanSettings* settings) {
  if (!CheckVersion(version)) {
    return nullptr;
  }
#if IMPELLER_ENABLE_VULKAN
  auto context = ContextVK::Create(ContextVK::Settings(*settings));
  if (!context || !context->IsValid()) {
    VALIDATION_LOG << "Could not create valid context.";
    return nullptr;
  }
  return context.Leak();
#else   // IMPELLER_ENABLE_VULKAN
  VALIDATION_LOG << "Vulkan not available.";
  return nullptr;
#endif  // IMPELLER_ENABLE_VULKAN
}

IMPELLER_EXTERN_C
void ImpellerContextRetain(ImpellerContext context) {
  ObjectBase::SafeRetain(context);
}

IMPELLER_EXTERN_C
void ImpellerContextRelease(ImpellerContext context) {
  ObjectBase::SafeRelease(context);
}

IMPELLER_EXTERN_C
bool ImpellerContextGetVulkanInfo(ImpellerContext IMPELLER_NONNULL context,
                                  ImpellerContextVulkanInfo* out_vulkan_info) {
#if IMPELLER_ENABLE_VULKAN
  if (!GetPeer(context)->IsVulkan()) {
    VALIDATION_LOG << "Not a Vulkan context.";
    return false;
  }
  return reinterpret_cast<ContextVK*>(GetPeer(context))
      ->GetInfo(*out_vulkan_info);
#else   // IMPELLER_ENABLE_VULKAN
  VALIDATION_LOG << "Vulkan not available.";
  return nullptr;
#endif  // IMPELLER_ENABLE_VULKAN
}

IMPELLER_EXTERN_C
ImpellerVulkanSwapchain ImpellerVulkanSwapchainCreateNew(
    ImpellerContext context,
    void* vulkan_surface_khr) {
#if IMPELLER_ENABLE_VULKAN
  return Create<SwapchainVK>(
             *GetPeer(context),                                  //
             reinterpret_cast<VkSurfaceKHR>(vulkan_surface_khr)  //
             )
      .Leak();
#else   // IMPELLER_ENABLE_VULKAN
  VALIDATION_LOG << "Vulkan not available.";
  return nullptr;
#endif  // IMPELLER_ENABLE_VULKAN
}

IMPELLER_EXTERN_C
void ImpellerVulkanSwapchainRetain(ImpellerVulkanSwapchain swapchain) {
  ObjectBase::SafeRetain(swapchain);
}

IMPELLER_EXTERN_C
void ImpellerVulkanSwapchainRelease(ImpellerVulkanSwapchain swapchain) {
  ObjectBase::SafeRelease(swapchain);
}

IMPELLER_EXTERN_C
ImpellerSurface ImpellerVulkanSwapchainAcquireNextSurfaceNew(
    ImpellerVulkanSwapchain swapchain) {
  return GetPeer(swapchain)->AcquireNextSurface().Leak();
}

IMPELLER_EXTERN_C ImpellerDisplayListBuilder ImpellerDisplayListBuilderNew(
    const ImpellerRect* cull_rect) {
  return Create<DisplayListBuilder>(cull_rect).Leak();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderRetain(ImpellerDisplayListBuilder builder) {
  ObjectBase::SafeRetain(builder);
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderRelease(ImpellerDisplayListBuilder builder) {
  ObjectBase::SafeRelease(builder);
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderSave(ImpellerDisplayListBuilder builder) {
  GetPeer(builder)->Save();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderSaveLayer(ImpellerDisplayListBuilder builder,
                                         const ImpellerRect* bounds,
                                         ImpellerPaint paint,
                                         ImpellerImageFilter backdrop) {
  GetPeer(builder)->SaveLayer(ToImpellerType(*bounds),  //
                              GetPeer(paint),           //
                              GetPeer(backdrop)         //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderRestore(ImpellerDisplayListBuilder builder) {
  GetPeer(builder)->Restore();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderScale(ImpellerDisplayListBuilder builder,
                                     float x_scale,
                                     float y_scale) {
  GetPeer(builder)->Scale(Size{x_scale, y_scale});
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderRotate(ImpellerDisplayListBuilder builder,
                                      float angle_degrees) {
  GetPeer(builder)->Rotate(Degrees{angle_degrees});
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderTranslate(ImpellerDisplayListBuilder builder,
                                         float x_translation,
                                         float y_translation) {
  GetPeer(builder)->Translate(Point{x_translation, y_translation});
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderSetTransform(ImpellerDisplayListBuilder builder,
                                            const ImpellerMatrix* transform) {
  GetPeer(builder)->SetTransform(ToImpellerType(*transform));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderTransform(ImpellerDisplayListBuilder builder,
                                         const ImpellerMatrix* transform) {
  GetPeer(builder)->Transform(ToImpellerType(*transform));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderGetTransform(ImpellerDisplayListBuilder builder,
                                            ImpellerMatrix* out_transform) {
  FromImpellerType(GetPeer(builder)->GetTransform(), *out_transform);
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderResetTransform(
    ImpellerDisplayListBuilder builder) {
  GetPeer(builder)->ResetTransform();
}

IMPELLER_EXTERN_C
uint32_t ImpellerDisplayListBuilderGetSaveCount(
    ImpellerDisplayListBuilder builder) {
  return GetPeer(builder)->GetSaveCount();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderRestoreToCount(
    ImpellerDisplayListBuilder builder,
    uint32_t count) {
  GetPeer(builder)->RestoreToCount(count);
}

IMPELLER_EXTERN_C
void ImpellerPathRetain(ImpellerPath path) {
  ObjectBase::SafeRetain(path);
}

IMPELLER_EXTERN_C
void ImpellerPathRelease(ImpellerPath path) {
  ObjectBase::SafeRelease(path);
}

IMPELLER_EXTERN_C
void ImpellerPathGetBounds(ImpellerPath path, ImpellerRect* out_bounds) {
  *out_bounds = GetPeer(path)->GetBounds();
}

IMPELLER_EXTERN_C
ImpellerPathBuilder ImpellerPathBuilderNew() {
  return Create<PathBuilder>().Leak();
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderRetain(ImpellerPathBuilder builder) {
  ObjectBase::SafeRetain(builder);
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderRelease(ImpellerPathBuilder builder) {
  ObjectBase::SafeRelease(builder);
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderMoveTo(ImpellerPathBuilder builder,
                               const ImpellerPoint* location) {
  GetPeer(builder)->MoveTo(ToImpellerType(*location));
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderLineTo(ImpellerPathBuilder builder,
                               const ImpellerPoint* location) {
  GetPeer(builder)->LineTo(ToImpellerType(*location));
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderQuadraticCurveTo(ImpellerPathBuilder builder,
                                         const ImpellerPoint* control_point,
                                         const ImpellerPoint* end_point) {
  GetPeer(builder)->QuadraticCurveTo(ToImpellerType(*control_point),
                                     ToImpellerType(*end_point));
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderCubicCurveTo(ImpellerPathBuilder builder,
                                     const ImpellerPoint* control_point_1,
                                     const ImpellerPoint* control_point_2,
                                     const ImpellerPoint* end_point) {
  GetPeer(builder)->CubicCurveTo(ToImpellerType(*control_point_1),  //
                                 ToImpellerType(*control_point_2),  //
                                 ToImpellerType(*end_point)         //
  );
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderAddRect(ImpellerPathBuilder builder,
                                const ImpellerRect* rect) {
  GetPeer(builder)->AddRect(ToImpellerType(*rect));
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderAddArc(ImpellerPathBuilder builder,
                               const ImpellerRect* oval_bounds,
                               float start_angle_degrees,
                               float end_angle_degrees) {
  GetPeer(builder)->AddArc(ToImpellerType(*oval_bounds),  //
                           Degrees{start_angle_degrees},  //
                           Degrees{end_angle_degrees}     //
  );
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderAddOval(ImpellerPathBuilder builder,
                                const ImpellerRect* oval_bounds) {
  GetPeer(builder)->AddOval(ToImpellerType(*oval_bounds));
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderAddRoundedRect(
    ImpellerPathBuilder builder,
    const ImpellerRect* rect,
    const ImpellerRoundingRadii* rounding_radii) {
  GetPeer(builder)->AddRoundedRect(ToImpellerType(*rect),
                                   ToImpellerType(*rounding_radii));
}

IMPELLER_EXTERN_C
void ImpellerPathBuilderClose(ImpellerPathBuilder builder) {
  GetPeer(builder)->Close();
}

IMPELLER_EXTERN_C
ImpellerPath ImpellerPathBuilderCopyPathNew(ImpellerPathBuilder builder,
                                            ImpellerFillType fill) {
  return GetPeer(builder)->CopyPath(ToImpellerType(fill)).Leak();
}

IMPELLER_EXTERN_C
ImpellerPath ImpellerPathBuilderTakePathNew(ImpellerPathBuilder builder,
                                            ImpellerFillType fill) {
  return GetPeer(builder)->TakePath(ToImpellerType(fill)).Leak();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderClipRect(ImpellerDisplayListBuilder builder,
                                        const ImpellerRect* rect,
                                        ImpellerClipOperation op) {
  GetPeer(builder)->ClipRect(ToImpellerType(*rect), ToImpellerType(op));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderClipOval(ImpellerDisplayListBuilder builder,
                                        const ImpellerRect* oval_bounds,
                                        ImpellerClipOperation op) {
  GetPeer(builder)->ClipOval(ToImpellerType(*oval_bounds), ToImpellerType(op));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderClipRoundedRect(
    ImpellerDisplayListBuilder builder,
    const ImpellerRect* rect,
    const ImpellerRoundingRadii* radii,
    ImpellerClipOperation op) {
  GetPeer(builder)->ClipRoundedRect(ToImpellerType(*rect),   //
                                    ToImpellerType(*radii),  //
                                    ToImpellerType(op)       //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderClipPath(ImpellerDisplayListBuilder builder,
                                        ImpellerPath path,
                                        ImpellerClipOperation op) {
  GetPeer(builder)->ClipPath(*GetPeer(path), ToImpellerType(op));
}

IMPELLER_EXTERN_C
ImpellerPaint ImpellerPaintNew() {
  return Create<Paint>().Leak();
}

IMPELLER_EXTERN_C
void ImpellerPaintRetain(ImpellerPaint paint) {
  ObjectBase::SafeRetain(paint);
}

IMPELLER_EXTERN_C
void ImpellerPaintRelease(ImpellerPaint paint) {
  ObjectBase::SafeRelease(paint);
}

IMPELLER_EXTERN_C
void ImpellerPaintSetColor(ImpellerPaint paint, const ImpellerColor* color) {
  GetPeer(paint)->SetColor(ToDisplayListType(*color));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetBlendMode(ImpellerPaint paint, ImpellerBlendMode mode) {
  GetPeer(paint)->SetBlendMode(ToImpellerType(mode));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetDrawStyle(ImpellerPaint paint, ImpellerDrawStyle style) {
  GetPeer(paint)->SetDrawStyle(ToDisplayListType(style));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetStrokeCap(ImpellerPaint paint, ImpellerStrokeCap cap) {
  GetPeer(paint)->SetStrokeCap(ToDisplayListType(cap));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetStrokeJoin(ImpellerPaint paint, ImpellerStrokeJoin join) {
  GetPeer(paint)->SetStrokeJoin(ToDisplayListType(join));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetStrokeWidth(ImpellerPaint paint, float width) {
  GetPeer(paint)->SetStrokeWidth(width);
}

IMPELLER_EXTERN_C
void ImpellerPaintSetStrokeMiter(ImpellerPaint paint, float miter) {
  GetPeer(paint)->SetStrokeMiter(miter);
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawPaint(ImpellerDisplayListBuilder builder,
                                         ImpellerPaint paint) {
  GetPeer(builder)->DrawPaint(*GetPeer(paint));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawLine(ImpellerDisplayListBuilder builder,
                                        const ImpellerPoint* from,
                                        const ImpellerPoint* to,
                                        ImpellerPaint paint) {
  GetPeer(builder)->DrawLine(ToImpellerType(*from),  //
                             ToImpellerType(*to),    //
                             *GetPeer(paint)         //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawDashedLine(
    ImpellerDisplayListBuilder builder,
    const ImpellerPoint* from,
    const ImpellerPoint* to,
    float on_length,
    float off_length,
    ImpellerPaint paint) {
  GetPeer(builder)->DrawDashedLine(ToImpellerType(*from),  //
                                   ToImpellerType(*to),    //
                                   on_length,              //
                                   off_length,             //
                                   *GetPeer(paint)         //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawRect(ImpellerDisplayListBuilder builder,
                                        const ImpellerRect* rect,
                                        ImpellerPaint paint) {
  GetPeer(builder)->DrawRect(ToImpellerType(*rect), *GetPeer(paint));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawOval(ImpellerDisplayListBuilder builder,
                                        const ImpellerRect* oval_bounds,
                                        ImpellerPaint paint) {
  GetPeer(builder)->DrawOval(ToImpellerType(*oval_bounds), *GetPeer(paint));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawRoundedRect(
    ImpellerDisplayListBuilder builder,
    const ImpellerRect* rect,
    const ImpellerRoundingRadii* radii,
    ImpellerPaint paint) {
  GetPeer(builder)->DrawRoundedRect(ToImpellerType(*rect),   //
                                    ToImpellerType(*radii),  //
                                    *GetPeer(paint)          //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawRoundedRectDifference(
    ImpellerDisplayListBuilder builder,
    const ImpellerRect* outer_rect,
    const ImpellerRoundingRadii* outer_radii,
    const ImpellerRect* inner_rect,
    const ImpellerRoundingRadii* inner_radii,
    ImpellerPaint paint) {
  GetPeer(builder)->DrawRoundedRectDifference(ToImpellerType(*outer_rect),   //
                                              ToImpellerType(*outer_radii),  //
                                              ToImpellerType(*inner_rect),   //
                                              ToImpellerType(*inner_radii),  //
                                              *GetPeer(paint)                //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawPath(ImpellerDisplayListBuilder builder,
                                        ImpellerPath path,
                                        ImpellerPaint paint) {
  GetPeer(builder)->DrawPath(*GetPeer(path), *GetPeer(paint));
}

IMPELLER_EXTERN_C
ImpellerTexture ImpellerTextureCreateWithContentsNew(
    ImpellerContext context,
    const ImpellerTextureDescriptor* descriptor,
    const ImpellerMapping* contents,
    void* contents_on_release_user_data) {
  TextureDescriptor desc;
  desc.storage_mode = StorageMode::kHostVisible;
  desc.type = TextureType::kTexture2D;
  desc.format = ToImpellerType(descriptor->pixel_format);
  desc.size = ToImpellerType(descriptor->size);
  desc.mip_count = std::min(descriptor->mip_count, 1u);
  desc.usage = TextureUsage::kShaderRead;
  desc.compression_type = CompressionType::kLossless;
  auto texture = Create<Texture>(*GetPeer(context), desc);
  if (!texture->IsValid()) {
    VALIDATION_LOG << "Could not create texture.";
    return nullptr;
  }
  // Depending on whether the de-allocation can be delayed, it may be possible
  // to avoid a data copy.
  if (contents->on_release) {
    // Avoids data copy.
    auto wrapped_contents = std::make_shared<fml::NonOwnedMapping>(
        contents->data,    // data ptr
        contents->length,  // data length
        [on_release = contents->on_release, contents_on_release_user_data](
            auto, auto) {
          on_release(contents_on_release_user_data);
        }  // release callback
    );
    if (!texture->SetContents(std::move(wrapped_contents))) {
      VALIDATION_LOG << "Could not set texture contents.";
      return nullptr;
    }
  } else {
    // May copy.
    if (!texture->SetContents(contents->data, contents->length)) {
      VALIDATION_LOG << "Could not set texture contents.";
      return nullptr;
    }
  }
  return texture.Leak();
}

IMPELLER_EXTERN_C
ImpellerTexture ImpellerTextureCreateWithOpenGLTextureHandleNew(
    ImpellerContext context,
    const ImpellerTextureDescriptor* descriptor,
    uint64_t external_gl_handle) {
  auto impeller_context = GetPeer(context)->GetContext();
  if (impeller_context->GetBackendType() !=
      impeller::Context::BackendType::kOpenGLES) {
    VALIDATION_LOG << "Context is not OpenGL.";
    return nullptr;
  }

  const auto& impeller_context_gl =
      impeller::ContextGLES::Cast(*impeller_context);
  const auto& reactor = impeller_context_gl.GetReactor();

  TextureDescriptor desc;
  desc.storage_mode = StorageMode::kDevicePrivate;
  desc.type = TextureType::kTexture2D;
  desc.format = ToImpellerType(descriptor->pixel_format);
  desc.size = ToImpellerType(descriptor->size);
  desc.mip_count = std::min(descriptor->mip_count, 1u);
  desc.usage = TextureUsage::kShaderRead;
  desc.compression_type = CompressionType::kLossless;

  auto texture = TextureGLES::WrapTexture(
      reactor,                                                         //
      desc,                                                            //
      reactor->CreateHandle(HandleType::kTexture, external_gl_handle)  //
  );
  if (!texture || !texture->IsValid()) {
    VALIDATION_LOG << "Could not wrap external texture.";
    return nullptr;
  }
  texture->SetCoordinateSystem(TextureCoordinateSystem::kUploadFromHost);
  return Create<Texture>(impeller::Context::BackendType::kOpenGLES,
                         std::move(texture))
      .Leak();
}

IMPELLER_EXTERN_C
void ImpellerTextureRetain(ImpellerTexture texture) {
  ObjectBase::SafeRetain(texture);
}

IMPELLER_EXTERN_C
void ImpellerTextureRelease(ImpellerTexture texture) {
  ObjectBase::SafeRelease(texture);
}

IMPELLER_EXTERN_C
uint64_t ImpellerTextureGetOpenGLHandle(ImpellerTexture texture) {
  auto interop_texture = GetPeer(texture);
  if (interop_texture->GetBackendType() !=
      impeller::Context::BackendType::kOpenGLES) {
    VALIDATION_LOG << "Can only fetch the texture handle of an OpenGL texture.";
    return 0u;
  }
  return TextureGLES::Cast(*interop_texture->GetTexture())
      .GetGLHandle()
      .value_or(0u);
}

IMPELLER_EXTERN_C
void ImpellerDisplayListRetain(ImpellerDisplayList display_list) {
  ObjectBase::SafeRetain(display_list);
}

IMPELLER_EXTERN_C
void ImpellerDisplayListRelease(ImpellerDisplayList display_list) {
  ObjectBase::SafeRelease(display_list);
}

IMPELLER_EXTERN_C
ImpellerDisplayList ImpellerDisplayListBuilderCreateDisplayListNew(
    ImpellerDisplayListBuilder builder) {
  auto dl = GetPeer(builder)->Build();
  if (!dl->IsValid()) {
    return nullptr;
  }
  return dl.Leak();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawDisplayList(
    ImpellerDisplayListBuilder builder,
    ImpellerDisplayList display_list,
    float opacity) {
  GetPeer(builder)->DrawDisplayList(*GetPeer(display_list), opacity);
}

IMPELLER_EXTERN_C
ImpellerSurface ImpellerSurfaceCreateWrappedFBONew(ImpellerContext context,
                                                   uint64_t fbo,
                                                   ImpellerPixelFormat format,
                                                   const ImpellerISize* size) {
#if IMPELLER_ENABLE_OPENGLES
  if (!GetPeer(context)->IsGL()) {
    VALIDATION_LOG << "Context is not OpenGL.";
    return nullptr;
  }
  return Create<SurfaceGLES>(*GetPeer(context),       //
                             fbo,                     //
                             ToImpellerType(format),  //
                             ToImpellerType(*size))   //
      .Leak();
#else   // IMPELLER_ENABLE_OPENGLES
  VALIDATION_LOG << "OpenGL unavailable.";
  return nullptr;
#endif  // IMPELLER_ENABLE_OPENGLES
}

IMPELLER_EXTERN_C
ImpellerSurface ImpellerSurfaceCreateWrappedMetalDrawableNew(
    ImpellerContext context,
    void* metal_drawable) {
#if IMPELLER_ENABLE_METAL
  if (!GetPeer(context)->IsMetal()) {
    VALIDATION_LOG << "Context is not Metal.";
    return nullptr;
  }
  return Create<SurfaceMTL>(*GetPeer(context), metal_drawable).Leak();
#else   // IMPELLER_ENABLE_METAL
  VALIDATION_LOG << "Metal unavailable.";
  return nullptr;
#endif  // IMPELLER_ENABLE_METAL
}

IMPELLER_EXTERN_C void ImpellerSurfaceRetain(ImpellerSurface surface) {
  ObjectBase::SafeRetain(surface);
}

IMPELLER_EXTERN_C
void ImpellerSurfaceRelease(ImpellerSurface surface) {
  ObjectBase::SafeRelease(surface);
}

IMPELLER_EXTERN_C
bool ImpellerSurfaceDrawDisplayList(ImpellerSurface surface,
                                    ImpellerDisplayList display_list) {
  return GetPeer(surface)->DrawDisplayList(*GetPeer(display_list));
}

IMPELLER_EXTERN_C
bool ImpellerSurfacePresent(ImpellerSurface surface) {
  return GetPeer(surface)->Present();
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawTexture(ImpellerDisplayListBuilder builder,
                                           ImpellerTexture texture,
                                           const ImpellerPoint* point,
                                           ImpellerTextureSampling sampling,
                                           ImpellerPaint paint) {
  GetPeer(builder)->DrawTexture(*GetPeer(texture),            //
                                ToImpellerType(*point),       //
                                ToDisplayListType(sampling),  //
                                GetPeer(paint)                //
  );
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawTextureRect(
    ImpellerDisplayListBuilder builder,
    ImpellerTexture texture,
    const ImpellerRect* src_rect,
    const ImpellerRect* dst_rect,
    ImpellerTextureSampling sampling,
    ImpellerPaint paint) {
  GetPeer(builder)->DrawTextureRect(*GetPeer(texture),            //
                                    ToImpellerType(*src_rect),    //
                                    ToImpellerType(*dst_rect),    //
                                    ToDisplayListType(sampling),  //
                                    GetPeer(paint)                //
  );
}

IMPELLER_EXTERN_C
void ImpellerColorSourceRetain(ImpellerColorSource color_source) {
  ObjectBase::SafeRetain(color_source);
}

IMPELLER_EXTERN_C
void ImpellerColorSourceRelease(ImpellerColorSource color_source) {
  ObjectBase::SafeRelease(color_source);
}

static std::pair<std::vector<flutter::DlColor>, std::vector<Scalar>>
ParseColorsAndStops(uint32_t stop_count,
                    const ImpellerColor* colors,
                    const float* stops) {
  if (stop_count == 0) {
    return {};
  }
  std::pair<std::vector<flutter::DlColor>, std::vector<Scalar>> result;
  result.first.reserve(stop_count);
  result.second.reserve(stop_count);
  for (size_t i = 0; i < stop_count; i++) {
    result.first.emplace_back(ToDisplayListType(colors[i]));
    result.second.emplace_back(stops[i]);
  }
  return result;
}

IMPELLER_EXTERN_C
ImpellerColorSource ImpellerColorSourceCreateLinearGradientNew(
    const ImpellerPoint* start_point,
    const ImpellerPoint* end_point,
    uint32_t stop_count,
    const ImpellerColor* colors,
    const float* stops,
    ImpellerTileMode tile_mode,
    const ImpellerMatrix* transformation) {
  const auto colors_and_stops = ParseColorsAndStops(stop_count, colors, stops);
  return ColorSource::MakeLinearGradient(
             ToImpellerType(*start_point),  //
             ToImpellerType(*end_point),    //
             colors_and_stops.first,        //
             colors_and_stops.second,       //
             ToDisplayListType(tile_mode),  //
             transformation == nullptr ? Matrix{}
                                       : ToImpellerType(*transformation)  //
             )
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerColorSource ImpellerColorSourceCreateRadialGradientNew(
    const ImpellerPoint* center,
    float radius,
    uint32_t stop_count,
    const ImpellerColor* colors,
    const float* stops,
    ImpellerTileMode tile_mode,
    const ImpellerMatrix* transformation) {
  const auto colors_and_stops = ParseColorsAndStops(stop_count, colors, stops);
  return ColorSource::MakeRadialGradient(
             ToImpellerType(*center),       //
             radius,                        //
             colors_and_stops.first,        //
             colors_and_stops.second,       //
             ToDisplayListType(tile_mode),  //
             transformation == nullptr ? Matrix{}
                                       : ToImpellerType(*transformation)  //
             )
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerColorSource ImpellerColorSourceCreateConicalGradientNew(
    const ImpellerPoint* start_center,
    float start_radius,
    const ImpellerPoint* end_center,
    float end_radius,
    uint32_t stop_count,
    const ImpellerColor* colors,
    const float* stops,
    ImpellerTileMode tile_mode,
    const ImpellerMatrix* transformation) {
  const auto colors_and_stops = ParseColorsAndStops(stop_count, colors, stops);
  return ColorSource::MakeConicalGradient(
             ToImpellerType(*start_center),  //
             start_radius,                   //
             ToImpellerType(*end_center),    //
             end_radius,                     //
             colors_and_stops.first,         //
             colors_and_stops.second,        //
             ToDisplayListType(tile_mode),   //
             transformation == nullptr ? Matrix{}
                                       : ToImpellerType(*transformation)  //
             )
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerColorSource ImpellerColorSourceCreateSweepGradientNew(
    const ImpellerPoint* center,
    float start,
    float end,
    uint32_t stop_count,
    const ImpellerColor* colors,
    const float* stops,
    ImpellerTileMode tile_mode,
    const ImpellerMatrix* transformation) {
  const auto colors_and_stops = ParseColorsAndStops(stop_count, colors, stops);
  return ColorSource::MakeSweepGradient(
             ToImpellerType(*center),       //
             start,                         //
             end,                           //
             colors_and_stops.first,        //
             colors_and_stops.second,       //
             ToDisplayListType(tile_mode),  //
             transformation == nullptr ? Matrix{}
                                       : ToImpellerType(*transformation)  //
             )
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerColorSource ImpellerColorSourceCreateImageNew(
    ImpellerTexture image,
    ImpellerTileMode horizontal_tile_mode,
    ImpellerTileMode vertical_tile_mode,
    ImpellerTextureSampling sampling,
    const ImpellerMatrix* transformation) {
  return ColorSource::MakeImage(
             *GetPeer(image),                          //
             ToDisplayListType(horizontal_tile_mode),  //
             ToDisplayListType(vertical_tile_mode),    //
             ToDisplayListType(sampling),              //
             transformation == nullptr ? Matrix{}
                                       : ToImpellerType(*transformation)  //
             )
      .Leak();
}

struct SamplersAndUniforms {
  std::vector<std::shared_ptr<flutter::DlColorSource>> samplers;
  std::shared_ptr<std::vector<uint8_t>> uniforms;
};

static SamplersAndUniforms ReadSamplersAndUniforms(ImpellerTexture* samplers,
                                                   size_t samplers_count,
                                                   const uint8_t* data,
                                                   size_t data_bytes_length) {
  SamplersAndUniforms result;
  result.uniforms = std::make_shared<std::vector<uint8_t>>();
  if (data_bytes_length > 0) {
    std::copy(data, data + data_bytes_length,
              std::back_inserter(*result.uniforms));
  }
  if (samplers_count > 0) {
    result.samplers.reserve(samplers_count);
    for (size_t i = 0; i < samplers_count; i++) {
      result.samplers.emplace_back(flutter::DlColorSource::MakeImage(
          DlImageImpeller::Make(GetPeer(samplers[i])->GetTexture()),  //
          flutter::DlTileMode::kClamp,                                //
          flutter::DlTileMode::kClamp                                 //
          ));
    }
  }
  return result;
}

IMPELLER_EXTERN_C ImpellerColorSource
ImpellerColorSourceCreateFragmentProgramNew(
    ImpellerContext context,
    ImpellerFragmentProgram fragment_program,
    ImpellerTexture* samplers,
    size_t samplers_count,
    const uint8_t* data,
    size_t data_bytes_length) {
  auto bindings = ReadSamplersAndUniforms(samplers, samplers_count, data,
                                          data_bytes_length);
  return ColorSource::MakeFragmentProgram(*GetPeer(context),             //
                                          *GetPeer(fragment_program),    //
                                          std::move(bindings.samplers),  //
                                          std::move(bindings.uniforms)   //
                                          )
      .Leak();
}

IMPELLER_EXTERN_C
void ImpellerColorFilterRetain(ImpellerColorFilter color_filter) {
  ObjectBase::SafeRetain(color_filter);
}

IMPELLER_EXTERN_C
void ImpellerColorFilterRelease(ImpellerColorFilter color_filter) {
  ObjectBase::SafeRelease(color_filter);
}

IMPELLER_EXTERN_C
ImpellerColorFilter ImpellerColorFilterCreateBlendNew(
    const ImpellerColor* color,
    ImpellerBlendMode blend_mode) {
  return ColorFilter::MakeBlend(ToImpellerType(*color),
                                ToImpellerType(blend_mode))
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerColorFilter ImpellerColorFilterCreateColorMatrixNew(
    const ImpellerColorMatrix* color_matrix) {
  return ColorFilter::MakeMatrix(color_matrix->m).Leak();
}

IMPELLER_EXTERN_C
void ImpellerMaskFilterRetain(ImpellerMaskFilter mask_filter) {
  ObjectBase::SafeRetain(mask_filter);
}

IMPELLER_EXTERN_C
void ImpellerMaskFilterRelease(ImpellerMaskFilter mask_filter) {
  ObjectBase::SafeRelease(mask_filter);
}

IMPELLER_EXTERN_C
ImpellerMaskFilter ImpellerMaskFilterCreateBlurNew(ImpellerBlurStyle style,
                                                   float sigma) {
  return MaskFilter::MakeBlur(ToDisplayListType(style), sigma).Leak();
}

IMPELLER_EXTERN_C
void ImpellerImageFilterRetain(ImpellerImageFilter image_filter) {
  ObjectBase::SafeRetain(image_filter);
}

IMPELLER_EXTERN_C
void ImpellerImageFilterRelease(ImpellerImageFilter image_filter) {
  ObjectBase::SafeRelease(image_filter);
}

IMPELLER_EXTERN_C
ImpellerImageFilter ImpellerImageFilterCreateBlurNew(
    float x_sigma,
    float y_sigma,
    ImpellerTileMode tile_mode) {
  return ImageFilter::MakeBlur(x_sigma, y_sigma, ToDisplayListType(tile_mode))
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerImageFilter ImpellerImageFilterCreateDilateNew(float x_radius,
                                                       float y_radius) {
  return ImageFilter::MakeDilate(x_radius, y_radius).Leak();
}

IMPELLER_EXTERN_C
ImpellerImageFilter ImpellerImageFilterCreateErodeNew(float x_radius,
                                                      float y_radius) {
  return ImageFilter::MakeErode(x_radius, y_radius).Leak();
}

IMPELLER_EXTERN_C
ImpellerImageFilter ImpellerImageFilterCreateMatrixNew(
    const ImpellerMatrix* matrix,
    ImpellerTextureSampling sampling) {
  return ImageFilter::MakeMatrix(ToImpellerType(*matrix),
                                 ToDisplayListType(sampling))
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerImageFilter ImpellerImageFilterCreateFragmentProgramNew(
    ImpellerContext context,
    ImpellerFragmentProgram fragment_program,
    ImpellerTexture* samplers,
    size_t samplers_count,
    const uint8_t* data,
    size_t data_bytes_length) {
  auto bindings = ReadSamplersAndUniforms(samplers, samplers_count, data,
                                          data_bytes_length);
  return ImageFilter::MakeFragmentProgram(*GetPeer(context),             //
                                          *GetPeer(fragment_program),    //
                                          std::move(bindings.samplers),  //
                                          std::move(bindings.uniforms)   //
                                          )
      .Leak();
}

IMPELLER_EXTERN_C ImpellerImageFilter ImpellerImageFilterCreateComposeNew(
    ImpellerImageFilter outer,
    ImpellerImageFilter inner) {
  return ImageFilter::MakeCompose(*GetPeer(outer), *GetPeer(inner)).Leak();
}

IMPELLER_EXTERN_C
void ImpellerPaintSetColorFilter(ImpellerPaint paint,
                                 ImpellerColorFilter color_filter) {
  GetPeer(paint)->SetColorFilter(*GetPeer(color_filter));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetColorSource(ImpellerPaint paint,
                                 ImpellerColorSource color_source) {
  GetPeer(paint)->SetColorSource(*GetPeer(color_source));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetImageFilter(ImpellerPaint paint,
                                 ImpellerImageFilter image_filter) {
  GetPeer(paint)->SetImageFilter(*GetPeer(image_filter));
}

IMPELLER_EXTERN_C
void ImpellerPaintSetMaskFilter(ImpellerPaint paint,
                                ImpellerMaskFilter mask_filter) {
  GetPeer(paint)->SetMaskFilter(*GetPeer(mask_filter));
}

IMPELLER_EXTERN_C
ImpellerParagraphStyle ImpellerParagraphStyleNew() {
  return Create<ParagraphStyle>().Leak();
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleRetain(ImpellerParagraphStyle paragraph_style) {
  ObjectBase::SafeRetain(paragraph_style);
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleRelease(ImpellerParagraphStyle paragraph_style) {
  ObjectBase::SafeRelease(paragraph_style);
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetForeground(ImpellerParagraphStyle paragraph_style,
                                         ImpellerPaint paint) {
  GetPeer(paragraph_style)->SetForeground(Ref(GetPeer(paint)));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetBackground(ImpellerParagraphStyle paragraph_style,
                                         ImpellerPaint paint) {
  GetPeer(paragraph_style)->SetBackground(Ref(GetPeer(paint)));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetFontWeight(ImpellerParagraphStyle paragraph_style,
                                         ImpellerFontWeight weight) {
  GetPeer(paragraph_style)->SetFontWeight(ToTxtType(weight));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetFontStyle(ImpellerParagraphStyle paragraph_style,
                                        ImpellerFontStyle style) {
  GetPeer(paragraph_style)->SetFontStyle(ToTxtType(style));
}

static std::string ReadString(const char* string) {
  if (string == nullptr) {
    return "";
  }
  return std::string{string};
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetFontFamily(ImpellerParagraphStyle paragraph_style,
                                         const char* family_name) {
  GetPeer(paragraph_style)->SetFontFamily(ReadString(family_name));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetFontSize(ImpellerParagraphStyle paragraph_style,
                                       float size) {
  GetPeer(paragraph_style)->SetFontSize(size);
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetHeight(ImpellerParagraphStyle paragraph_style,
                                     float height) {
  GetPeer(paragraph_style)->SetHeight(height);
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetTextAlignment(
    ImpellerParagraphStyle paragraph_style,
    ImpellerTextAlignment align) {
  GetPeer(paragraph_style)->SetTextAlignment(ToTxtType(align));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetTextDirection(
    ImpellerParagraphStyle paragraph_style,
    ImpellerTextDirection direction) {
  GetPeer(paragraph_style)->SetTextDirection(ToTxtType(direction));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetTextDecoration(
    ImpellerParagraphStyle paragraph_style,
    const ImpellerTextDecoration* decoration) {
  GetPeer(paragraph_style)->SetTextDecoration(*decoration);
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetMaxLines(ImpellerParagraphStyle paragraph_style,
                                       uint32_t max_lines) {
  GetPeer(paragraph_style)->SetMaxLines(max_lines);
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetLocale(ImpellerParagraphStyle paragraph_style,
                                     const char* locale) {
  GetPeer(paragraph_style)->SetLocale(ReadString(locale));
}

IMPELLER_EXTERN_C
void ImpellerParagraphStyleSetEllipsis(ImpellerParagraphStyle paragraph_style,
                                       const char* ellipsis) {
  GetPeer(paragraph_style)->SetEllipsis(ReadString(ellipsis));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawParagraph(ImpellerDisplayListBuilder builder,
                                             ImpellerParagraph paragraph,
                                             const ImpellerPoint* point) {
  GetPeer(builder)->DrawParagraph(*GetPeer(paragraph), ToImpellerType(*point));
}

IMPELLER_EXTERN_C
void ImpellerDisplayListBuilderDrawShadow(ImpellerDisplayListBuilder builder,
                                          ImpellerPath path,
                                          const ImpellerColor* color,
                                          float elevation,
                                          bool occluder_is_transparent,
                                          float device_pixel_ratio) {
  GetPeer(builder)->DrawShadow(*GetPeer(path),             //
                               ToDisplayListType(*color),  //
                               elevation,                  //
                               occluder_is_transparent,    //
                               device_pixel_ratio          //
  );
}

IMPELLER_EXTERN_C
ImpellerParagraphBuilder ImpellerParagraphBuilderNew(
    ImpellerTypographyContext context) {
  auto builder =
      Create<ParagraphBuilder>(Ref<TypographyContext>(GetPeer(context)));
  if (!builder->IsValid()) {
    VALIDATION_LOG << "Could not create valid paragraph builder.";
    return nullptr;
  }
  return builder.Leak();
}

IMPELLER_EXTERN_C
void ImpellerParagraphBuilderRetain(
    ImpellerParagraphBuilder paragraph_builder) {
  ObjectBase::SafeRetain(paragraph_builder);
}

IMPELLER_EXTERN_C
void ImpellerParagraphBuilderRelease(
    ImpellerParagraphBuilder paragraph_builder) {
  ObjectBase::SafeRelease(paragraph_builder);
}

IMPELLER_EXTERN_C
void ImpellerParagraphBuilderPushStyle(
    ImpellerParagraphBuilder paragraph_builder,
    ImpellerParagraphStyle style) {
  GetPeer(paragraph_builder)->PushStyle(*GetPeer(style));
}

IMPELLER_EXTERN_C
void ImpellerParagraphBuilderPopStyle(
    ImpellerParagraphBuilder paragraph_builder) {
  GetPeer(paragraph_builder)->PopStyle();
}

IMPELLER_EXTERN_C
void ImpellerParagraphBuilderAddText(ImpellerParagraphBuilder paragraph_builder,
                                     const uint8_t* data,
                                     uint32_t length) {
  if (data == nullptr) {
    length = 0;
  }
  if (length == 0) {
    return;
  }
  GetPeer(paragraph_builder)->AddText(data, length);
}

IMPELLER_EXTERN_C
ImpellerParagraph ImpellerParagraphBuilderBuildParagraphNew(
    ImpellerParagraphBuilder paragraph_builder,
    float width) {
  return GetPeer(paragraph_builder)->Build(width).Leak();
}

IMPELLER_EXTERN_C
void ImpellerParagraphRetain(ImpellerParagraph paragraph) {
  ObjectBase::SafeRetain(paragraph);
}

IMPELLER_EXTERN_C
void ImpellerParagraphRelease(ImpellerParagraph paragraph) {
  ObjectBase::SafeRelease(paragraph);
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetMaxWidth(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetMaxWidth();
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetHeight(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetHeight();
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetLongestLineWidth(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetLongestLineWidth();
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetMinIntrinsicWidth(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetMinIntrinsicWidth();
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetMaxIntrinsicWidth(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetMaxIntrinsicWidth();
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetIdeographicBaseline(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetIdeographicBaseline();
}

IMPELLER_EXTERN_C
float ImpellerParagraphGetAlphabeticBaseline(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetAlphabeticBaseline();
}

IMPELLER_EXTERN_C
uint32_t ImpellerParagraphGetLineCount(ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetLineCount();
}

IMPELLER_EXTERN_C
void ImpellerParagraphGetWordBoundary(ImpellerParagraph paragraph,
                                      size_t code_unit_index,
                                      ImpellerRange* out_range) {
  *out_range = GetPeer(paragraph)->GetWordBoundary(code_unit_index);
}

IMPELLER_EXTERN_C
ImpellerTypographyContext ImpellerTypographyContextNew() {
  auto context = Create<TypographyContext>();
  if (!context->IsValid()) {
    VALIDATION_LOG << "Could not create typography context.";
    return nullptr;
  }
  return context.Leak();
}

IMPELLER_EXTERN_C
void ImpellerTypographyContextRetain(ImpellerTypographyContext context) {
  ObjectBase::SafeRetain(context);
}

IMPELLER_EXTERN_C
void ImpellerTypographyContextRelease(ImpellerTypographyContext context) {
  ObjectBase::SafeRelease(context);
}

IMPELLER_EXTERN_C
bool ImpellerTypographyContextRegisterFont(ImpellerTypographyContext context,
                                           const ImpellerMapping* contents,
                                           void* contents_on_release_user_data,
                                           const char* family_name_alias) {
  auto wrapped_contents = std::make_unique<fml::NonOwnedMapping>(
      contents->data,    // data ptr
      contents->length,  // data length
      [on_release = contents->on_release, contents_on_release_user_data](auto,
                                                                         auto) {
        on_release(contents_on_release_user_data);
      }  // release callback
  );
  return GetPeer(context)->RegisterFont(std::move(wrapped_contents),
                                        family_name_alias);
}

IMPELLER_EXTERN_C
ImpellerLineMetrics ImpellerParagraphGetLineMetrics(
    ImpellerParagraph paragraph) {
  return GetPeer(paragraph)->GetLineMetrics().GetC();
}

IMPELLER_EXTERN_C
ImpellerGlyphInfo ImpellerParagraphCreateGlyphInfoAtCodeUnitIndexNew(
    ImpellerParagraph paragraph,
    size_t code_unit_index) {
  return GetPeer(paragraph)
      ->GetGlyphInfoAtCodeUnitIndex(code_unit_index)
      .Leak();
}

IMPELLER_EXTERN_C
ImpellerGlyphInfo ImpellerParagraphCreateGlyphInfoAtParagraphCoordinatesNew(
    ImpellerParagraph paragraph,
    double x,
    double y) {
  return GetPeer(paragraph)
      ->GetClosestGlyphInfoAtParagraphCoordinates(x, y)
      .Leak();
}

//------------------------------------------------------------------------------
// Line Metrics
//------------------------------------------------------------------------------

IMPELLER_EXTERN_C
void ImpellerLineMetricsRetain(ImpellerLineMetrics line_metrics) {
  ObjectBase::SafeRetain(line_metrics);
}

IMPELLER_EXTERN_C
void ImpellerLineMetricsRelease(ImpellerLineMetrics line_metrics) {
  ObjectBase::SafeRelease(line_metrics);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetUnscaledAscent(ImpellerLineMetrics metrics,
                                            size_t line) {
  return GetPeer(metrics)->GetUnscaledAscent(line);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetAscent(ImpellerLineMetrics metrics, size_t line) {
  return GetPeer(metrics)->GetAscent(line);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetDescent(ImpellerLineMetrics metrics, size_t line) {
  return GetPeer(metrics)->GetDescent(line);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetBaseline(ImpellerLineMetrics metrics,
                                      size_t line) {
  return GetPeer(metrics)->GetBaseline(line);
}

IMPELLER_EXTERN_C
bool ImpellerLineMetricsIsHardbreak(ImpellerLineMetrics metrics, size_t line) {
  return GetPeer(metrics)->IsHardbreak(line);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetWidth(ImpellerLineMetrics metrics, size_t line) {
  return GetPeer(metrics)->GetWidth(line);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetHeight(ImpellerLineMetrics metrics, size_t line) {
  return GetPeer(metrics)->GetHeight(line);
}

IMPELLER_EXTERN_C
double ImpellerLineMetricsGetLeft(ImpellerLineMetrics metrics, size_t line) {
  return GetPeer(metrics)->GetLeft(line);
}

IMPELLER_EXTERN_C
size_t ImpellerLineMetricsGetCodeUnitStartIndex(ImpellerLineMetrics metrics,
                                                size_t line) {
  return GetPeer(metrics)->GetCodeUnitStartIndex(line);
}

IMPELLER_EXTERN_C
size_t ImpellerLineMetricsGetCodeUnitEndIndex(ImpellerLineMetrics metrics,
                                              size_t line) {
  return GetPeer(metrics)->GetCodeUnitEndIndex(line);
}

IMPELLER_EXTERN_C
size_t ImpellerLineMetricsGetCodeUnitEndIndexExcludingWhitespace(
    ImpellerLineMetrics metrics,
    size_t line) {
  return GetPeer(metrics)->GetCodeUnitEndIndexExcludingWhitespace(line);
}

IMPELLER_EXTERN_C
size_t ImpellerLineMetricsGetCodeUnitEndIndexIncludingNewline(
    ImpellerLineMetrics metrics,
    size_t line) {
  return GetPeer(metrics)->GetCodeUnitEndIndexIncludingNewline(line);
}

//------------------------------------------------------------------------------
// Glyph Info
//------------------------------------------------------------------------------

IMPELLER_EXTERN_C
void ImpellerGlyphInfoRetain(ImpellerGlyphInfo glyph_info) {
  ObjectBase::SafeRetain(glyph_info);
}

IMPELLER_EXTERN_C
void ImpellerGlyphInfoRelease(ImpellerGlyphInfo glyph_info) {
  ObjectBase::SafeRelease(glyph_info);
}

IMPELLER_EXTERN_C
size_t ImpellerGlyphInfoGetGraphemeClusterCodeUnitRangeBegin(
    ImpellerGlyphInfo glyph_info) {
  return GetPeer(glyph_info)->GetGraphemeClusterCodeUnitRangeBegin();
}

IMPELLER_EXTERN_C
size_t ImpellerGlyphInfoGetGraphemeClusterCodeUnitRangeEnd(
    ImpellerGlyphInfo glyph_info) {
  return GetPeer(glyph_info)->GetGraphemeClusterCodeUnitRangeEnd();
}

IMPELLER_EXTERN_C
void ImpellerGlyphInfoGetGraphemeClusterBounds(ImpellerGlyphInfo glyph_info,
                                               ImpellerRect* out_bounds) {
  *out_bounds = GetPeer(glyph_info)->GetGraphemeClusterBounds();
}

IMPELLER_EXTERN_C
bool ImpellerGlyphInfoIsEllipsis(ImpellerGlyphInfo glyph_info) {
  return GetPeer(glyph_info)->IsEllipsis();
}

IMPELLER_EXTERN_C
ImpellerTextDirection ImpellerGlyphInfoGetTextDirection(
    ImpellerGlyphInfo glyph_info) {
  return GetPeer(glyph_info)->GetTextDirection();
}

//------------------------------------------------------------------------------
// Fragment Program
//------------------------------------------------------------------------------

static std::shared_ptr<fml::Mapping> MakeCopyableMapping(
    const ImpellerMapping* data,
    void* user_data) {
  if (data == nullptr) {
    return nullptr;
  }

  // If the user has supplied a release callback, defer deallocation to them.
  if (auto callback = data->on_release) {
    return std::make_shared<fml::NonOwnedMapping>(
        data->data,    //
        data->length,  //
        [user_data, callback](const uint8_t* data, size_t size) {
          callback(user_data);
        });
  }

  return std::make_shared<fml::MallocMapping>(
      fml::MallocMapping::Copy(data->data, data->length));
}

IMPELLER_EXTERN_C
ImpellerFragmentProgram ImpellerFragmentProgramNew(
    const ImpellerMapping* data,
    void* data_release_user_data) {
  auto data_mapping = MakeCopyableMapping(data, data_release_user_data);
  auto program = Create<FragmentProgram>(std::move(data_mapping));
  if (!program->IsValid()) {
    VALIDATION_LOG << "Could not create valid fragment program.";
    return nullptr;
  }
  return program.Leak();
}

IMPELLER_EXTERN_C
void ImpellerFragmentProgramRetain(ImpellerFragmentProgram fragment_program) {
  ObjectBase::SafeRetain(fragment_program);
}

IMPELLER_EXTERN_C
void ImpellerFragmentProgramRelease(ImpellerFragmentProgram fragment_program) {
  ObjectBase::SafeRelease(fragment_program);
}

}  // namespace impeller::interop
