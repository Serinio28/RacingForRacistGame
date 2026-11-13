#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d/box2d.h"

// Gravity in m/s^2. Screen Y grows DOWNWARDS and we convert meters <-> pixels without flipping Y,
// so a POSITIVE Y gravity makes bodies fall down on screen. That is why the world receives -GRAVITY_Y.
// No gravity to the bottom of the screen
#define GRAVITY_X 0.0f
#define GRAVITY_Y 0.0f

// Top view: gravity points into the screen. Box2D never sees it.
// We only need it for the normal force: N = m * g
#define GRAVITY_Z 9.8f

// Fixed time step of world->Step()
#define PHYSICS_DT (1.0f / 60.0f)

// Units: Box2D works in meters (float), raylib draws in pixels (int)
#define PIXELS_PER_METER 50.0f // if touched change METER_PER_PIXEL too
#define METER_PER_PIXEL 0.02f  // this is 1 / PIXELS_PER_METER !

#define METERS_TO_PIXELS(m) ((int) floor(PIXELS_PER_METER * (m)))
#define PIXEL_TO_METERS(p)  ((float) METER_PER_PIXEL * (p))

// Small class to return to other modules to track position and rotation of physics bodies
class PhysBody
{
public:
	PhysBody() : body(NULL), listener(NULL)
	{}

	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
	b2Body* body;
	// Module that listens to the collisions of this body
	Module* listener;
};

// Module --------------------------------------
class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	// TODO 6: Collision filter. category: what the body is. mask: what it collides with. group: overrides both
	PhysBody* CreateCircle(int x, int y, int radius, float density = 1.0f, uint16 category = 0x0001, uint16 mask = 0xFFFF, int16 group = 0);
	PhysBody* CreateRectangle(int x, int y, int half_width, int half_height, uint16 category = 0x0001, uint16 mask = 0xFFFF, int16 group = 0);
	PhysBody* CreateRectangleSensor(int x, int y, int half_width, int half_height);
	PhysBody* CreateChain(int x, int y, const int* points, int size);

	void DestroyBody(PhysBody* pbody);

	// Top view friction against the floor. applied_force: the other forces on the body this frame (N)
	void ApplyFriction(PhysBody* pbody, b2Vec2 applied_force);

	void BeginContact(b2Contact* contact) override;

public:

	// Friction coefficients against the floor
	float static_friction;
	float dynamic_friction;

private:

	bool debug;
	b2World* world;

	// Static body with no fixtures, needed to create joints like the mouse joint
	b2Body* ground;
	b2MouseJoint* mouse_joint;
};
