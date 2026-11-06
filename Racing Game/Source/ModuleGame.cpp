#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <algorithm>

// Base class for everything in the scene that has a physics body
class PhysicEntity
{
protected:

	PhysicEntity(PhysBody* _body)
		: body(_body)
	{

	}

public:
	virtual ~PhysicEntity() = default;

	// Draw the entity at the position of its body
	virtual void Update() = 0;

public:
	PhysBody* body;
};

class Circle : public PhysicEntity
{
public:
	Circle(ModulePhysics* physics, int _x, int _y, Texture2D _texture)
		: PhysicEntity(physics->CreateCircle(_x, _y, 25))
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		// Red when the mouse is inside the body
		Color tint = body->Contains(GetMouseX(), GetMouseY()) ? RED : WHITE;

		// Box2D positions are the center of the body: draw the texture around its center
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
		Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, tint);
	}

private:
	Texture2D texture;
};

class Box : public PhysicEntity
{
public:
	Box(ModulePhysics* physics, int _x, int _y, Texture2D _texture)
		: PhysicEntity(physics->CreateRectangle(_x, _y, 50, 25))
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		Color tint = body->Contains(GetMouseX(), GetMouseY()) ? RED : WHITE;

		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)texture.width, (float)texture.height };
		Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, tint);
	}

private:
	Texture2D texture;
};

// Top view ball: random mass, pushed to the right, rubs against the floor
class Ball : public PhysicEntity
{
public:
	// Same push for every ball: a = F / m, heavier balls accelerate less
	static constexpr float push_force = 100.0f; // N, to the right

	// Seconds before the ball is destroyed
	static constexpr double lifetime = 10.0;

	Ball(ModulePhysics* physics, int _x, int _y, float _mass, Texture2D _texture)
		: PhysicEntity(physics->CreateCircle(_x, _y, Radius(_mass), Density(_mass)))
		, mass(_mass)
		, texture(_texture)
	{

	}

	// TODO 3: Heavier balls are bigger
	// Same material: area grows with mass, so radius grows with sqrt(mass)
	// 1 kg -> 10 px, 100 kg -> 100 px
	static int Radius(float mass)
	{
		return (int)(10.0f * sqrtf(mass));
	}

	// mass = density * area, area in m^2
	static float Density(float mass)
	{
		float radius = PIXEL_TO_METERS(Radius(mass));
		return mass / (b2_pi * radius * radius);
	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		int radius = Radius(mass);

		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)radius * 2.0f, (float)radius * 2.0f };
		Vector2 origin = { (float)radius, (float)radius };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, WHITE);

		const char* text = TextFormat("%d kg", (int)mass);
		DrawText(text, x - MeasureText(text, 20) / 2, y - 10, 20, BLACK);
	}

public:
	float mass;
	Timer life;

private:
	Texture2D texture;
};

class Chain : public PhysicEntity
{
public:
	static constexpr int points[24] = {
		-38, 80,
		-44, -54,
		-16, -60,
		-16, -17,
		19, -19,
		19, -79,
		61, -77,
		57, 73,
		17, 78,
		20, 16,
		-25, 13,
		-9, 72
	};

	Chain(ModulePhysics* physics, int _x, int _y)
		: PhysicEntity(physics->CreateChain(_x, _y, points, 24))
	{

	}

	void Update() override
	{
		// No texture: press F1 to see it
	}
};

class Rick : public PhysicEntity
{
public:
	// Pivot 0, 0
	static constexpr int rick_head[64] = {
		14, 36,
		42, 40,
		40, 0,
		75, 30,
		88, 4,
		94, 39,
		111, 36,
		104, 58,
		107, 62,
		117, 67,
		109, 73,
		110, 85,
		106, 91,
		109, 99,
		103, 104,
		100, 115,
		106, 121,
		103, 125,
		98, 126,
		95, 137,
		83, 147,
		67, 147,
		53, 140,
		46, 132,
		34, 136,
		38, 126,
		23, 123,
		30, 114,
		10, 102,
		29, 90,
		0, 75,
		30, 62
	};

