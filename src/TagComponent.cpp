#include "TagComponent.h"
#include "GameWorld.h"
#include "Components.h"
#include "imgui.h"
#include <map>

const std::map<Tags, const char*> tags_string = {
    {Tags::Actor, "Actor"},
    {Tags::Protagonist, "Protagonist"},
    {Tags::Enemy, "Enemy"},
    {Tags::Projectile, "Projectile"},
    {Tags::Hurtbox, "Hurtbox"}
};

void entity_edit_tags(GameWorld &world, unsigned int entity) {
    using namespace ImGui;

    if (world.entities()[entity].test(CTag) and CollapsingHeader("tags")) {
        auto &tc = world.tag_c(entity);
        auto flags = tc.to_ulong();
        for (auto &pair : tags_string) {
            CheckboxFlags(pair.second, &flags, 1 << static_cast<int>(pair.first));
        }
        tc = bset(flags);
    }
}