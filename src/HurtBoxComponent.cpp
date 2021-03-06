#include "HurtBoxComponent.h"
#include "CollisionComponent.h"
#include "GameWorld.h"
#include "TagComponent.h"

#include <algorithm>

void
hurtbox::on_dynamic_collision(GameWorld& world, const unsigned int entity)
{
  auto& dc = world.get<DynamicColComponent>(entity);
  auto& hb = world.get<HurtBoxComponent>(entity);

  if (std::find(hb.hit_entities.begin(), hb.hit_entities.end(), dc.collided) !=
      hb.hit_entities.end()) {
    return;
    // only on Actors
  } else if (!world.get<TagComponent>(dc.collided)
                .tags.test(static_cast<int>(Tags::Actor))) {
    return;
  } else if (dc.collided == hb.owner) {
    // do nothing for now
  }

  auto fn = hb.hurt_function;
  if (fn) {
    if (fn(world, dc.collided, hb.owner, entity))
      hb.hit_entities.push_back(dc.collided);
  }
  // we ahve a hit! check if owner has made a response yet
  if (std::find(hb.hit_entities.begin(), hb.hit_entities.end(), hb.owner) ==
      hb.hit_entities.end()) {
    fn = hb.on_hit;
    if (fn) {
      if (fn(world, hb.owner, dc.collided, entity)) {
        hb.hit_entities.push_back(hb.owner);
      }
    }
  }
}
