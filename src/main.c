#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "list.h"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

const int window_width = 1280;
const int window_height = 720;
const int rendering_scale = 8;
const int tile_size = 32;

typedef enum {
	TILE_TYPE_MIN,
	TILE_TYPE_SPACE,
	TILE_TYPE_WALL,
	TILE_TYPE_MAX
} TileType;

typedef enum {
	ACTOR_TYPE_MIN,
	ACTOR_TYPE_PLAYER,
	ACTOR_TYPE_ENEMY,
	ACTOR_TYPE_MAX
} ActorType;

typedef struct {
	int r;
	int g;
	int b;
	int a;
} Color;

const Color COLOR_WHITE = {255, 255, 255, 255};
const Color COLOR_BLACK = {0, 0, 0, 255};
const Color COLOR_RED = {255, 0, 0, 255};
const Color COLOR_GREEN = {0, 255, 0, 255};
const Color COLOR_BLUE = {0, 0, 255, 255};

typedef struct {
	char* name;
	SDL_Texture* texture;
	int src_width;
	int src_height;
} Texture;

typedef struct {
	Texture* texture;
	int width;
	int height;
	Color color;
} Sprite;

typedef struct {
	int x;
	int y;
} Position;

typedef struct {
	ActorType actor_type;
	Sprite* sprite;
} Actor;

typedef struct {
	int index;
	TileType tile_type;
	Sprite* sprite;
	Actor* actor;
} Tile;

typedef struct {
	int width;
	int height;
	Tile** tiles;
} Dungeon;

typedef struct {
	int running;
	SDL_Renderer* renderer;
	List* textures_list;
	Dungeon* dungeon;
} Game;

int rnd_int(int lower, int upper) {
	return (rand() % (upper - lower + 1)) + lower;
}

Texture* LoadTexture(Game* game, char* file_path, char* texture_name) {

	SDL_Texture* loaded_texture = IMG_LoadTexture(game->renderer, file_path);

	if (loaded_texture == NULL) {
		SDL_Log("Texture loading error!\n%s\n", IMG_GetError());
		game->running = -1;
		return NULL;
	}

	int texture_width, texture_height;
	SDL_QueryTexture(loaded_texture, NULL, NULL, &texture_width, &texture_height);

	SDL_Log("Loaded texture from file \"%s\", size %dx%d\n", file_path, texture_width, texture_height);

	Texture* texture = malloc(sizeof(Texture));
	texture->name = texture_name;
	texture->texture = loaded_texture;
	texture->src_width = texture_width;
	texture->src_height = texture_height;

	SDL_Log("Created texture with name \"%s\"\n", texture_name);

	return texture;
}

void FreeTexture(Texture* texture) {

	SDL_Log("Releasing memory occupied for loaded texture \"%s\".", texture->name);

	SDL_DestroyTexture(texture->texture);
	free(texture);

}

void LoadTextures(Game* game) {

	SDL_Log("Loading game textures...");

	game->textures_list = CreateList(100, sizeof(Texture), "Texture");
	ListPush(game->textures_list, LoadTexture(game, "sprites/player.png", "player")); // (void*)
	ListPush(game->textures_list, LoadTexture(game, "sprites/wall.png", "wall")); // (void*)
	ListPrint(game->textures_list);

	SDL_Log("Loading game textures complete.");

}

void FreeTextures(List* textures_list) {
	
	SDL_Log("Releasing memory occupied for loaded textures...");

	for (size_t i = 0; i < textures_list->cursor; i++) {
		Texture* texture = textures_list->elements[i];
		FreeTexture(texture);
	}

	FreeList(textures_list);

}

Texture* TextureGetByName(List* textures_list, char* texture_name) {
	for (size_t i = 0; i < textures_list->cursor; i++) {
		Texture* texture = textures_list->elements[i];
		if (texture->name == texture_name) {
			return texture;
		}
	}
	SDL_Log("-> could not find texture by name: %s\n", texture_name);
	return NULL;
}

Sprite* CreateSprite(Game* game, char* texture_name) {

	Sprite* sprite = malloc(sizeof(Sprite));
	sprite->texture = TextureGetByName(game->textures_list, texture_name);

	if (sprite->texture == NULL) {
		SDL_Log("Create sprite error!\n");
		return NULL;
	}

	sprite->width = sprite->texture->src_width;
	sprite->height = sprite->texture->src_height;

	sprite->color = COLOR_WHITE;

	SDL_Log("Created sprite from texture \"%s\", size %dx%d\n", texture_name, sprite->width, sprite->height);

	return sprite;
}

void FreeSprite(Sprite* sprite) {
	free(sprite);
}

void RenderSprite(Game* game, int x, int y, Sprite* sprite) {

	SDL_SetTextureColorMod(sprite->texture->texture, sprite->color.r, sprite->color.g, sprite->color.b);
	SDL_SetTextureAlphaMod(sprite->texture->texture, sprite->color.a);

	SDL_Rect dstrect = {x, y, sprite->width * rendering_scale, sprite->height * rendering_scale};

	SDL_RenderCopy(game->renderer, sprite->texture->texture, NULL, &dstrect);

}

Actor* CreateActor(ActorType actor_type, Sprite* sprite) {
	
	Actor* actor = malloc(sizeof(Actor));
	actor->actor_type = actor_type;
	actor->sprite = sprite;

	return actor;

}

void FreeActor(Actor* actor) {

	SDL_Log("Releasing memory occupied for actor...\n");

	FreeSprite(actor->sprite);
	free(actor);

}

