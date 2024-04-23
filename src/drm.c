#include <drm.h>
#include <fcntl.h>
#include <stdio.h>

#include <debug_config.h>

int initDrm()
{
	int ret, fd = 0;
	drmModeResPtr res;

#ifndef AMD
	fd = open("/dev/dri/card0", O_RDWR);
#endif
#ifdef AMD
	fd = open("/dev/dri/card1", O_RDWR);
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

	drmModeFreeResources(res);

	return 0;
}