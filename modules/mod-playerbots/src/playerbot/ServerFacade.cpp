
#include "playerbot/playerbot.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/ServerFacade.h"

#include "Database/DatabaseEnv.h"
#include "PlayerbotAI.h"

#include "Movement/TargetedMovementGenerator.h"

ServerFacade::ServerFacade() {}
ServerFacade::~ServerFacade() {}

float ServerFacade::GetDistance2d(Unit *unit, WorldObject* wo)
{
    if (!unit || !wo)
        return false;

    float dist = unit->GetDistance2d(wo, SizeFactor::None);
    return round(dist * 10.0f) / 10.0f;
}

float ServerFacade::GetDistance2d(Unit *unit, float x, float y)
{
    float dist = unit->GetDistance2d(x, y, SizeFactor::None);
    return round(dist * 10.0f) / 10.0f;
}

float ServerFacade::GetAggroDistance(Unit* attacker, Unit* target)
{
    if (!attacker || !target)
        return 0.0f;

    // Penqle owns aggro-radius calculation on Creature rather than Unit.
    // Player units do not automatically aggro nearby enemies, so zero is the
    // correct semantic for the travel/flee hazard checks using this helper.
    if (Creature* creature = attacker->ToCreature())
        return creature->GetAttackDistance(target);

    return 0.0f;
}

bool ServerFacade::IsDistanceLessThan(float dist1, float dist2)
{
    return dist1 - dist2 < sPlayerbotAIConfig.targetPosRecalcDistance;
}

bool ServerFacade::IsDistanceGreaterThan(float dist1, float dist2)
{
    return dist1 - dist2 > sPlayerbotAIConfig.targetPosRecalcDistance;
}

bool ServerFacade::IsDistanceGreaterOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceLessThan(dist1, dist2);
}

bool ServerFacade::IsDistanceLessOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceGreaterThan(dist1, dist2);
}

void ServerFacade::SetFacingTo(Unit* unit, float angle, bool force)
{
    MotionMaster &mm = *unit->GetMotionMaster();
    if (!force && !unit->IsStopped()) unit->SetFacingTo(angle);
    else
    {
        unit->SetOrientation(angle);
        unit->SendHeartBeat();
    }
    //unit->m_movementInfo.RemoveMovementFlag(MovementFlags(MOVEFLAG_SPLINE_ENABLED | MOVEFLAG_FORWARD));
}

bool ServerFacade::IsFriendlyTo(Unit* bot, Unit* to)
{
    return bot->IsFriendlyTo(to);
}

bool ServerFacade::IsHostileTo(Unit* bot, Unit* to)
{
    return bot->IsHostileTo(to);
}

bool ServerFacade::IsFriendlyTo(WorldObject* bot, Unit* to)
{
    return bot->IsFriendlyTo(to);
}

bool ServerFacade::IsHostileTo(WorldObject* bot, Unit* to)
{
    return bot->IsHostileTo(to);
}


bool ServerFacade::IsSpellReady(Player* bot, uint32 spell, uint32 /*itemId*/)
{
    SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spell);
    return spellInfo && !bot->HasSpellCooldown(spell) && !bot->HasSpellCategoryCooldown(spellInfo->Category);
}



bool ServerFacade::IsUnderwater(Unit* unit)
{
    return unit->IsUnderwater();
}

FactionTemplateEntry const* ServerFacade::GetFactionTemplateEntry(Unit* unit)
{
    return unit->GetFactionTemplateEntry();
}

namespace
{
struct TargetedMovementInfo
{
    Unit* target = nullptr;
    float angle = 0.0f;
    float offset = 0.0f;
};

template <class Owner>
TargetedMovementInfo GetTargetedMovementInfo(MovementGenerator const* generator, MovementGeneratorType type)
{
    if (!generator)
        return {};

    if (type == CHASE_MOTION_TYPE)
    {
        auto const* chase = static_cast<ChaseMovementGenerator<Owner> const*>(generator);
        return { chase->GetTarget(), chase->GetAngle(), chase->GetOffset() };
    }

    if (type == FOLLOW_MOTION_TYPE)
    {
        auto const* follow = static_cast<FollowMovementGenerator<Owner> const*>(generator);
        return { follow->GetTarget(), follow->GetAngle(), follow->GetOffset() };
    }

    return {};
}

TargetedMovementInfo GetTargetedMovementInfo(Unit* owner)
{
    if (!owner || !owner->GetMotionMaster())
        return {};

    MovementGenerator const* generator = owner->GetMotionMaster()->GetCurrent();
    MovementGeneratorType type = owner->GetMotionMaster()->GetCurrentMovementGeneratorType();
    return owner->IsPlayer()
        ? GetTargetedMovementInfo<Player>(generator, type)
        : GetTargetedMovementInfo<Creature>(generator, type);
}
}

Unit* ServerFacade::GetChaseTarget(Unit* target)
{
    return GetTargetedMovementInfo(target).target;
}

float ServerFacade::GetChaseAngle(Unit* target)
{
    return GetTargetedMovementInfo(target).angle;
}

float ServerFacade::GetChaseOffset(Unit* target)
{
    return GetTargetedMovementInfo(target).offset;
}

bool ServerFacade::isMoving(Unit* unit)
{
    return !unit->IsStopped() ||
        unit->m_movementInfo.HasMovementFlag(MovementFlags(MOVEFLAG_JUMPING | MOVEFLAG_FALLINGFAR));
}
