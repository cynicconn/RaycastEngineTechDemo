#pragma once 

#include <time.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include "Player.h"

#define BENCHMARK			0

// Environment Values
#define	RAY_LENGTH			100
#define	COLUMN_RATIO		1
#define	WALL_SCALE			0.4f
#define	DARKNESS_INTENSITY	15
#define	REPEAT_WALL			2

// Stores Texture and cached Texture info 
typedef struct
{
	SDL_Texture* img;
	int			 width;
	int			 height; 

} Image;

// Holds flags and dev values
typedef struct
{
	int isFullscreen;
	int texturedWalls;
	int displayMap;
	int drawRays;
	int enabledLighting;

} Debug;

// Contains all major elements for running the game
typedef struct
{
	Player			player;

	// Game Textures
	Image			img_Checker;
	Image			img_Wall;
	Image			img_Floor;
	Image			img_Hand;

	SDL_Window*		window;
	SDL_Renderer*	renderer;

	Timer			timer;
	GameMap			gameMap;
	Debug			debug;

	Uint8*			statePrev;

} GameState;

void EnableFullscreen( SDL_Window* window, Debug* debug )
{
	if ( debug->isFullscreen != SDL_WINDOW_FULLSCREEN)
	{
		debug->isFullscreen = SDL_WINDOW_FULLSCREEN;
		SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN);
	}

} // EnableFullscreen()

void DisableFullscreen( SDL_Window* window, Debug* debug )
{
	if ( debug->isFullscreen != 0 )
	{
		debug->isFullscreen = 0;
		SDL_SetWindowFullscreen( window, 0);
	}

} // DisableFullscreen()

void ProcessDebugInput( const Uint8* state, Uint8* statePrev, SDL_Window* window, Debug* debug )
{
	// Toggle Debug Displays
	if ( state[SDL_SCANCODE_1] )
	{
		debug->texturedWalls = TRUE;
	}
	if (state[SDL_SCANCODE_2])
	{
		debug->texturedWalls = FALSE;
	}
	if (state[SDL_SCANCODE_3])
	{
		debug->enabledLighting = FALSE;
	}
	if (state[SDL_SCANCODE_4])
	{
		debug->enabledLighting = TRUE;
	}
	if (state[SDL_SCANCODE_9])
	{
		debug->drawRays = FALSE;
	}
	if (state[SDL_SCANCODE_0])
	{
		debug->drawRays = TRUE;
	}
	if (state[SDL_SCANCODE_M])
	{
		debug->displayMap	= TRUE;
		debug->drawRays		= TRUE;
	}
	if (state[SDL_SCANCODE_N])
	{
		debug->displayMap	= FALSE;
		debug->drawRays		= FALSE;
	}
	if (state[SDL_SCANCODE_F3])
	{
		DisableFullscreen( window, debug );
	}
	if (state[SDL_SCANCODE_F4])
	{
		EnableFullscreen(window, debug);
	}

} // ProcessDebugInput()

int ProcessWindowEvents(SDL_Window* window)
{
	int done = FALSE;
	SDL_Event event;

	// Check for events
	while ( SDL_PollEvent(&event) )
	{
		switch (event.type)
		{
		case SDL_WINDOWEVENT_CLOSE:
		{
			if (window)
			{
				SDL_DestroyWindow(window);
				window	= NULL;
				done	= TRUE;
			}
		}
		break;
		case SDL_KEYDOWN:
		{
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				done = TRUE;
				break;
			}
		}
		break;
		case SDL_QUIT:
			done = TRUE;
			break;
		}
	}
	return done;

} // ProcessWindowEvents()

int ProcessInputsAndEvents( GameState *game )
{
	int done = ProcessWindowEvents(game->window);

	const Uint8* state = SDL_GetKeyboardState(NULL);
	ProcessDebugInput(state, game->statePrev, game->window, &game->debug );
	ProcessPlayerInput( state, &game->player, &game->gameMap, game->timer.deltaTime );

	return done;

} // ProcessInputsAndEvents()

// Debug Displaying 2D Map
void DrawMap(GameState *game)
{
	for (int i = 0; i < MAP_SIZE; i++) // Rows
	{
		for (int j = 0; j < MAP_SIZE; j++) // Columns
		{
			if ( game->gameMap.map[i][j] == FALSE)
			{
				continue;
			}
			SDL_Rect tileRect = { j * GRID_RES.x, i * GRID_RES.y, GRID_RES.x, GRID_RES.y };
			SDL_RenderCopy(game->renderer, game->img_Checker.img, NULL, &tileRect);
		} // for

	} // for

} // DrawMap()

  // Draw Player on 2D Map ( including camera facing )
