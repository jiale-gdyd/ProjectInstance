/* GStreamer
 * Copyright (C) <2018-2019> Seungha Yang <seungha.yang@navercorp.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_CUDA_UTILS_H__
#define __GST_CUDA_UTILS_H__

#ifndef GST_USE_UNSTABLE_API
#warning "The Cuda library from gst-plugins-bad is unstable API and may change in future."
#warning "You can define GST_USE_UNSTABLE_API to avoid this warning."
#endif

#include "cuda-prelude.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include "cuda-gst.h"
#include "gstcudaloader.h"
#include "gstcudacontext.h"
#include "gstcudamemory.h"

G_BEGIN_DECLS

#ifndef GST_DISABLE_GST_DEBUG
static inline gboolean
_gst_cuda_debug(CUresult result, GstDebugCategory * category,
    const gchar * file, const gchar * function, gint line)
{
  const gchar *_error_name, *_error_text;
  if (result != CUDA_SUCCESS) {
    CuGetErrorName (result, &_error_name);
    CuGetErrorString (result, &_error_text);
    gst_debug_log (category, GST_LEVEL_WARNING, file, function, line,
        NULL, "CUDA call failed: %s, %s", _error_name, _error_text);

    return FALSE;
  }

  return TRUE;
}

/**
 * gst_cuda_result:
 * @result: CUDA device API return code `CUresult`
 *
 * Returns: %TRUE if CUDA device API call result is CUDA_SUCCESS
 */
#define gst_cuda_result(result) \
  _gst_cuda_debug(result, GST_CAT_DEFAULT, __FILE__, GST_FUNCTION, __LINE__)
#else

static inline gboolean
_gst_cuda_debug(CUresult result, GstDebugCategory * category,
    const gchar * file, const gchar * function, gint line)
{
  return result == CUDA_SUCCESS;
}

/**
 * gst_cuda_result:
 * @result: CUDA device API return code `CUresult`
 *
 * Returns: %TRUE if CUDA device API call result is CUDA_SUCCESS
 *
 * Since: 1.22
 */
#define gst_cuda_result(result) \
  _gst_cuda_debug(result, NULL, __FILE__, GST_FUNCTION, __LINE__)
#endif

/**
 * GstCudaQuarkId:
 *
 * Since: 1.22
 */

typedef enum
{
  GST_CUDA_QUARK_GRAPHICS_RESOURCE = 0,

  /* end of quark list */
  GST_CUDA_QUARK_MAX = 1
} GstCudaQuarkId;

/**
 * GstCudaGraphicsResourceType:
 * @GST_CUDA_GRAPHICS_RESSOURCE_NONE: Ressource represents a CUDA buffer.
 * @GST_CUDA_GRAPHICS_RESSOURCE_GL_BUFFER: Ressource represents a GL buffer.
 * @GST_CUDA_GRAPHICS_RESSOURCE_D3D11_RESOURCE: Ressource represents a D3D resource.
 *
 * Since: 1.22
 */
typedef enum
{
  GST_CUDA_GRAPHICS_RESOURCE_NONE = 0,
  GST_CUDA_GRAPHICS_RESOURCE_GL_BUFFER = 1,
  GST_CUDA_GRAPHICS_RESOURCE_D3D11_RESOURCE = 2,
} GstCudaGraphicsResourceType;

/**
 * GstCudaGraphicsResource:
 *
 * Since: 1.22
 */
typedef struct _GstCudaGraphicsResource
{
  GstCudaContext *cuda_context;
  /* GL context or D3D11 device */
  GstObject *graphics_context;

  GstCudaGraphicsResourceType type;
  CUgraphicsResource resource;
  CUgraphicsRegisterFlags flags;

  gboolean registered;
  gboolean mapped;
} GstCudaGraphicsResource;

GST_CUDA_API
gboolean        gst_cuda_ensure_element_context (GstElement * element,
                                                 gint device_id,
                                                 GstCudaContext ** cuda_ctx);

GST_CUDA_API
gboolean        gst_cuda_handle_set_context     (GstElement * element,
                                                 GstContext * context,
                                                 gint device_id,
                                                 GstCudaContext ** cuda_ctx);

GST_CUDA_API
gboolean        gst_cuda_handle_context_query   (GstElement * element,
                                                 GstQuery * query,
                                                 GstCudaContext * cuda_ctx);

GST_CUDA_API
GstContext *    gst_context_new_cuda_context    (GstCudaContext * cuda_ctx);

GST_CUDA_API
GQuark          gst_cuda_quark_from_id          (GstCudaQuarkId id);

GST_CUDA_API
GstCudaGraphicsResource * gst_cuda_graphics_resource_new  (GstCudaContext * context,
                                                           GstObject * graphics_context,
                                                           GstCudaGraphicsResourceType type);

GST_CUDA_API
gboolean        gst_cuda_graphics_resource_register_gl_buffer (GstCudaGraphicsResource * resource,
                                                               guint buffer,
                                                               CUgraphicsRegisterFlags flags);

#ifdef G_OS_WIN32
GST_CUDA_API
gboolean        gst_cuda_graphics_resource_register_d3d11_resource (GstCudaGraphicsResource * resource,
                                                                    ID3D11Resource * d3d11_resource,
                                                                    CUgraphicsRegisterFlags flags);
#endif

GST_CUDA_API
void            gst_cuda_graphics_resource_unregister (GstCudaGraphicsResource * resource);

GST_CUDA_API
CUgraphicsResource gst_cuda_graphics_resource_map (GstCudaGraphicsResource * resource,
                                                   CUstream stream,
                                                   CUgraphicsMapResourceFlags flags);

GST_CUDA_API
void            gst_cuda_graphics_resource_unmap (GstCudaGraphicsResource * resource,
                                                  CUstream stream);

GST_CUDA_API
void            gst_cuda_graphics_resource_free (GstCudaGraphicsResource * resource);

G_END_DECLS

#endif /* __GST_CUDA_UTILS_H__ */
