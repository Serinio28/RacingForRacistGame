#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

#include <algorithm>

// Only balls of the same colour merge
enum BallColor
{
	BALL_RED,
	BALL_BLUE,
	BALL_GHOST
};

enum CarColor
{
	CAR_RED,
	CAR_BLUE,
	CAR_BLACK
};

// TODO 5: Category flags, one bit each. Keep 0x0001 for DEFAULT: the rest of the scene
enum PhysicCategory
{
	CATEGORY_DEFAULT = 1 << 0,
	CATEGORY_RED = 1 << 1,
	CATEGORY_BLUE = 1 << 2,
	CATEGORY_GHOST = 1 << 3
};

// TODO 8: Same negative group: never collide with each other
enum PhysicGroup
{
	GROUP_GHOSTS = -1
};

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

// Top view ball: random mass and colour, pushed to the right, rubs against the floor
class Ball : public PhysicEntity
{
public:
	// Same push for every ball: a = F / m, heavier balls accelerate less
	static constexpr float push_force = 100.0f; // N, to the right

	// Seconds before the ball is destroyed
	static constexpr double lifetime = 10.0;

	// TODO 1: Initial velocity, in m/s
	// TODO 7: Send the collision filter of its colour
	Ball(ModulePhysics* physics, int _x, int _y, float _mass, BallColor _color, Texture2D _texture, b2Vec2 velocity = b2Vec2_zero)
		: PhysicEntity(physics->CreateCircle(_x, _y, Radius(_mass), Density(_mass), Category(_color), Mask(_color), Group(_color)))
		, mass(_mass)
		, color(_color)
		, texture(_texture)
	{
		body->body->SetLinearVelocity(velocity);
	}

	// Heavier balls are bigger
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

	// TODO 7: Red balls collide with red balls, ghosts and the scene. Same for blue
	// Ghosts collide with everything
	static uint16 Category(BallColor color)
	{
		switch (color)
		{
		case BALL_RED: return CATEGORY_RED;
		case BALL_BLUE: return CATEGORY_BLUE;
		default: return CATEGORY_GHOST;
		}
	}

	static uint16 Mask(BallColor color)
	{
		switch (color)
		{
		case BALL_RED: return CATEGORY_DEFAULT | CATEGORY_RED | CATEGORY_GHOST;
		case BALL_BLUE: return CATEGORY_DEFAULT | CATEGORY_BLUE | CATEGORY_GHOST;
		default: return CATEGORY_DEFAULT | CATEGORY_RED | CATEGORY_BLUE | CATEGORY_GHOST;
		}
	}

	// TODO 8: Ghosts share a negative group: they never touch each other, whatever their mask says
	static int16 Group(BallColor color)
	{
		return (color == BALL_GHOST) ? GROUP_GHOSTS : 0;
	}

	Color Tint() const
	{
		switch (color)
		{
		case BALL_RED: return Color{ 255, 110, 110, 255 };
		case BALL_BLUE: return Color{ 110, 150, 255, 255 };
		default: return Fade(WHITE, 0.4f);
		}
	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		int radius = Radius(mass);

		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { (float)x, (float)y, (float)radius * 2.0f, (float)radius * 2.0f };
		Vector2 origin = { (float)radius, (float)radius };
		DrawTexturePro(texture, source, dest, origin, body->GetRotation() * RAD2DEG, Tint());

		const char* text = TextFormat("%d kg", (int)mass);
		DrawText(text, x - MeasureText(text, 20) / 2, y - 10, 20, BLACK);
	}

public:
	float mass;
	BallColor color;
	Timer life;

private:
	Texture2D texture;
};




class Coche : public PhysicEntity
{
public:
	// Same push for every ball: a = F / m, heavier balls accelerate less
	static constexpr float push_force = 100.0f; // N, to the right

	// Seconds before the ball is destroyed
	static constexpr double lifetime = 10.0;

	// TODO 1: Initial velocity, in m/s
	// TODO 7: Send the collision filter of its colour
	Coche(ModulePhysics* physics, int _x, int _y, float _mass, CarColor _color2, Texture2D _texture, b2Vec2 velocity = b2Vec2_zero)
		: PhysicEntity(physics->CreateCircle(_x, _y, Radius(_mass), Density(_mass), Category(_color2), Mask(_color2), Group(_color2)))
		, mass(_mass)
		, color2(_color2)
		, texture(_texture)
	{
		body->body->SetLinearVelocity(velocity);
	}

	// Heavier balls are bigger
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

	// TODO 7: Red balls collide with red balls, ghosts and the scene. Same for blue
	// Ghosts collide with everything
	static uint16 Category(CarColor color2)
	{
		switch (color2)
		{
		case CAR_RED: return CATEGORY_RED;
		case CAR_BLUE: return CATEGORY_BLUE;
		default: return CATEGORY_GHOST;
		}
	}

	static uint16 Mask(CarColor color2)
	{
		switch (color2)
		{
		case CAR_RED: return CATEGORY_DEFAULT | CATEGORY_RED | CATEGORY_GHOST;
		case CAR_BLUE: return CATEGORY_DEFAULT | CATEGORY_BLUE | CATEGORY_GHOST;
		default: return CATEGORY_DEFAULT | CATEGORY_RED | CATEGORY_BLUE | CATEGORY_GHOST;
		}
	}

