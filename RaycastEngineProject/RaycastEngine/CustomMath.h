#pragma once

#include <math.h>

// Macro Definitions
#define FALSE		0
#define TRUE		!(FALSE)

typedef struct
{
	double tickCurrent;
	double tickPrevious;
	double frameRate;

	Uint64 perfCurrent;
	Uint64 perfPrevious;
	double deltaTime;

} Timer;

typedef struct
{
	int x, y;
} VecI2;

typedef struct
{
	double x, y;
} Vec2;

static double degrees_to_radians(double angle)
{
	return angle * M_PI / 180.0;
} // degrees_to_radians()

const Vec2 DIR_POSITIVE = { 1, 1 };
const Vec2 DIR_NEGATIVE = { -1, -1 };

Vec2 vec2(double x, double y)
{
	Vec2 newVec = { x,y };
	return newVec;

} // vec2()

VecI2 vecI2( int x, int y )
{
	VecI2 newVec = { x,y };
	return newVec;

} // vecI2()

Vec2 Normalize(Vec2 *vec2)
{
	double magnitude = sqrt((vec2->x * vec2->x) + (vec2->y * vec2->y));
	vec2->x = vec2->x / magnitude;
	vec2->y = vec2->y / magnitude;
	return *vec2;

} // Normalize()

Vec2 GetProjectedVector(Vec2* startPos, Vec2* dir, double length)
{
	Vec2 newVec = { startPos->x - dir->x * length, startPos->y - dir->y * length };
	return newVec;

} // GetProjectedVector()

int clampI(int x, int lower, int upper)
{
	return min(upper, max(x, lower));

} // clampI()

double clamp(double x, double lower, double upper)
{
	return min(upper, max(x, lower));

} // clamp()

double square(double x)
{
	double square_of_x;
	square_of_x = x * x;
	return square_of_x;

} // square()

double Distance(Vec2 vecA, Vec2 vecB)
{
	return sqrt( square(vecB.x - vecA.x) + square(vecB.y - vecA.y));

} // Distance()