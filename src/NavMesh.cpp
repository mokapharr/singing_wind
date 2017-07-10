//
// Created by jammer on 22/05/17.
//

#include "NavMesh.h"
#include "triangulate.h"
#include "WVecMath.h"
#include "GameWorld.h"
#include "Collision.h"
#include "ColGrid.h"
#include <ColShape.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>

std::unordered_map<NavNode, std::vector<NavLink>> walkable_from_tri(std::array<WVec, 3> tri, const StaticGrid &grid) {
    std::unordered_map<NavNode, std::vector<NavLink>> graph;
    std::vector<NavNode> nodes;
    for (auto i = 0 ; i < 3 ; ++i) {
        // check if point is on top of island
        auto norm_tang = w_normalize(w_tangent(tri[i] - tri[(i+1)%3]));
        if (w_dot(WVec(0, 1), norm_tang) > c_max_floor_angle) {
            auto point = (tri[i] + tri[(i+1)%3]) / 2.f - norm_tang * 10.f;
            auto result = ColResult();

            auto col_p = ColPoint();
            col_p.transform(glm::translate(WTransform(), point));
            for (const auto &shape : grid.get_objects()) {
                auto res = shape->collides(col_p);
                if (res.collides) {
                    result.collides = true;
                }
            }
            if (!result.collides) {
                NavNode n1 = {tri[i].x, tri[i].y - 15};
                NavNode n2 = {tri[(i+1)%3].x, tri[(i+1)%3].y - 15};
                NavLink link_1 = {n2, n1};
                NavLink link_2 = {n1, n2};
                graph[n1].push_back(link_1);
                graph[n2].push_back(link_2);
            }
        }
    }
    return graph;
}

/*void fly_path_search(const WVec &from, const WVec &to, NavMesh &mesh) {
    const NavNode & node = mesh.get_nearest(from);
    const NavNode & _to = mesh.get_nearest(to);
    if (a_star_search(mesh.m_graph, node, _to, mesh.m_path)) {
    }
}*/

NavMesh build_navmesh(const std::vector<Island> &m_islands, StaticGrid &grid) {
    NavMesh mesh;
    std::vector<WVec> triangles;
    for (const auto &island : m_islands) {
        triangulate_island(island, triangles);
        for (unsigned int i = 0 ; i < triangles.size() / 3 ; ++i) {
            auto p1 = triangles[i*3];
            auto p2 = triangles[i*3+1];
            auto p3 = triangles[i*3+2];
            std::unordered_map<NavNode, std::vector<NavLink>> links = walkable_from_tri({{p1, p2, p3}}, grid);
            for (const auto &pr : links) {
                auto node = pr.first;
                for (const auto &link : pr.second) {
                    mesh.m_graph[node].push_back(link);
                }
            }
        }
    }
    build_levels_connections(mesh, grid);
    // todo check space for earch node
    build_node_space(mesh, grid);
    mesh.build_tree();
    return mesh;
}

void visit_node(NavNode &node, NavMesh &mesh, std::set<NavNode> &visited, unsigned int level) {
    if (visited.count(node)) {
        return;
    }
    mesh.m_levels[node] = level;
    visited.insert(node);
    for (auto &link : mesh.m_graph.at(node)) {
        visit_node(link.to, mesh, visited, level);
    }
}

LinkType find_link_type(const NavLink &link) {
    float dx = abs(link.to.x - link.from.x);
    float dy = link.to.y - link.from.y;

    if (abs(dy) > dx) {
        // fall or jump
        if (dy < 0) {
            return LinkType::JumpUp;
        }
        return LinkType::Drop;
    }
    // along
    return LinkType::JumpAlong;
}

void find_shortest_links(unsigned int l1, unsigned int l2, NavMesh &mesh, StaticGrid &grid) {
    struct Match {
        float dist = std::numeric_limits<float>::max();
        NavNode node;
        bool create= false;
    };
    std::unordered_map<NavNode, Match> outer_matches;
    std::unordered_map<NavNode, Match> inner_matches;

    for (auto &outer: mesh.m_graph) {
        auto &outer_node = outer.first;
        if (mesh.m_levels.at(outer_node) != l1) {
            continue;
        }
        for (auto &inner : mesh.m_graph) {
            auto &inner_node = inner.first;
            if (mesh.m_levels.at(inner_node) != l2) {
                continue;
            }
            auto result = cast_ray_vs_static_grid(grid, outer_node, inner_node);
            if (result.hits ){// < w_magnitude(outer_node - inner_node)) {
                continue;
            }
            NavLink link{outer_node, inner_node};
            if (link.cost < outer_matches[outer_node].dist && link.cost < inner_matches[inner_node].dist) {
                outer_matches[outer_node].node = inner_node;
                outer_matches[outer_node].dist = link.cost;
                outer_matches[outer_node].create = true;

                inner_matches[inner_node].node = outer_node;
                inner_matches[inner_node].dist = link.cost;
                inner_matches[inner_node].create = true;
            }
        }
    }

    for (auto &pair : outer_matches) {
        auto &n1 = pair.first;
        auto &match = pair.second;
        if (match.create && inner_matches.at(match.node).node == n1) {
            NavLink l1{match.node, n1};
            l1.link_type = find_link_type(l1);
            mesh.m_graph.at(n1).push_back(l1);

            NavLink l2{n1, match.node};
            l2.link_type = find_link_type(l2);
            mesh.m_graph.at(match.node).push_back(l2);
        }

    }
}