	// TODO 8: Ghosts share a negative group: they never touch each other, whatever their mask says
	static int16 Group(CarColor color2)
	{
		return (color2 == CAR_BLACK) ? GROUP_GHOSTS : 0;
	}

	Color Tint() const
	{
		switch (color2)
		{
		case CAR_RED: return Color{ 255, 110, 110, 255 };
		case CAR_BLUE: return Color{ 110, 150, 255, 255 };
		default: return Fade(WHITE, 0.4f);
		}
	}

	void Update() override
	{
		int x, y;
		body->GetPosition(x, y);

		int radius = Radius(mass);


		const char* text = TextFormat("%d kg", (int)mass);
		DrawText(text, x - MeasureText(text, 20) / 2, y - 10, 20, BLACK);
	}

public:
	float mass;
	CarColor color2;

private:
	Texture2D texture;
};






class Carretera : public PhysicEntity
{
public:
	// Pivot 0, 0
	static constexpr int carretera[64] = {
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

	Carretera(ModulePhysics* physics, int _x, int _y, Texture2D _texture)
		: PhysicEntity(physics->CreateChain(_x, _y, carretera, 64))
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

	entities.push_back(new Carretera(App->physics, 0, 0, rick));


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

	// TODO 3: Merge the balls that touched: destroy both, create one with the sum of the masses
	for (const std::pair<Ball*, Ball*>& pair : balls_to_merge)
	{
		Ball* a = pair.first;
		Ball* b = pair.second;

		float mass = a->mass + b->mass;

		// At the centre of mass of both
		b2Vec2 position = (1.0f / mass) * (a->mass * a->body->body->GetPosition() + b->mass * b->body->body->GetPosition());

		// TODO 4: Conservation of momentum: m1 * v1 + m2 * v2 = (m1 + m2) * v
		b2Vec2 velocity = (1.0f / mass) * (a->mass * a->body->body->GetLinearVelocity() + b->mass * b->body->body->GetLinearVelocity());

		BallColor color = a->color;
		DestroyBall(a);
		DestroyBall(b);

		Ball* ball = new Ball(App->physics, METERS_TO_PIXELS(position.x), METERS_TO_PIXELS(position.y), mass, color, circle, velocity);
		ball->body->listener = this;
		balls.push_back(ball);

		App->audio->PlayFx(bonus_fx);
	}
	balls_to_merge.clear();

	// Top view balls ------------------------------------------------------

	// Every second, create a ball at the left of the screen, at a random height
	// Random mass, from 1 kg to 100 kg
	if (spawn_timer.ReadSec() >= 1.0)
	{
		spawn_timer.Start();

		float mass = (float)GetRandomValue(1, 100);
		int radius = Ball::Radius(mass);

		// Keep it away from the sensor at the bottom
		int y = GetRandomValue(radius, SCREEN_HEIGHT - 50 - radius);

		BallColor color = (BallColor)GetRandomValue(BALL_RED, BALL_GHOST);

		Ball* ball = new Ball(App->physics, radius, y, mass, color, circle);
		ball->body->listener = this; // we want its collisions
		balls.push_back(ball);
	}

	// Destroy the balls after 10 seconds
	for (int i = (int)balls.size() - 1; i >= 0; --i)
	{
		if (balls[i]->life.ReadSec() >= Ball::lifetime)
			DestroyBall(balls[i]);
	}

	for (Ball* ball : balls)
	{
		// Push each ball to the right with a constant force
		b2Vec2 force(Ball::push_force, 0.0f);
		ball->body->body->ApplyForceToCenter(force, true);

		// Friction against the floor: static and dynamic
		App->physics->ApplyFriction(ball->body, force);
	}

	// S increases the static friction coefficient. Past 1.0, back to 0
	if (IsKeyPressed(KEY_S))
	{
		App->physics->static_friction += 0.1f;
		if (App->physics->static_friction > 1.05f) // float sums: 1.0 may be 1.0000001
			App->physics->static_friction = 0.0f;
	}

	// D increases the dynamic friction coefficient. Past 1.0, back to 0
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

	// Show both coefficients at the bottom of the screen
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

// Circles play bonus_fx when they collide. Balls of the same colour merge
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

	// TODO 2: Two balls of the same colour touched: keep them, merge them in Update()
	// Never create or destroy bodies here: the world is locked
	// Each contact arrives twice, (A, B) and (B, A): add the pair only once
	Ball* a = FindBall(bodyA);
	if (a != NULL)
	{
		Ball* b = FindBall(bodyB);
		if (b != NULL && a->color == b->color && !IsMerging(a) && !IsMerging(b))
			balls_to_merge.push_back(std::make_pair(a, b));
		return;
	}

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

Ball* ModuleGame::FindBall(PhysBody* pbody)
{
	for (Ball* ball : balls)
	{
		if (ball->body == pbody)
			return ball;
	}
	return NULL;
}

// True if the ball is already in a pair to merge
bool ModuleGame::IsMerging(Ball* ball)
{
	for (const std::pair<Ball*, Ball*>& pair : balls_to_merge)
	{
		if (pair.first == ball || pair.second == ball)
			return true;
	}
	return false;
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
	balls_to_merge.clear();
}