	Rick(ModulePhysics* physics, int _x, int _y, Texture2D _texture)
		: PhysicEntity(physics->CreateChain(_x, _y, rick_head, 64))
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		// Chains have no volume: Contains() is never true for them
		Color tint = body->Contains(GetMouseX(), GetMouseY()) ? RED : WHITE;

		DrawTextureEx(texture, Vector2{ (float)x, (float)y }, body->GetRotation() * RAD2DEG, 1.0f, tint);
	}

private:
	Texture2D texture;
};

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	ray_on = false;
	ray.x = ray.y = 0;
	sensor = NULL;
}

ModuleGame::~ModuleGame()
{}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;

	App->renderer->camera.x = App->renderer->camera.y = 0;

	circle = LoadTexture("Assets/wheel.png");
	box = LoadTexture("Assets/crate.png");
	rick = LoadTexture("Assets/rick_head.png");

	bonus_fx = App->audio->LoadFx("Assets/bonus.wav");

	// Sensor at the bottom of the screen
	sensor = App->physics->CreateRectangleSensor(SCREEN_WIDTH / 2, SCREEN_HEIGHT, SCREEN_WIDTH / 2, 25);
	sensor->listener = this;

	spawn_timer.Start();

	return ret;
}

// Unload assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");

	DestroyAllEntities();

	App->physics->DestroyBody(sensor);
	sensor = NULL;

	UnloadTexture(circle);
	UnloadTexture(box);
	UnloadTexture(rick);

	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	// Destroy the bodies that touched the sensor
	for (PhysBody* body : bodies_to_destroy)
	{
		for (PhysicEntity* entity : entities)
		{
			if (entity->body == body)
			{
				DestroyEntity(entity);
				break;
			}
		}
	}
	bodies_to_destroy.clear();

	// Top view balls ------------------------------------------------------

	// TODO 1: Every second, create a ball at the left of the screen, at a random height
	// TODO 3: Random mass, from 1 kg to 100 kg
	if (spawn_timer.ReadSec() >= 1.0)
	{
		spawn_timer.Start();

		float mass = (float)GetRandomValue(1, 100);
		int radius = Ball::Radius(mass);

		// Keep it away from the sensor at the bottom
		int y = GetRandomValue(radius, SCREEN_HEIGHT - 50 - radius);

		balls.push_back(new Ball(App->physics, radius, y, mass, circle));
	}

	// TODO 1: Destroy the balls after 10 seconds
	for (int i = (int)balls.size() - 1; i >= 0; --i)
	{
		if (balls[i]->life.ReadSec() >= Ball::lifetime)
			DestroyBall(balls[i]);
	}

	for (Ball* ball : balls)
	{
		// TODO 2: Push each ball to the right with a constant force
		b2Vec2 force(Ball::push_force, 0.0f);
		ball->body->body->ApplyForceToCenter(force, true);

		// TODO 4: Friction against the floor: static and dynamic
		App->physics->ApplyFriction(ball->body, force);
	}

	// TODO 5: S increases the static friction coefficient. Past 1.0, back to 0
	if (IsKeyPressed(KEY_S))
	{
		App->physics->static_friction += 0.1f;
		if (App->physics->static_friction > 1.05f) // float sums: 1.0 may be 1.0000001
			App->physics->static_friction = 0.0f;
	}

	// TODO 6: D increases the dynamic friction coefficient. Past 1.0, back to 0
	if (IsKeyPressed(KEY_D))
	{
		App->physics->dynamic_friction += 0.1f;
		if (App->physics->dynamic_friction > 1.05f)
			App->physics->dynamic_friction = 0.0f;
	}

	if (IsKeyPressed(KEY_SPACE))
	{
		ray_on = !ray_on;
		ray.x = GetMouseX();
		ray.y = GetMouseY();
	}

	if (IsKeyPressed(KEY_ONE))
	{
		Circle* c = new Circle(App->physics, GetMouseX(), GetMouseY(), circle);
		c->body->listener = this;
		entities.push_back(c);
	}

	if (IsKeyPressed(KEY_TWO))
	{
		entities.push_back(new Box(App->physics, GetMouseX(), GetMouseY(), box));
	}

	if (IsKeyPressed(KEY_THREE))
	{
		entities.push_back(new Chain(App->physics, GetMouseX(), GetMouseY()));
	}

	if (IsKeyPressed(KEY_FOUR))
	{
		entities.push_back(new Rick(App->physics, GetMouseX(), GetMouseY(), rick));
	}

	if (IsKeyPressed(KEY_BACKSPACE))
	{
		DestroyAllEntities();
	}

	// Prepare for raycast ------------------------------------------------------

	vec2i mouse;
	mouse.x = GetMouseX();
	mouse.y = GetMouseY();
	int ray_hit = ray.DistanceTo(mouse);

	vec2f normal(0.0f, 0.0f);

	// All draw functions ------------------------------------------------------

	for (Ball* ball : balls)
	{
		ball->Update();
	}

	for (PhysicEntity* entity : entities)
	{
		entity->Update();

		// Keep the closest hit
		if (ray_on)
		{
			vec2f hit_normal(0.0f, 0.0f);
			int hit = entity->body->RayCast(ray.x, ray.y, mouse.x, mouse.y, hit_normal.x, hit_normal.y);
			if (hit >= 0 && hit < ray_hit)
			{
				ray_hit = hit;
				normal = hit_normal;
			}
		}
	}

	// TODO 5 and 6: Show both coefficients at the bottom of the screen
	DrawText(TextFormat("Static (S): %.1f   Dynamic (D): %.1f", App->physics->static_friction, App->physics->dynamic_friction), 10, SCREEN_HEIGHT - 30, 20, BLACK);

	// ray -----------------
	if (ray_on == true)
	{
		vec2f destination((float)(mouse.x - ray.x), (float)(mouse.y - ray.y));
		destination.Normalize();
		destination *= (float)ray_hit;

		DrawLine(ray.x, ray.y, (int)(ray.x + destination.x), (int)(ray.y + destination.y), RED);

		if (normal.x != 0.0f || normal.y != 0.0f)
		{
			DrawLine((int)(ray.x + destination.x), (int)(ray.y + destination.y), (int)(ray.x + destination.x + normal.x * 25.0f), (int)(ray.y + destination.y + normal.y * 25.0f), Color{ 100, 255, 100, 255 });
		}
	}

	return UPDATE_CONTINUE;
}

