
#include <stdlib.h>

#include "generation.h"
#include "utils.h"

short* GenerateCave(int width, int height, float alive_chance, int number_of_steps, int death_limit, int birth_limit) {

	CaveMap map;
	map.width = width;
	map.height = height;
	map.alive_chance = alive_chance;
	map.number_of_steps = number_of_steps;
	map.death_limit = death_limit;
	map.birth_limit = birth_limit;
	map.cells = malloc(width * height * sizeof(short));

	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++) {

			size_t index = y * height + x;

			if (rnd_float() < alive_chance) map.cells[index] = 1;
			else map.cells[index] = 0;

		}
	}

	for (int i = 0; i < number_of_steps; i++) {
		CaveDoSimulationStep(&map);
	}

	return map.cells;

}

void CaveDoSimulationStep(CaveMap* map) {

	short* new_cells = malloc(map->width * map->height * sizeof(short));

	for (size_t y = 0; y < map->height; y++) {
		for (size_t x = 0; x < map->width; x++) {

			size_t index = y * map->height + x;

			int nbs = CaveCountAliveNeighbours(map, x, y);

			if (map->cells[index] == 1) {
				if (nbs < map->death_limit) new_cells[index] = 0;
				else new_cells[index] = 1;
			} else {
				if (nbs > map->birth_limit) new_cells[index] = 1;
				else new_cells[index] = 0;
			}

		}
	}

	free(map->cells);
	map->cells = new_cells;

}

int CaveCountAliveNeighbours(CaveMap* map, int x, int y) {

	int count = 0;

	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {

			int neighbour_x = x + i;
			int neighbour_y = y + j;

			int index = neighbour_y * map->height + neighbour_x;

			if (i == 0 && j == 0) {

			} else if (neighbour_x < 0 || neighbour_y < 0 || neighbour_x >= map->width || neighbour_y >= map->height) {
				count++;
			} else if (map->cells[index] == 1) {
				count++;
			}

		}
	}

	return count;

}
