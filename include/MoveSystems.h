//
// Created by tobi on 4/27/17.
//

#ifndef SINGING_WIND_MOVESYSTEMS_H
#define SINGING_WIND_MOVESYSTEMS_H

#include "WindDefs.h"
#include "Components.h"
#include <map>
#include <vector>
#include <memory>

class GameWorld;

enum class MoveStateName : int
{
  OnGround,
  Falling,
  Flying,
  SimpleFlying,
  Hover,
  state_count
};

enum class TimedMoveStateName : int
{
  None,
  Still,
  FlyingAccel,
  Knockback,
  MeleeAttack,
  MeleeCast,
  LoungeCast,
  LoungeAttack,
  LoungeAfterCast,
  DiskCast,
  DiskProjectile,
  SixPatternCast,
  SixPatternAfterCast,
  PatternProjectile,
  state_count
};

const std::map<MoveStateName, const char*> movestate_string = {
  { MoveStateName::OnGround, "OnGround" },
  { MoveStateName::Falling, "Falling" },
  { MoveStateName::Flying, "Flying" },
  { MoveStateName::SimpleFlying, "SimpleFlying" },
  { MoveStateName::Hover, "Hover" }
};

class MoveState
{
public:
  virtual void enter(GameWorld&, unsigned int) = 0;
  virtual void accel(GameWorld&, unsigned int) = 0;
  virtual MoveStateName name() = 0;
  // static transition func
};

class TimedMoveState
{
public:
  virtual void enter(GameWorld&, unsigned int) = 0;
  virtual void accel(GameWorld&, unsigned int) = 0;
  virtual void leave(GameWorld&, unsigned int) = 0;
  virtual std::unique_ptr<TimedMoveState> next() = 0;
  TimedMoveStateName name() { return m_name; }
  float timer;

protected:
  TimedMoveState(TimedMoveStateName name, float timer)
    : timer(timer)
    , m_name(name)
  {
  }

private:
  TimedMoveStateName m_name;
};

enum class MoveSetName : int
{
  Protagonist,
  TestEnemy,
  TimedOnly
};

class MoveSet
{
public:
  virtual std::unique_ptr<MoveState> transition(GameWorld&, unsigned int) = 0;
  virtual void init(GameWorld&, unsigned int) = 0;
  virtual MoveSetName name() const = 0;
};

struct MoveComponent
{
  WVec velocity = { 0, 0 };
  WVec accel = { 0, 0 };
  WVec additional_force = { 0, 0 };
  std::unique_ptr<MoveState> movestate = nullptr;
  std::unique_ptr<MoveSet> moveset = nullptr;
  std::unique_ptr<TimedMoveState> special_movestate = nullptr;
  float mass = 1;
  float time_fac = 1;

  float c_max_change_angle = 0.08;
  float timer = 0;
  static const Components type = CMove;
};

struct GroundMoveComponent
{
  float c_accel = 1000;
  float c_stop_friction = 8;
  float c_turn_mod = 4;
  float c_max_vel = 100;
  static const Components type = CGroundMove;
};

struct FallComponent
{
  float c_accel = 500;
  float c_turn_mod = 4;
  float c_jump_height = 1000;
  float c_max_vel = 100;
  static const Components type = CFall;
};

struct FlyComponent
{
  float c_lift = 0.0055;
  float c_stall_angle = 0.26;
  float c_accel_force = 1000;
  float c_accel_time = 0.7f;
  float c_drag = 0.0026;
  float c_push_vel = 500.f;

  // bcurve for acceling
  WVec from = { 0.000, 0.000 };
  WVec ctrl_from = { 0.819312513, -0.31962499 };
  WVec ctrl_to = { -0.0288125277, 1.40112495 };
  WVec to = { 1.000, 0.870875001 };
  static const Components type = CFly;
};

struct SimpleFlyComponent
{
  float c_max_vel = 500;
  float c_accel = 1000;
  float c_arrive_radius = 70;
  float c_stop_coef = 0.04;
  static const Components type = CSimpleFly;
};

class TimedOnlyMoveSet : public MoveSet
{
public:
  std::unique_ptr<MoveState> transition(GameWorld&, unsigned int) override
  {
    return nullptr;
  }
  void init(GameWorld&, unsigned int) override;
  MoveSetName name() const override { return MoveSetName::TimedOnly; }
};

namespace movement {
void
interrupt(GameWorld&, unsigned int);
void
init_moveset(GameWorld&, unsigned int, MoveSetName);
void
entity_edit(GameWorld&, unsigned int);
}

#endif // SINGING_WIND_MOVESYSTEMS_H
