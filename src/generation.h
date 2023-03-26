
#ifndef GENERATION_H
#define GENERATION_H

typedef struct {
	int width;
	int height;
	float alive_chance;
	int number_of_steps;
	int death_limit;
	int birth_limit;
	short* cells;
} CaveMap;

short* GenerateCave(int width, int height, float alive_chance, int number_of_steps, int death_limit, int birth_limit);
void CaveDoSimulationStep(CaveMap* map);
int CaveCountAliveNeighbours(CaveMap* map, int x, int y);

#endif
