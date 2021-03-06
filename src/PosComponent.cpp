#include "PosComponent.h"
#include "GameWorld.h"
#include "WindDefs.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#include "wraparound.h"

WVec
PosComponent::wrapped_position(const WVec& other)
{
  return get_wrapped_global_position(global_position, other);
}

void
build_global_transform(GameWorld& world, unsigned int entity)
{
  using namespace glm;
  auto& pc = world.get<PosComponent>(entity);
  assert(abs(pc.direction) == 1);
  pc.global_transform =
    world.get<PosComponent>(pc.parent).global_transform *
    rotate(scale(translate(WTransform(), pc.position), WVec(pc.direction, 1)),
           pc.rotation);
  pc.global_position = { pc.global_transform * WVec3(0.f, 0.f, 1.f) };
}

void
rotate_angle(float angle, float max_angle, PosComponent& pc)
{
  pc.rotation += copysignf(fmin(max_angle, abs(angle)), angle);

  if (pc.rotation < 0) {
    pc.rotation *= -1;
    pc.direction *= -1;
  } else if (pc.rotation > (float)M_PI) {
    pc.rotation = 2 * (float)M_PI - pc.rotation;
    pc.direction *= -1;
  }
}

void
rotate_to(const WVec& to, float max_angle, PosComponent& pc)
{
  assert(pc.rotation >= 0);
  auto local_to = WVec(glm::inverse(pc.global_transform) * WVec3(to, 1));
  float local_angle = atan2f(local_to.x, -local_to.y);
  rotate_angle(local_angle, max_angle, pc);
}
