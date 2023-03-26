#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "list.h"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

int rnd_int(int lower, int upper) {
	return (rand() % (upper - lower + 1)) + lower;
}

float lerp(float v0, float v1, float t) {
	return (1 - t) * v0 + t * v1;
}

const int window_width = 1280;
const int window_height = 720;
const int rendering_scale = 8;
const int tile_size = 8 * rendering_scale;

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

typedef enum {
	ACTION_TYPE_MIN,
	ACTION_TYPE_MOVE,
	ACTION_TYPE_MAX
} ActionType;

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
	float x;
	float y;
} Vector2;

typedef struct {
	Vector2 position;
} Camera;

void CameraUpdate(Camera* camera, int target_x, int target_y) {

	float half_screen_w = window_width / 2;
	float half_screen_h = window_height / 2;

	float rel_target_x = -target_x + half_screen_w;
	float rel_target_y = -target_y + half_screen_h;

	float offset_x = lerp(camera->position.x, rel_target_x, 0.01f);
	float offset_y = lerp(camera->position.y, rel_target_y, 0.01f);

	camera->position.x = offset_x;
	camera->position.y = offset_y;

}

typedef struct {
	int x;
	int y;
} MoveActionData;

typedef struct {
	ActionType type;
	void* data;
} Action;

typedef struct Tile Tile;

typedef struct {
	ActorType type;
	Sprite* sprite;
	Tile* tile;
	Action* action;
} Actor;

struct Tile {
	int index;
	TileType type;
	Position position;
	Sprite* sprite;
	Actor* actor;
};

typedef struct {
	int width;
	int height;
	Tile** tiles;
} Dungeon;

typedef struct {
	int running;
	SDL_Renderer* renderer;
	List* textures_list;
	Camera camera;
	Dungeon* dungeon;
} Game;

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
	ListPush(game->textures_list, LoadTexture(game, "sprites/player.png", "player"));
	ListPush(game->textures_list, LoadTexture(game, "sprites/wall.png", "wall"));
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
		SDL_Log("Sprite creation: texture loading error!\n");
		return NULL;
	}

	sprite->width = sprite->texture->src_width;
	sprite->height = sprite->texture->src_height;

	sprite->color = COLOR_WHITE;

	return sprite;
}

void FreeSprite(Sprite* sprite) {
	free(sprite);
}

void RenderSprite(Game* game, int x, int y, Sprite* sprite) {

	SDL_SetTextureColorMod(sprite->texture->texture, sprite->color.r, sprite->color.g, sprite->color.b);
	SDL_SetTextureAlphaMod(sprite->texture->texture, sprite->color.a);

	SDL_Rect dstrect = {x + game->camera.position.x, y + game->camera.position.y, tile_size, tile_size};

	SDL_RenderCopy(game->renderer, sprite->texture->texture, NULL, &dstrect);

}

Actor* CreateActor(ActorType type, Sprite* sprite) {

	Actor* actor = calloc(1, sizeof(Actor));
	actor->type = type;
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
	actor->tile = tile;
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
			tile->position = (Position) {x, y};
			tile->type = TILE_TYPE_SPACE;
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
				FreeActor(tile->actor);
			}

			FreeSprite(tile->sprite);
			free(tile);

		}
	}

	SDL_Log("tiles end");

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
				tile->type = TILE_TYPE_SPACE;
				tile->sprite = NULL;
				break;
			case TILE_TYPE_WALL:
				tile->type = TILE_TYPE_WALL;
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

			int render_x = x * tile_size;
			int render_y = y * tile_size;

			if (tile->actor == NULL) {
				switch (tile->type) {
				case TILE_TYPE_SPACE:
					break;
				case TILE_TYPE_WALL:
					RenderSprite(game, render_x, render_y, tile->sprite);
					break;
				default:
					break;
				}
			} else {
				Actor* actor = tile->actor;
				RenderSprite(game, render_x, render_y, actor->sprite);
			}

		}
	}

}

Action* CreateMoveAction(int x, int y) {
	
	Action* action = malloc(sizeof(Action));
	action->type = ACTION_TYPE_MOVE;

	MoveActionData* data = malloc(sizeof(MoveActionData));
	data->x = x;
	data->y = y;

	action->data = data;

	return action;

}

