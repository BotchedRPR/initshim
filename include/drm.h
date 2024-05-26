#ifndef INITSHIM_DRM_H
#define INITSHIM_DRM_H

#if __has_include(<drm/drm.h>) && __has_include(<drm/drm_mode.h>)
#include <drm/drm.h>
#include <drm/drm_mode.h>
#else
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#endif
#include <xf86drm.h>
#include <xf86drmMode.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

struct framebuffer {
    int fd;
    uint32_t buffer_id;
    uint16_t res_x;
    uint16_t res_y;
    uint8_t *data;
    uint32_t size;
    struct drm_mode_create_dumb dumb_framebuffer;
    drmModeCrtcPtr crtc;
    drmModeConnectorPtr connector;
    drmModeModeInfoPtr resolution;
};

int initDrm();
const char *connector_type_name(unsigned int type);

#endif //INITSHIM_DRM_H
