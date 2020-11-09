#include "libdroplay.h"
#include "emu8950.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ao/ao.h>
#pragma RcB2 LINK "-lao"

#define MSX_CLK 3579545
#define SAMPLERATE 44100
#define NCHANNELS 1
#define FLUSH_SAMPLES 128

struct AoWriter {
	ao_device *device;
	ao_sample_format format;
	int aodriver;
};

static int AoWriter_init(struct AoWriter *self) {
	ao_initialize();
	memset(self, 0, sizeof(*self));
	self->format.bits = 16;
	self->format.channels = NCHANNELS;
	self->format.rate = SAMPLERATE;
	self->format.byte_format = AO_FMT_LITTLE;
	self->aodriver = ao_default_driver_id();
	self->device = ao_open_live(self->aodriver, &self->format, NULL);
	return self->device != NULL;
}

static int AoWriter_write(struct AoWriter *self, void* buffer, size_t bufsize) {
	return ao_play(self->device, buffer, bufsize);
}

static int AoWriter_close(struct AoWriter *self) {
	return ao_close(self->device);
}

struct droplayer {
	FILE *f;
	OPL *chip[2];
	dro2hdr hdr;
	struct AoWriter ao;
	int16_t sndbuf[FLUSH_SAMPLES];
	unsigned bufpos;
};

static void flush_buffer(droplayer d) {
	AoWriter_write(&d->ao, d->sndbuf, d->bufpos*2*NCHANNELS);
	d->bufpos = 0;
}

static void flush_buffer_if_needed(droplayer d) {
	if(d->bufpos >= FLUSH_SAMPLES) flush_buffer(d);
}

static void WORD(char *buf, uint32_t data) {
	buf[0] = data & 0xff;
	buf[1] = (data & 0xff00) >> 8;
}

static int16_t get_sample(droplayer d) {
	union {
		uint8_t b8[2];
		uint16_t b16[1];
	} buf;
	WORD(buf.b8, OPL_calc(d->chip[0]));
	return buf.b16[0];
}

static void write_reg(droplayer d, int chipno, unsigned reg, unsigned val) {
	if(chipno == 0)
		OPL_writeReg(d->chip[0], reg, val);
	/* we don't support 2 OPL chips yet */
}

static void produce_samples(droplayer d, unsigned delay) {
	unsigned long i, samples_required = SAMPLERATE*delay/1000;
	for(i=0; i < samples_required; ++i) {
		d->sndbuf[d->bufpos++] = get_sample(d);
		flush_buffer_if_needed(d);
	}
}

/* process OPL commands until a delay is encountered.
   return 0 on file end, or delay in milliseconds. */
static unsigned dro2play(droplayer d) {
	while(1) {
		unsigned char code[2];

		if(feof(d->f))
			return 0;
		if(fread(code, 1, 2, d->f) != 2)
			return 0;
		if(code[0] == d->hdr.iShortDelayCode)
			return code[1] + 1;
		else if(code[0] == d->hdr.iLongDelayCode)
			return (code[1] + 1) << 8;

		unsigned reg = d->hdr.iCodemap[code[0] & 0x7f];
		write_reg(d, code[0] >> 7, reg, code[1]);
	}
}

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))
const char *droplayer_strerror(int err) {
	static const char etab[][20] = {
	[0] = "unknown error code\0",
	[1] = "short read\0        ",
	[2] = "unknown format\0    ",
	[3] = "invalid header\0    ",
	[4] = "unsupported version",
	[5] = "fopen failed\0      ",
	};
	if(err >= ARRAY_SIZE(etab) || err < 0) err = 0;
	return etab[err];
}

static uint16_t readUINT16LE(FILE *f, int *err) {
	unsigned char c[2];
	if(2 != fread(c, 1, 2, f)) { *err = 1; return 0; };
	*err = 0;
	return c[0] | (c[1] << 8);
}
static uint32_t readUINT32LE(FILE *f, int *err) {
	unsigned char c[4];
	if(4 != fread(c, 1, 4, f)) { *err = 1; return 0; };
	*err = 0;
	return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
}

static int read_header(FILE *f, struct dro2hdr *hdr) {
	char buf[9];
	buf[8] = 0;
	int err;
	uint16_t version[2];
	if(8 != fread(buf, 1, 8, f)) return 1;
	if(strcmp(buf, "DBRAWOPL")) return 2;
	version[0] = readUINT16LE(f, &err);
	if(err) return 1;
	version[1] = readUINT16LE(f, &err);
	if(err) return 1;
	if(!(version[0] == 2 && version[1] == 0)) return 4;
	hdr->iLengthPairs = readUINT32LE(f, &err);
	if(err) return 1;
	hdr->iLengthMS = readUINT32LE(f, &err);
	if(err) return 1;
	if(6 != fread(&hdr->iHardwareType, 1, 6, f)) return 1;
	err = hdr->iCodemapLength;
	if(err >= 128) return 3;
	if(err != fread(hdr->iCodemap, 1, err, f)) return 1;
	return 0;
}

droplayer droplayer_new(void) {
	droplayer d = calloc(sizeof (struct droplayer), 1);
	AoWriter_init(&d->ao);
	d->chip[0] = OPL_new(MSX_CLK, SAMPLERATE);
	d->chip[1] = 0;
	OPL_setChipType(d->chip[0], 2);
	d->bufpos = 0;
	d->f = 0;
	return d;
}

int droplayer_fopen(droplayer d, const char *filename) {
	d->f = fopen(filename, "r");
	if(!d->f) return 5;
	return read_header(d->f, &d->hdr);
}

void droplayer_delete(droplayer d) {
	AoWriter_close(&d->ao);
	fclose(d->f);
	d->f = 0;
	OPL_delete(d->chip[0]);
	d->chip[0] = 0;
	free(d);
}

void droplayer_play(droplayer d) {
	unsigned delay;
	while((delay = dro2play(d)))
		produce_samples(d, delay);
	flush_buffer(d);
}

