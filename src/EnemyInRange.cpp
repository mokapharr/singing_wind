#include "EnemyInRange.h"
#include "StaticGrid.h"
#include "Collision.h"
#include "GameWorld.h"
#include "Pathfinding.h"
#include "PosComponent.h"
#include "TagComponent.h"

EnemyInRange::EnemyInRange(GameWorld& world, unsigned int entity, float radius)
  : m_world(world)
  , m_entity(entity)
  , m_radius(radius)
{
}

behaviour_tree::Status
EnemyInRange::update()
{
  using namespace behaviour_tree;
  const WVec& pos = m_world.get<PosComponent>(m_entity).global_position;
  auto colliders =
    m_world.prune_sweep().find_in_radius(pos, m_radius, m_entity);

  for (auto& col : colliders) {
    if (m_world.get<TagComponent>(col.entity)
          .tags.test(static_cast<int>(Tags::Protagonist))) {
      m_world.get<PathingComponent>(m_entity).following = col.entity;
      auto cast_result =
        m_world.grid().raycast_against_grid(pos, (col.maxs + col.mins) / 2.0f);
      if (!cast_result.hits) {
        return Status::Success;
      }
    }
  }

  return Status::Failure;
}
