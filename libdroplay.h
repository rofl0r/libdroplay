#ifndef DROPLAY_H
#define DROPLAY_H

#pragma RcB2 DEP "libdroplay.c"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dro2hdr {
	uint32_t iLengthPairs;
	uint32_t iLengthMS;
	uint8_t iHardwareType;
	uint8_t iFormat;
	uint8_t iCompression;
	uint8_t iShortDelayCode;
	uint8_t iLongDelayCode;
	uint8_t iCodemapLength;
	uint8_t iCodemap[128];
} dro2hdr;

struct droplayer_;

typedef struct droplayer_* droplayer;

droplayer droplayer_new(void);

/* return 0 on success, else error number */
int droplayer_fopen(droplayer d, const char *filename);

/* use this to get a string for an error returned by the above */
const char *droplayer_strerror(int err);

void droplayer_play(droplayer d);

void droplayer_delete(droplayer d);

#ifdef __cplusplus
}
#endif

#endif
