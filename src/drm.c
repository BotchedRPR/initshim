#include <drm.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>


struct type_name {
    unsigned int type;
    const char *name;
};

static const struct type_name connector_type_names[] = {
        { DRM_MODE_CONNECTOR_Unknown, "unknown" },
        { DRM_MODE_CONNECTOR_VGA, "VGA" },
        { DRM_MODE_CONNECTOR_DVII, "DVI-I" },
        { DRM_MODE_CONNECTOR_DVID, "DVI-D" },
        { DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
        { DRM_MODE_CONNECTOR_Composite, "composite" },
        { DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
        { DRM_MODE_CONNECTOR_LVDS, "LVDS" },
        { DRM_MODE_CONNECTOR_Component, "component" },
        { DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
        { DRM_MODE_CONNECTOR_DisplayPort, "DP" },
        { DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
        { DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
        { DRM_MODE_CONNECTOR_TV, "TV" },
        { DRM_MODE_CONNECTOR_eDP, "eDP" },
        { DRM_MODE_CONNECTOR_VIRTUAL, "Virtual" },
        { DRM_MODE_CONNECTOR_DSI, "DSI" },
        { DRM_MODE_CONNECTOR_DPI, "DPI" },
};

const char *connector_type_name(unsigned int type)
{
    if (type < ARRAY_SIZE(connector_type_names) && type >= 0) {
        return connector_type_names[type].name;
    }

    return "INVALID";
}

void releaseDumbFB(struct framebuffer *fb)
{
    if (fb->fd) {
        /* Try to become master again, else we can't set CRTC. Then the current master needs to reset everything. */
        drmSetMaster(fb->fd);
        if (fb->crtc) {
            /* Set back to orignal frame buffer */
            drmModeSetCrtc(fb->fd, fb->crtc->crtc_id, fb->crtc->buffer_id, 0, 0, &fb->connector->connector_id, 1, fb->resolution);
            drmModeFreeCrtc(fb->crtc);
        }
        if (fb->buffer_id)
            drmModeFreeFB(drmModeGetFB(fb->fd, fb->buffer_id));
        /* This will also release resolution */
        if (fb->connector) {
            drmModeFreeConnector(fb->connector);
            fb->resolution = 0;
        }
        if (fb->dumb_framebuffer.handle)
            ioctl(fb->fd, DRM_IOCTL_MODE_DESTROY_DUMB, fb->dumb_framebuffer);
        close(fb->fd);
    }

}

int createDumbFB(int *fd, drmModeResPtr *res, struct framebuffer *fb)
{
    drmModeConnectorPtr connectorPtr = 0;
    drmModeEncoderPtr encoder = 0;

    /* Get the DSI-1 connector */
    for(int i = 0; i < (*res)->count_connectors; i++) {
        char name[32];

        connectorPtr = drmModeGetConnectorCurrent(*fd, (*res)->connectors[i]);
        if (!connectorPtr) {
            continue;
        }

        snprintf(name, sizeof(name), "%s-%u", connector_type_name(connectorPtr->connector_type),
                 connectorPtr->connector_type_id);

        if (strncmp(name, "DSI-1", sizeof(name)) == 0)
            break;

        drmModeFreeConnector(connectorPtr);
        connectorPtr = 0;
    }

    if(!connectorPtr)
    {
        return 5;
    }

    /* Make sure we have at least 1 mode */
    if(connectorPtr->count_modes == 0)
    {
        return 6;
    }

    /* Get the "preferred" resolution */
    drmModeModeInfoPtr resolution = 0;
    if(connectorPtr->count_modes == 3)
    {
        /* Preferred is 60hz, we bump it up to 120 */
        resolution = &connectorPtr->modes[2]; // ZTE Z60 Ultra / REDMAGIC 9 Pro 120Hz
    }
    else
    {
        /* If not a Z60/REDMAGIC, we just use the preferred one */
        for(int i = 0; i < connectorPtr->count_modes; i++)
        {
            resolution = &connectorPtr->modes[i];
            if(resolution->type & DRM_MODE_TYPE_PREFERRED)
                break;
        }
    }
    fb->dumb_framebuffer.height = resolution->vdisplay;
    fb->dumb_framebuffer.width = resolution->hdisplay;
    fb->dumb_framebuffer.bpp = 32;

    int err = 0;

    /* Create the dumb framebuffer, map to fb device */
    err = ioctl(*fd, DRM_IOCTL_MODE_CREATE_DUMB, &fb->dumb_framebuffer);
    if(err)
    {
        releaseDumbFB(fb);
        return err;
    }

    /* Add the framebuffer to drm */
    err = drmModeAddFB(*fd, resolution->hdisplay, resolution->vdisplay, 24, 32,
                       fb->dumb_framebuffer.pitch, fb->dumb_framebuffer.handle, &fb->buffer_id);
    if(err)
    {
        releaseDumbFB(fb);
        return err;
    }

    /* Get the encoder and crtc info */
    encoder = drmModeGetEncoder(*fd, connectorPtr->encoder_id);
    if(!encoder)
    {
        err = 10;
        releaseDumbFB(fb);
        drmModeFreeEncoder(encoder);
    }

    fb->crtc = drmModeGetCrtc(*fd, encoder->crtc_id);

    struct drm_mode_map_dumb mreq;

    memset(&mreq, 0, sizeof(mreq));
    mreq.handle = fb->dumb_framebuffer.handle;

    /* Mapping dumb framebuffer */
    err = drmIoctl(*fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    if(err)
    {
        releaseDumbFB(fb);
        drmModeFreeEncoder(encoder);
    }

    fb->data = mmap(0, fb->dumb_framebuffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, mreq.offset);
    if (fb->data == MAP_FAILED) {
        err = 11;
        releaseDumbFB(fb);
        drmModeFreeEncoder(encoder);
    }

    /* Finishing up */
    fb->fd = *fd;
    fb->connector = connectorPtr;
    fb->resolution = resolution;
}

int initDrm()
{
	int ret, fd = 0;
	drmModeResPtr res;

#ifndef AMD
	fd = open("/dev/dri/card0", O_RDWR);
#endif
	if(fd < 0)
	{
		printf("Couldn't find drm (dri) device.");
		return fd;
	}

	res = drmModeGetResources(fd);
	if(!res)
	{
		printf("Couldn't find resources for drm (dri) device.");
		return 1;
	}

	printf("\n---DEBUG INFO: DRM DEVICE---\n");

	printf("\nFB: ");
	for (int i = 0; i < res->count_fbs; i++) {
		printf("%d ", res->fbs[i]);
	}

	printf("\nCRTC: ");
	for (int i = 0; i < res->count_crtcs; i++) {
		printf("%d ", res->crtcs[i]);
	}

	printf("\nencoder: ");
	for (int i = 0; i < res->count_encoders; i++) {
		printf("%d ", res->encoders[i]);
	}

    printf("\nMapping memory for dumb framebuffer device");
    struct framebuffer fb;
    memset(&fb, 0, sizeof(fb));

    createDumbFB(&fd, &res, &fb);

	drmModeFreeResources(res);

	return 0;
}