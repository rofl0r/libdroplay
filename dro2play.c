#include "libdroplay.h"

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv) {
	if (argc < 2) {
		printf("usage: %s filename\n", argv[0]);
		return 1;
	}
	const char* filename = argv[1];
	droplayer d = droplayer_new();
	int ret;
	if((ret = droplayer_fopen(d, filename))) {
		fprintf(stderr, "error: loading DRO2 file format failed: %s\n", droplayer_strerror(ret));
	}
	droplayer_play(d);
	droplayer_delete(d);
	return 0;
}

