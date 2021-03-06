#include "Pathfinding.h"
#include "Collision.h"
#include "Components.h"
#include "GameWorld.h"
#include "StaticGrid.h"
#include "InputComponent.h"
#include "NavMesh.h"
#include "PosComponent.h"
#include "WVecMath.h"
#include "CollisionComponent.h"
#include <imgui.h>
#include <queue>

template<class pair>
struct PQCompare
{
  inline bool operator()(const pair& lhs, const pair& rhs)
  {
    return lhs.first > rhs.first;
  };
};

void
construct_path(const std::unordered_map<NavNode, NavNode>& mesh_path,
               PathingComponent& pc,
               const NavNode& min_heur_node)
{
  pc.path.clear();
  NavNode current = min_heur_node;
  NavNode next = mesh_path.at(min_heur_node);
  pc.path.push_back(WVec{ current.x, current.y });
  pc.path.push_back(
    WVec{ current.x, current.y }); // twice b/c index 0 is for real coord
  while (next != current) {
    pc.path.push_back(WVec{ next.x, next.y });
    current = next;
    next = mesh_path.at(current);
  }
}

PathfindingStatus
a_star_search(const NavMesh& mesh,
              const NavNode& start,
              const NavNode& goal,
              PathingComponent& pc)
{
  using namespace std;
  // todo: put in max_dist cutoff and construct path before returning

  // mesh.m_path.clear();
  // mesh.m_cost.clear();
  std::unordered_map<NavNode, NavNode> path;
  std::unordered_map<NavNode, float> cost;

  typedef pair<float, NavNode> PQElement;
  typedef priority_queue<PQElement, vector<PQElement>, PQCompare<PQElement>>
    PQueue;
  PQueue frontier;
  frontier.emplace(0, start);

  path[start] = start;
  cost[start] = 0;

  float min_heuristic = std::numeric_limits<float>::max();
  NavNode min_heur_node = start;

  while (!frontier.empty()) {
    auto current = frontier.top().second;
    frontier.pop();

    if (current == goal) {
      construct_path(path, pc, goal);
      return PathfindingStatus::Success;
    }

    for (const auto& link : mesh.m_graph.at(current)) {
      float new_cost = cost[current] + link.cost;
      if (!cost.count(link.to) || new_cost < cost[link.to]) {
        cost[link.to] = new_cost;
        auto heur = heuristic(link.to, goal);

        if (heur >= pc.c_max_mh_dist) {
          continue;
        }

        if (heur < min_heuristic) {
          min_heuristic = heur;
          min_heur_node = link.to;
        }

        float priority = new_cost + heur;
        frontier.emplace(priority, link.to);
        path[link.to] = current;
      }
    }
  }
  // construct_path(mesh.m_path, pc, min_heur_node);
  return PathfindingStatus::Failure;
}

PathfindingStatus
get_path(const WVec& from,
         const WVec& to,
         const GameWorld& world,
         PathingComponent& pc)
{
  // first of all check if there is direct visiblity
  auto direct_sight = world.grid().raycast_against_grid(from, to);
  if (!direct_sight.hits) {
    pc.path.clear();
    pc.path.push_back(to);
    pc.index = pc.path.size() - 1;
    return PathfindingStatus::Success;
  }

  // need to find real path
  const auto& mesh = world.navmesh();
  auto node_from = mesh.get_nearest_visible(from, world.grid());
  auto node_to = mesh.get_nearest_visible(to, world.grid());
  auto result = a_star_search(mesh, node_from, node_to, pc);
  if (result == PathfindingStatus::Failure) {
    return result;
  }

  // out goal to index 0
  pc.path[0] = to;

  // smooth path
  auto& node = pc.path[1];

  node.y -= pc.c_padding;

  pc.index = pc.path.size() - 1;
  return PathfindingStatus::Success;
}

void
get_path_platform(const WVec&, const WVec&, NavMesh&, PathingComponent&)
{
}
void
get_path_jump(const WVec&, const WVec&, NavMesh&, PathingComponent&)
{
}

// PathfindingStatus get_path(GameWorld &world, unsigned int entity) {
//     auto &pc = world.path_c(entity);
//     auto &pos = world.pos_c(entity).global_position;
//     WVec follow;
//     if (pc.following != 0) {
//         follow = world.pos_c(pc.following).global_position;
//     } else {
//         follow = world.input_c(entity).mouse.get();
//     }
//     return get_path_fly(pos, follow, world, pc);
// }

void
entity_edit_pathfind(GameWorld& world, unsigned int entity)
{
  using namespace ImGui;
  if (world.entities()[entity].test(CPathing) and CollapsingHeader("pathing")) {
    auto& pc = world.get<PathingComponent>(entity);

    if (DragFloat("max mh dist", &pc.c_max_mh_dist)) {
    }
    if (DragFloat("padding", &pc.c_padding)) {
    }
  }
}

WVec
nearest_dist_with_radii(const WVec& a_pos,
                        float a_radius,
                        const WVec& b_pos,
                        float b_radius)
{
  auto distance = b_pos - a_pos;
  return w_normalize(distance) * (w_magnitude(distance) - a_radius - b_radius);
}

bool
can_follow_path_until_zero(const WVec& pos,
                           PathingComponent& pc,
                           const GameWorld& world)
{
  while (pc.index > 0) {
    // can see current point
    auto result = world.grid().raycast_against_grid(pos, pc.path[pc.index]);
    if (result.hits) {
      return false;
    }
    result = world.grid().raycast_against_grid(pos, pc.path[pc.index - 1]);
    if (!result.hits) {
      pc.index--;
      pc.path.pop_back();
    } else {
      break;
    }
  }
  return true;
}