void DebugDrawPlayer(GameState *game)
{
	// Debug Draw Player Dir
	const float DIR_LENGTH = RESOLUTION.x / 10.0f;
	Vec2 playerPos = { game->player.pos.x, game->player.pos.y };
	Vec2 endPos = GetProjectedVector(&playerPos, &game->player.direction, -DIR_LENGTH);
	SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(game->renderer, (int)playerPos.x, (int)playerPos.y, (int)endPos.x, (int)endPos.y);

	// Debug Draw Camera Plane
	Vec2 camStart = GetProjectedVector(&endPos, &game->player.cameraPlane, -DIR_LENGTH);
	Vec2 camEnd = GetProjectedVector(&endPos, &game->player.cameraPlane, DIR_LENGTH);
	SDL_SetRenderDrawColor(game->renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(game->renderer, (int)camStart.x, (int)camStart.y, (int)camEnd.x, (int)camEnd.y);

	// Draw Player Pos
	SDL_SetRenderDrawColor(game->renderer, 0, 255, 0, 255);
	double sizeHalf = game->player.debugSize / 2.0f;
	SDL_Rect rect = { (int)(game->player.pos.x - sizeHalf), (int)(game->player.pos.y - sizeHalf), game->player.debugSize, game->player.debugSize };
	SDL_RenderFillRect(game->renderer, &rect);

} // DebugDrawPlayerDir()

Hit Raycast(GameState *game, int column)
{
	// Initialize Hit ( Default )
	Hit hit;
	memset(&hit, 0, sizeof(hit) );

	// Calculate Ray Position and Direction
	const double COLUMN_TOTAL  = RESOLUTION.x * game->gameMap.columnRatio;
	const double CAMERA_COORD  = (2 * ( column / COLUMN_TOTAL ) ) - 1; //x-coordinate in camera space

	Vec2 rayDir			= GetProjectedVector(&game->player.direction, &game->player.cameraPlane, -CAMERA_COORD );
	Vec2 rayEndPos		= { game->player.pos.x, game->player.pos.y }; // Current End Point of Ray that increments and tests for Hit

	//which box of the map we're in
	Vec2 mapPos = { game->player.pos.x / GRID_RES.x, game->player.pos.y / GRID_RES.y };

	//length of ray from current position to next x or y-side
	Vec2 sideDist;

	//length of ray from one x or y-side to next x or y-side
	Vec2 rayDirSquared		= { square(rayDir.x), square(rayDir.y) };
	Vec2 deltaDist			= { sqrt(1 + rayDirSquared.y / rayDirSquared.x ), sqrt(1 + rayDirSquared.x / rayDirSquared.y ) };
	double perpWallDist;

	//what direction to step in x or y-direction (either +1 or -1)
	VecI2 mapStep = { 0, 0 };

	// Truncate Map Pos
	int mapX = (int)mapPos.x;
	int mapY = (int)mapPos.y;

	//calculate step and initial sideDist
	if (rayDir.x < 0)
	{
		mapStep.x	= -1;
		sideDist.x	= (mapPos.x - mapX ) * deltaDist.x;
	}
	else
	{
		mapStep.x	= 1;
		sideDist.x	= (mapX + 1.0 - mapPos.x) * deltaDist.x;
	}
	if (rayDir.y < 0)
	{
		mapStep.y	= -1;
		sideDist.y	= (mapPos.y - mapY) * deltaDist.y;
	}
	else
	{
		mapStep.y	= 1;
		sideDist.y	= (mapY + 1.0 - mapPos.y) * deltaDist.y;
	}

	//perform DDA
	while (hit.isHit == FALSE)
	{
		//jump to next map square, OR in x-direction, OR in y-direction
		if ( sideDist.x < sideDist.y )
		{
			sideDist.x	+= deltaDist.x;
			mapX		+= mapStep.x;
			rayEndPos.x += mapStep.x * GRID_RES.x;
			hit.isSide	 = TRUE;
		}
		else
		{
			sideDist.y	+= deltaDist.y;
			mapY		+= mapStep.y;
			rayEndPos.y += mapStep.y * GRID_RES.y;
			hit.isSide  = FALSE;
		}

		int xOut = (mapX < 0 || mapX > MAP_SIZE - 1);
		int yOut = (mapY < 0 || mapY > MAP_SIZE - 1);
		if (xOut || yOut)
		{
			return hit;
		}

		//Check if ray has hit a wall
		if (game->gameMap.map[mapY][mapX] > 0)
		{
			hit.isHit = TRUE;
		}

	} // while

	//Calculate distance projected on camera direction ( otherwise oblique distance will give fisheye effect!)
	if (hit.isSide == TRUE)
	{
		perpWallDist = (mapX - mapPos.x + (1 - mapStep.x) / 2) / rayDir.x;
	}
	else
	{
		perpWallDist = (mapY - mapPos.y + (1 - mapStep.y) / 2) / rayDir.y;
	}

	// Populate with Hit Data
	hit.x		= (int)game->player.pos.x;
	hit.y		= (int)game->player.pos.y;
	hit.end.x	= (int)rayEndPos.x;
	hit.end.y	= (int)rayEndPos.y;
	hit.point.x = mapX;
	hit.point.y = mapY;
	hit.dist	= perpWallDist;

	// Calculate where was wall hit on the X Axis
	double wallX; //where exactly the wall was hit
	if (hit.isSide == TRUE)
	{
		wallX = mapPos.y + perpWallDist * rayDir.y;
	}
	else
	{
		wallX = mapPos.x + perpWallDist * rayDir.x;
	}
	wallX -= floor(wallX);

	// Extend UV Coordinates over N map tiles
	const int TEX_WIDTH		= game->img_Wall.width;
	const int SPREAD		= game->gameMap.repeatWall;
	const double oneOver	= ( 1.0f / SPREAD );
	double offset			= 0;

	if (hit.isSide == TRUE )
	{
		int between		= mapY - ( mapY - (mapY % SPREAD) );
		offset			= (between / (double)SPREAD);
	}
	if (hit.isSide == FALSE )
	{
		int between		= mapX - (mapX - (mapX % SPREAD));
		offset			= (between / (double)SPREAD);
	}
	wallX *= oneOver;
	wallX += offset;
	
	// X Coordinate on the Texture based on where wall was hit
	int texX = (int)( wallX * TEX_WIDTH );
	if (hit.isSide == TRUE && rayDir.x > 0)
	{
		texX = TEX_WIDTH - texX - 1;
	}
	if (hit.isSide == FALSE && rayDir.y < 0)
	{
		texX = TEX_WIDTH - texX - 1;
	}
	hit.texX = texX;

	return hit;

} // Raycast()

void DrawHand( GameState* game )
{
	const Vec2 FRAME_PADDING	= { 0.9f, 1.075f }; // for making sure hand is not completly in bottom right corner
	Anim* handAnim				= &game->player.handAnim;
	const double BOB_STRENGTH	= ( game->player.isMoving == FALSE ) ? 0 : handAnim->strength;
	double timeRate				= game->timer.tickCurrent * handAnim->rate;
	double upAnim				= sin(timeRate) * BOB_STRENGTH;
	int handSize				= RESOLUTION.x / 2.5f;
	double handOffsetX			= (RESOLUTION.x * FRAME_PADDING.x) - handSize;
	double handOffsetY			= (RESOLUTION.y * FRAME_PADDING.y) - handSize - upAnim;

	// Render Hand Overlay
	SDL_Rect handRect = { (int)handOffsetX, (int)handOffsetY, handSize, handSize };
	SDL_RenderCopy(game->renderer, game->img_Hand.img, NULL, &handRect );

} // DrawHand()

// Draw one Column to represent world based on Raycast Hit result
void RenderColumn(GameState *game, Hit *hit, int i )
{
	// Calculate Column Height based on distance and resolution
	int columnHeight		= (int)( RESOLUTION.y / game->gameMap.wallScale / hit->dist );
	int horizonLine			= (RESOLUTION.y / 2) - (columnHeight / 2);
	SDL_Rect wallRect		= { i * (int)game->gameMap.columnRatio, horizonLine, (int)game->gameMap.columnRatio, columnHeight };

	// Calculate horizontal UV value ( V is always spans all of texture )
	const int TEX_SIZE		= game->img_Wall.width - 1;
	int texU				= (int) ( hit->texX * 0.6667f ); // multiply UV layout to account for perspective distortion
	texU					= clampI(texU, 1, TEX_SIZE);

	// Create Wall Column
	if (game->debug.texturedWalls)
	{
		SDL_Rect uvRect = { texU, 0, texU, TEX_SIZE };
		SDL_RenderCopy(game->renderer, game->img_Wall.img, &uvRect, &wallRect);
	}
	else
	{
		// Render Solid Color
		SDL_SetRenderDrawColor(game->renderer, 0, 255, 0, 255);
		SDL_RenderFillRect(game->renderer, &wallRect);
	}

	// Simple Shadowing
	if (game->debug.enabledLighting)
	{
		// Darken the Columns
		double darkness		 = (hit->isSide == FALSE) ? hit->dist * (game->gameMap.darknessIntensity/2.0f) : hit->dist * game->gameMap.darknessIntensity;
		darkness			+= (hit->isSide == FALSE) ? 128 : 0; // side walls are automatically darker
		Uint8 shadowAlpha	 = clampI( (int)darkness, 0, 255 );

		// Draw Transparent Rect over Column for "shading"
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, shadowAlpha);
		SDL_RenderFillRect(game->renderer, &wallRect);
	}

} // RenderColumn()

