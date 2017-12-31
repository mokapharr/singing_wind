#include "TestEnemyAI.h"
#include "GameWorld.h"
#include "Pathfinding.h"
#include "Behaviours.h"

behaviour_tree::BehaviourTree testenemy::get_tree(GameWorld &world, unsigned int entity) {
    using namespace behaviour_tree;
    auto tree = BehaviourTreeBuilder()
        .decorator<Inverter>()
            .node<EnemyInRange>(world, entity, 300)
            .end()
            // TODO: maybe do nothing?
        .end()
        .node<GoToEnemy>(world, entity, 500)
        .end()
    .end();

    return tree;
};