void build_levels_connections(NavMesh &mesh, StaticGrid &grid) {
    // first add level to nodes
    unsigned int level = 0;
    std::set<NavNode> visited;
    for (auto &pair : mesh.m_graph) {
        auto node = pair.first;
        if (!visited.count(node)) {
            level++;
        }

        visit_node(node, mesh, visited, level);
    }

    // find shortest link to each node on other level
    unsigned int max_level = 0;
    for (auto & pair : mesh.m_graph) {
        if (mesh.m_levels.at(pair.first) > max_level) {
            max_level = mesh.m_levels.at(pair.first);
        }
    }
    for (unsigned int i = 0 ; i < max_level ; ++i) {
        for (unsigned int j = i+1 ; j <= max_level ; ++j) {
            find_shortest_links(i, j, mesh, grid);
        }
    }
}

void build_node_space(NavMesh &mesh, StaticGrid &grid) {
    float check_height = 10000;
    for (auto &pair : mesh.m_graph) {
        NodeSpace ns;
        auto &inode = pair.first;
        auto node = WVec{inode.x, inode.y};
        // up
        auto result = cast_ray_vs_static_grid(grid, node, node + WVec{0, -check_height});
        if (result.hit_parameter < check_height) {
            ns.up = result.hit_parameter;
        }
        // left
        result = cast_ray_vs_static_grid(grid, node, node + WVec{-check_height, 0});
        if (result.hit_parameter < check_height) {
            ns.left = result.hit_parameter;
        }
        // right
        result = cast_ray_vs_static_grid(grid, node, node + WVec{check_height, 0});
        if (result.hit_parameter < check_height) {
            ns.right = result.hit_parameter;
        }
        mesh.m_space[inode] = ns;
    }
}

NavLink::NavLink(const NavNode &to, const NavNode &from)
    : to(to), from(from) {
    // manhattan distance
    cost = abs(to.x - from.x) + abs(to.y - from.y);
}


NavTree::NavTree() : m_cloud(), m_kd_tree(2, m_cloud, nanoflann::KDTreeSingleIndexAdaptorParams(5)) {
}

void NavTree::rebuild(const std::vector<NavNode> &nodes) {
    m_cloud.nodes = nodes;
    m_kd_tree.buildIndex();
}

std::vector<NavNode> NavTree::get_nearest(const WVec &pos, unsigned int n) {
    float query_pt[2] = {pos.x, pos.y};

    std::vector<size_t>   ret_index(n);
    std::vector<float> out_dist_sqr(n);

    m_kd_tree.knnSearch(&query_pt[0], n, ret_index.data(), out_dist_sqr.data());
    std::vector<NavNode> out;
    out.reserve(n);
    for (auto i : ret_index) {
        out.push_back(m_cloud.nodes[i]);
    }
    assert(out[0] == m_cloud.nodes[ret_index[0]]);
    return out;
}

void NavMesh::build_tree() {
    std::vector<NavNode> nodes;
    for (auto &pr : m_graph) {
        auto & node = pr.first;
        nodes.push_back(node);
    }
    m_tree.rebuild(nodes);
}

NavNode NavMesh::get_nearest(const WVec &pos) {
    return m_tree.get_nearest(pos)[0];
}

NavNode NavMesh::get_nearest_visible(const WVec &pos, StaticGrid &grid) {
    // get four nearest
    auto nodes = m_tree.get_nearest(pos, 4);
    for (auto &node : nodes) {
        if (!cast_ray_vs_static_grid(grid, pos, WVec{node.x, node.y}).hits) {
            return node;
        }
    }
    return nodes[0];
}

NavMesh::NavMesh(const NavMesh &other) {
    m_graph = other.m_graph;
    m_levels = other.m_levels;
    m_space = other.m_space;

    build_tree();

}

NavMesh& NavMesh::operator=(const NavMesh &other) {
    m_graph = other.m_graph;
    m_levels = other.m_levels;
    m_space = other.m_space;

    build_tree();
    return *this;
}