// Render the Game World in 2.5D 
void DrawWorld(GameState *game)
{
	if ( !game->debug.displayMap )
	{
		// Draw Ceiling
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 05, 255); // Near Black
		SDL_Rect wallRect = { 0, 0, RESOLUTION.x, RESOLUTION.y }; // Top Half of Screen
		SDL_RenderFillRect(game->renderer, &wallRect);

		// Draw Floor
		SDL_Rect floorRect = { 0, RESOLUTION.y / 2, RESOLUTION.x, RESOLUTION.y / 2 }; // Bottom Half of screen
		SDL_RenderCopy(game->renderer, game->img_Floor.img, NULL, &floorRect);
	}

	// Raycast and draw the World
	const int COLUMN_COUNT = (int)RESOLUTION.x * (int)COLUMN_RATIO;
	int i = 0;
	for (i = 0; i < COLUMN_COUNT; i++)
	{
		Hit hit = Raycast(game, i);
		if (hit.isHit == TRUE)
		{
			if (game->debug.drawRays) // Debug Draw Rays
			{
				SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
				SDL_RenderDrawLine(game->renderer, hit.x, hit.y, hit.end.x, hit.end.y );

				// Draw Hit Point Rects TODO: put in own function
				SDL_Rect hitRect = { hit.point.x * GRID_RES.x, hit.point.y * GRID_RES.y, GRID_RES.x, GRID_RES.y };
				if (hit.isSide == TRUE)
				{
					SDL_SetRenderDrawColor(game->renderer, 255, 255, 128, SDL_ALPHA_OPAQUE);
				}
				else
				{
					SDL_SetRenderDrawColor(game->renderer, 128, 255, 255, SDL_ALPHA_OPAQUE);
				}
				SDL_RenderFillRect(game->renderer, &hitRect);

				if (game->debug.displayMap)
				{
					continue;
				}
			}
			RenderColumn(game, &hit, i);
		}

	} // for

} // DrawWorld()

