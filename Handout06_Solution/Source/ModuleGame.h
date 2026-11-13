#pragma once

#include "Globals.h"
#include "Module.h"

#include "p2Point.h"

#include "raylib.h"
#include "Timer.h"
#include <vector>
#include <utility>

class PhysBody;
class PhysicEntity;
class Ball;


class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();

	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

	void DestroyEntity(PhysicEntity* entity);
	void DestroyBall(Ball* ball);

	// NULL if pbody is not a ball
	Ball* FindBall(PhysBody* pbody);
	bool IsMerging(Ball* ball);
	void DestroyAllEntities();

public:

	std::vector<PhysicEntity*> entities;

	Texture2D circle;
	Texture2D box;
	Texture2D rick;

	uint32 bonus_fx;

	vec2<int> ray;
	bool ray_on;

	// Sensor at the bottom of the screen
	PhysBody* sensor;
	std::vector<PhysBody*> bodies_to_destroy;

	// Top view balls
	std::vector<Ball*> balls;
	Timer spawn_timer;

	// Pairs of balls that touched: merged in Update()
	std::vector<std::pair<Ball*, Ball*>> balls_to_merge;
};
