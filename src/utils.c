
#include <stdlib.h>

#include "utils.h"

int rnd_int(int lower, int upper) {
	return (rand() % (upper - lower + 1)) + lower;
}

float rnd_float() {
	return (float) rand() / (float) (RAND_MAX);
}

float lerp(float v0, float v1, float t) {
	return (1 - t) * v0 + t * v1;
}