void DoRender( GameState *game )
{
	// Clear Screen
	SDL_SetRenderDrawColor( game->renderer, 0, 0, 255, 255);
	SDL_RenderClear(game->renderer);

	if ( game->debug.displayMap ) // / Debug Draw Topdown 2D Map
	{ 	
		DrawMap(game);
		DebugDrawPlayer(game);
		if (game->debug.drawRays)
		{
			DrawWorld(game);
		}
	}
	else // Render 2.5D World
	{
		DrawWorld(game);
		DrawHand(game);
	}

	// Flag Ready to Render
	SDL_RenderPresent(game->renderer);

} // DoRender()

void LoadImage( SDL_Renderer* renderer, const char* imgPath, Image* imageTo )
{
	SDL_Surface* imgSurface = NULL;
	imgSurface = IMG_Load(imgPath);
	if ( imgSurface == NULL)
	{
		printf("Cannot find: %s \n\n", imgPath );
		SDL_Quit();
		exit(1);
	}

	imageTo->img = SDL_CreateTextureFromSurface( renderer, imgSurface );
	SDL_FreeSurface( imgSurface );
	SDL_QueryTexture( imageTo->img, NULL, NULL, &imageTo->width, &imageTo->height );

} // LoadImage()

void GetResources( GameState* game )
{
	LoadImage(game->renderer, CHECKER_PATH,		&game->img_Checker );
	LoadImage(game->renderer, WALL_PATH,		&game->img_Wall );
	LoadImage(game->renderer, FLOOR_PATH,		&game->img_Floor );
	LoadImage(game->renderer, HAND_PATH,		&game->img_Hand );

} // GetResources()

