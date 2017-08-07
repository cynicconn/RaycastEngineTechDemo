#pragma once

#include "CustomMath.h"
#include "Map.h"

typedef struct
{
	int		isHit;
	int		isSide;
	double	dist;
	double	texX;
	VecI2	point;
	VecI2	end;
	int		x, y;
	double	angle;

} Hit;

typedef struct
{
	double rate;
	double strength;

} Anim;

typedef struct
{
	double	speed;
	int		isMoving;
	double	turnRate;
	Vec2	pos;
	Vec2	direction;
	Vec2	cameraPlane;
	Vec2	size;
	int		debugSize;
	float	animStrength;
	Anim	handAnim;

} Player;

void InitializePlayer( Player* player )
{
	player->pos.x = RESOLUTION.x  * 0.8f;
	player->pos.y = RESOLUTION.y * 0.3f;
	player->direction.x = -1;
	player->direction.y = 0;
	player->cameraPlane.x = 0 * FOCAL_LENGTH;
	player->cameraPlane.y = 1 * FOCAL_LENGTH;
	player->speed = 200.0f;
	player->turnRate = 2.0f;
	player->size.x = 2;
	player->size.y = 2;
	player->debugSize = 16;
	player->isMoving = FALSE;
	player->animStrength = 20;

	Anim povHandAnim = { 0.01f, 20 };
	player->handAnim = povHandAnim;

} // InitializePlayer()

// Returns true if Ray has "hit" a wall
Hit Inspect(GameMap map, double posX, double posY, Hit *hit)
{
	int gridResX = (int)RESOLUTION.x / MAP_SIZE;
	int gridResY = (int)RESOLUTION.y / MAP_SIZE;
	int cellX = (int)posX / gridResX;
	int cellY = (int)posY / gridResY;

	if (cellX < 0 || cellX > MAP_SIZE - 1)
	{
		return *hit;
	}
	if (cellY < 0 || cellY > MAP_SIZE - 1)
	{
		return *hit;
	}

	hit->isHit = map.map[cellY][cellX]; // if wall, or not
	if ( hit->isHit )
	{
		hit->point.x = cellX;
		hit->point.y = cellY;
	}

	return *hit;

} // Inspect()

void MovePlayer( GameMap* map, Player* player, Vec2 dir, Vec2 newPos, double deltaTime )
{
	Hit temp;
	Inspect( *map, player->pos.x + (dir.x * player->size.x * newPos.x * deltaTime), player->pos.y + (dir.y * player->size.y * newPos.y * deltaTime), &temp);
	if (temp.isHit == FALSE)
	{
		player->pos.x += dir.x * newPos.x * deltaTime;
		player->pos.y += dir.y * newPos.y * deltaTime;
		player->isMoving = TRUE;
	}

} // MovePlayer()

Vec2 UpdateCameraPlane( Vec2* cam, double sinA, double cosA )
{
	Vec2 newCamPlane;
	newCamPlane.x = (cam->x * cosA) - (cam->y * sinA);
	newCamPlane.y = (cam->y * cosA) + (cam->x * sinA);
	newCamPlane = Normalize( &newCamPlane );
	newCamPlane.x *= FOCAL_LENGTH;
	newCamPlane.y *= FOCAL_LENGTH;

	return newCamPlane;

} // UpdateCameraPlane()

void RotatePlayer(Player* player, double angle, double deltaTime )
{
	double rotAngle = angle * deltaTime;
	double x = player->direction.x;
	double y = player->direction.y;
	double cosA = cos(rotAngle);
	double sinA = sin(rotAngle);

	player->direction.x = (x * cosA) - (y * sinA);
	player->direction.y = (y * cosA) + (x * sinA);
	player->direction = Normalize(&player->direction);

	player->cameraPlane = UpdateCameraPlane( &player->cameraPlane, sinA, cosA );

} // RotatePlayer()

Vec2 CalculatePlayerVelocity( double speed, Vec2* dir )
{
	Vec2 newVec = { speed * dir->x, speed * dir->y };
	return newVec;

} // CalculatePlayerVelocity()

void ProcessPlayerInput( const Uint8* state, Player* player, GameMap* gameMap, double deltaTime )
{
	// Movement
	player->isMoving		= FALSE; // Always need to reflag every frame
	Vec2 forwardDir			= CalculatePlayerVelocity( player->speed, &player->direction );
	Vec2 lateralDir			= CalculatePlayerVelocity( player->speed, &player->cameraPlane );

	// Move
	if (state[SDL_SCANCODE_W] )
	{
		MovePlayer( gameMap, player, DIR_POSITIVE, forwardDir, deltaTime );
	}
	if (state[SDL_SCANCODE_S] )
	{
		MovePlayer( gameMap, player, DIR_NEGATIVE, forwardDir, deltaTime );
	}

	// Strafe
	if (state[SDL_SCANCODE_A])
	{
		MovePlayer( gameMap, player, DIR_NEGATIVE, lateralDir, deltaTime );
	}
	if (state[SDL_SCANCODE_D])
	{
		MovePlayer( gameMap, player, DIR_POSITIVE, lateralDir, deltaTime );
	}

	// Rotate
	if (state[SDL_SCANCODE_LEFT])
	{
		RotatePlayer(player, player->turnRate, deltaTime );
	}
	if (state[SDL_SCANCODE_RIGHT])
	{
		RotatePlayer(player, -player->turnRate, deltaTime );
	}

} // ProcessPlayerInput()