void ActorProcessAction(Game* game, Actor* actor) {

	if (actor->action == NULL) return;

	Action* action = actor->action;

	switch (action->type) {
	case ACTION_TYPE_MOVE:

		MoveActionData* data = (MoveActionData*) action->data;

		SDL_Log("Actor move by x=%d, y=%d", data->x, data->y);

		Tile* tile = actor->tile;

		int new_x = tile->position.x + data->x;
		int new_y = tile->position.y + data->y;

		if (new_x < 0 || new_x >= game->dungeon->width) break;
		if (new_y < 0 || new_y >= game->dungeon->height) break;

		int destination_tile_index = new_y * game->dungeon->height + new_x;
		Tile* destination_tile = game->dungeon->tiles[destination_tile_index];

		if (destination_tile->type == TILE_TYPE_WALL) break;

		tile->actor = NULL;
		destination_tile->actor = actor;
		actor->tile = destination_tile;
		
		SDL_Log("Actor prev tile x=%d, y=%d", tile->position.x, tile->position.y);
		SDL_Log("Actor new tile x=%d, y=%d", destination_tile->position.x, destination_tile->position.y);

		break;
	default:
		break;
	}

	free(action->data);
	free(action);
	actor->action = NULL;

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

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED); // SDL_RENDERER_PRESENTVSYNC

	if (renderer == NULL) {
		SDL_Log("Renderer initialization error!\n%s\n", SDL_GetError());
		return -1;
	}

	Game game;
	game.running = 1;
	game.renderer = renderer;

	LoadTextures(&game);

	Camera camera;
	camera.position = (Vector2) {};
	game.camera = camera;
	SDL_Log("Camera initialized x=%d, y=%d", camera.position.x, camera.position.y);

	game.dungeon = CreateDungeon(10, 10);
	GenerateDungeonTiles(&game);

	SDL_Log("Loading sprites");

	Sprite* player_sprite = CreateSprite(&game, "player");
	Color player_color = {50, 250, 50, 255};
	player_sprite->color = player_color;

	Actor* player_actor = CreateActor(ACTOR_TYPE_PLAYER, player_sprite);
	DungeonPlaceActor(game.dungeon, rnd_int(0, game.dungeon->width - 1), rnd_int(0, game.dungeon->height - 1), player_actor);

	Uint32 prev_time = SDL_GetTicks(); // delta time calculation

	while (game.running == 1) {

		Uint32 cur_time = SDL_GetTicks();
		float delta_time = (cur_time - prev_time) / 1000.0f;
		prev_time = cur_time;

		SDL_Log("delta time: %f", delta_time);
		SDL_Log("camera %f, %f", game.camera.position.x, game.camera.position.y);

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
				case SDLK_KP_8:
					player_actor->action = CreateMoveAction(0, -1);
					break;
				case SDLK_DOWN:
				case SDLK_KP_2:
					player_actor->action = CreateMoveAction(0, 1);
					break;
				case SDLK_LEFT:
				case SDLK_KP_4:
					player_actor->action = CreateMoveAction(-1, 0);
					break;
				case SDLK_RIGHT:
				case SDLK_KP_6:
					player_actor->action = CreateMoveAction(1, 0);
					break;
				case SDLK_KP_7:
					player_actor->action = CreateMoveAction(-1, -1);
					break;
				case SDLK_KP_9:
					player_actor->action = CreateMoveAction(1, -1);
					break;
				case SDLK_KP_1:
					player_actor->action = CreateMoveAction(-1, 1);
					break;
				case SDLK_KP_3:
					player_actor->action = CreateMoveAction(1, 1);
					break;
				case SDL_SCANCODE_PERIOD:
				case SDLK_KP_5:
					player_actor->action = CreateMoveAction(0, 0);
					break;
				default:
					break;
				}
				break;

			default:
				break;

			}
		}

		ActorProcessAction(&game, player_actor);

		int camera_target_x = player_actor->tile->position.x * tile_size + tile_size / 2;
		int camera_target_y = player_actor->tile->position.y * tile_size + tile_size / 2;
		CameraUpdate(&game.camera, camera_target_x, camera_target_y);

		SDL_SetRenderDrawColor(game.renderer, 150, 50, 50, 255);
		SDL_RenderClear(game.renderer);

		RenderDungeon(&game);

		SDL_RenderPresent(game.renderer);

	}

	FreeGame(&game);

	SDL_Log("SDL_DestroyWindow");
	SDL_DestroyWindow(window);

	SDL_Log("IMG_Quit");
	IMG_Quit();

	SDL_Log("SDL_Quit");
	SDL_Quit();
	
	return 0;
}