// Circles play bonus_fx when they collide
void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// Bodies that touch the sensor are destroyed
	if (bodyA == sensor)
	{
		// Never destroy bodies inside a collision callback: the world is locked
		// Keep them in a list and destroy them in Update()
		if (bodyB != NULL && std::find(bodies_to_destroy.begin(), bodies_to_destroy.end(), bodyB) == bodies_to_destroy.end())
			bodies_to_destroy.push_back(bodyB);
		return;
	}

	if (bodyB == sensor)
		return;

	App->audio->PlayFx(bonus_fx);
}

void ModuleGame::DestroyEntity(PhysicEntity* entity)
{
	entities.erase(std::find(entities.begin(), entities.end(), entity));
	App->physics->DestroyBody(entity->body);
	delete entity;
}

void ModuleGame::DestroyBall(Ball* ball)
{
	balls.erase(std::find(balls.begin(), balls.end(), ball));
	App->physics->DestroyBody(ball->body);
	delete ball;
}

void ModuleGame::DestroyAllEntities()
{
	for (PhysicEntity* entity : entities)
	{
		App->physics->DestroyBody(entity->body);
		delete entity;
	}
	entities.clear();

	for (Ball* ball : balls)
	{
		App->physics->DestroyBody(ball->body);
		delete ball;
	}
	balls.clear();

	bodies_to_destroy.clear();
}