void LoadGame( GameState* game )
{
	// Seed the pseudo-random number generator
	srand((int)time(NULL)); 

	// Load all textures etc needed for game
	GetResources( game );

	// Player Setup
	InitializePlayer( &game->player );

	// Setup GameMap
	InitializeMap(&game->gameMap, RAY_LENGTH, COLUMN_RATIO, WALL_SCALE, DARKNESS_INTENSITY, REPEAT_WALL );

} // LoadGame()

// Write out Debug Benchmark Info to File
void DebugOutput(const char* outputText)
{
	// Debug Output To File Setup
	SDL_RWops* debugFile = SDL_RWFromFile(DEBUG_PATH, "a+");
	if (debugFile == NULL)
	{
		printf("Warning: Unable to open file! SDL Error: %s\n", SDL_GetError());
		printf("New file created!\n");
		debugFile = SDL_RWFromFile(DEBUG_PATH, "+");
	}
	if (debugFile != NULL)
	{
		printf("Writing to File! \n");
		size_t len = SDL_strlen(outputText);
		SDL_RWwrite(debugFile, outputText, 1, len);

		//Close file handler 
		SDL_RWclose(debugFile);
	}

} // DebugOutput()

void UpdateTime( Timer* timer )
{
	// Delta Time
	timer->perfPrevious		= timer->perfCurrent;
	timer->perfCurrent		= SDL_GetPerformanceCounter();
	timer->deltaTime		= (timer->perfCurrent - timer->perfPrevious) / (double)SDL_GetPerformanceFrequency();

	// Frame Rate
	timer->tickPrevious		= timer->tickCurrent;
	timer->tickCurrent		= SDL_GetTicks();
	timer->frameRate		= 1000.0f / (timer->tickCurrent - timer->tickPrevious );

	printf("DT: %f \n", timer->deltaTime);
	printf("FPS: %d \n", (int)timer->frameRate);

} // UpdateTime()

int Benchmark()
{
#if BENCHMARK
	double timeInSeconds	= (int)clock() / CLOCKS_PER_SEC;

	if (timeInSeconds >= 5 )
	{
		char* output = malloc(16);
		snprintf(output, 16, "\n Timing: %d", SDL_GetTicks() );
		DebugOutput(output);
		return TRUE;
	}
	return FALSE;
#else
	return FALSE;
#endif

} // Benchmark()


int ExitGame( GameState* game )
{
	// Deallocate Resources
	SDL_DestroyTexture(game->img_Checker.img);
	SDL_DestroyTexture(game->img_Wall.img);
	SDL_DestroyTexture(game->img_Floor.img);
	SDL_DestroyTexture(game->img_Hand.img);

	SDL_DestroyWindow(game->window);
	SDL_DestroyRenderer(game->renderer);
	SDL_Quit();

	return 0;

} // ExitGame()

void SetupGameState( GameState *game )
{
	memset( game, 0, sizeof *game );
	game->window			= NULL;
	game->renderer			= NULL;
	game->debug				= (Debug) { FALSE, TRUE, FALSE, FALSE, TRUE };

} // SetupGameState()

int main(int argc, char *argv[])
{
	GameState game;
	SetupGameState(&game);

	// Setup Video
	SDL_Init(SDL_INIT_VIDEO);
	game.window = SDL_CreateWindow("RaycastEngine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RESOLUTION.x, RESOLUTION.y, 0);
	game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND);
	SDL_ShowCursor(SDL_DISABLE);

	LoadGame(&game);

	// Main Game Loop
	int done = 0;
	while ( !done )
	{
		UpdateTime(&game.timer);
		done = ProcessInputsAndEvents( &game );
		DoRender( &game );
		done += Benchmark();
	} // while

	// Exit Game, Unload All Memory
	return ExitGame( &game );

} // main()