void DungeonPlaceActor(Dungeon* dungeon, int x, int y, Actor* actor) {
	int tile_index = y * dungeon->height + x;
	Tile* tile = dungeon->tiles[tile_index];
	tile->actor = actor;
}

Dungeon* CreateDungeon(int width, int height) {

	Dungeon* dungeon = malloc(sizeof(Dungeon));

	dungeon->width = width;
	dungeon->height = height;
	dungeon->tiles = malloc(width * height * sizeof(Tile));

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int tile_index = y * height + x;

			Tile* tile = malloc(sizeof(Tile));
			tile->index = tile_index;
			tile->tile_type = TILE_TYPE_SPACE;
			tile->actor = NULL;

			dungeon->tiles[tile_index] = tile;
		}
	}

	SDL_Log("Created dungeon %dx%d\n", dungeon->width, dungeon->height);

	return dungeon;
}

void FreeDungeon(Dungeon* dungeon) {

	SDL_Log("Releasing memory occupied for dungeon...\n");

	for (int y = 0; y < dungeon->height; y++) {
		for (int x = 0; x < dungeon->width; x++) {

			int tile_index = y * dungeon->height + x;
			Tile* tile = dungeon->tiles[tile_index];

			if (tile->actor != NULL) {
				SDL_Log("releasing actor");
				FreeActor(tile->actor);
			}

			FreeSprite(tile->sprite);
			free(tile);

		}
	}

	free(dungeon->tiles);

}

void GenerateDungeonTiles(Game* game) {

	Dungeon* dungeon = game->dungeon;

	for (int y = 0; y < dungeon->height; y++) {
		for (int x = 0; x < dungeon->width; x++) {

			int tile_index = y * dungeon->height + x;
			Tile* tile = dungeon->tiles[tile_index];

			int tile_type = rnd_int(TILE_TYPE_MIN + 1,  TILE_TYPE_MAX - 1);

			switch (tile_type) {
			case TILE_TYPE_SPACE:
				tile->tile_type = TILE_TYPE_SPACE;
				// tile->sprite = NULL;
				break;
			case TILE_TYPE_WALL:
				SDL_Log("tile %d is wall", tile->index);
				tile->tile_type = TILE_TYPE_WALL;
				tile->sprite = CreateSprite(game, "wall");
				break;
			default:
				break;
			}

		}
	}

}

void RenderDungeon(Game* game) {

	Dungeon* dungeon = game->dungeon;

	for (int y = 0; y < dungeon->height; y++) {
		for (int x = 0; x < dungeon->width; x++) {

			int tile_index = y * dungeon->height + x;
			Tile* tile = dungeon->tiles[tile_index];

			if (tile->actor == NULL) {
				switch (tile->tile_type) {
				case TILE_TYPE_SPACE:
					break;
				case TILE_TYPE_WALL:
					int render_x = x * tile->sprite->width * rendering_scale;
					int render_y = y * tile->sprite->width * rendering_scale;
					RenderSprite(game, render_x, render_y, tile->sprite);
					break;
				default:
					break;
				}
			} else {
				Actor* actor = tile->actor;
				int render_x = x * actor->sprite->width * rendering_scale;
				int render_y = y * actor->sprite->width * rendering_scale;
				RenderSprite(game, render_x, render_y, actor->sprite);
			}

		}
	}

}

void FreeGame(Game* game) {

	FreeDungeon(game->dungeon);
	FreeTextures(game->textures_list);

	SDL_Log("SDL_DestroyRenderer");
	SDL_DestroyRenderer(game->renderer);

}

int main(void) {

	srand(time(NULL));

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	SDL_Window* window = SDL_CreateWindow("crog v0.1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, 0);

	if (window == NULL) {
		SDL_Log("Window initialization error!\n%s\n", SDL_GetError());
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

	if (renderer == NULL) {
		SDL_Log("Renderer initialization error!\n%s\n", SDL_GetError());
		return -1;
	}

	Game game;
	game.running = 1;
	game.renderer = renderer;

	LoadTextures(&game);

	game.dungeon = CreateDungeon(10, 10);
	GenerateDungeonTiles(&game);

	SDL_Log("Loading sprites");

	Sprite* player_sprite = CreateSprite(&game, "player");
	Color player_color = {50, 250, 50, 255};
	player_sprite->color = player_color;

	Actor* player_actor = CreateActor(ACTOR_TYPE_PLAYER, player_sprite);
	DungeonPlaceActor(game.dungeon, rnd_int(0, game.dungeon->width - 1), rnd_int(0, game.dungeon->height - 1), player_actor);

	while (game.running == 1) {
		
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {

			case SDL_QUIT:
				game.running = 0;
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					game.running = 0;
					break;
				case SDLK_UP:
					break;
				case SDLK_DOWN:
					break;
				case SDLK_LEFT:
					break;
				case SDLK_RIGHT:
					break;
				default:
					break;
				}
				break;

			default:
				break;

			}
		}

		SDL_SetRenderDrawColor(game.renderer, 150, 50, 50, 255);
		SDL_RenderClear(game.renderer);

		// RenderSprite(&game, 100, 100, player_sprite);
		// RenderSprite(&game, 200, 100, player2_sprite);

		RenderDungeon(&game);

		SDL_RenderPresent(game.renderer);
	}

	FreeSprite(player_sprite);
	FreeGame(&game);

	SDL_Log("SDL_DestroyWindow");
	SDL_DestroyWindow(window);

	SDL_Log("IMG_Quit");
	IMG_Quit();

	SDL_Log("SDL_Quit");
	SDL_Quit();
	
	return 0;
}
