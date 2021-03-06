/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"
#include "SpellScript.h"
#include "Player.h"

enum Spells
{
    SPELL_MORTAL_WOUND                  = 25646,
    SPELL_ENRAGE_10                     = 28371,
    SPELL_ENRAGE_25                     = 54427,
    SPELL_DECIMATE_10                   = 28374,
    SPELL_DECIMATE_25                   = 54426,
    SPELL_BERSERK                       = 26662,
    SPELL_INFECTED_WOUND                = 29306,
    SPELL_CHOW_SEARCHER                 = 28404,
};

enum Events
{
    EVENT_SPELL_MORTAL_WOUND            = 1,
    EVENT_SPELL_ENRAGE                  = 2,
    EVENT_SPELL_DECIMATE                = 3,
    EVENT_SPELL_BERSERK                 = 4,
    EVENT_SUMMON_ZOMBIE                 = 5,
    EVENT_CAN_EAT_ZOMBIE                = 6,
};

enum Misc
{
    NPC_ZOMBIE_CHOW                     = 16360,
};

enum Emotes
{
    EMOTE_SPOTS_ONE   = 0,
    EMOTE_DECIMATE    = 1,
    EMOTE_ENRAGE      = 2,
    EMOTE_DEVOURS_ALL = 3,
    EMOTE_BERSERK     = 4
};

const Position zombiePos[3] =
{
    {3267.9f, -3172.1f, 297.42f, 0.94f},
    {3253.2f, -3132.3f, 297.42f, 0},
    {3308.3f, -3185.8f, 297.42f, 1.58f},
};

class boss_gluth : public CreatureScript
{
public:
    boss_gluth() : CreatureScript("boss_gluth") { }

    CreatureAI* GetAI(Creature* pCreature) const override
    {
        return new boss_gluthAI (pCreature);
    }

    struct boss_gluthAI : public BossAI
    {
        explicit boss_gluthAI(Creature* c) : BossAI(c, BOSS_GLUTH), summons(me)
        {
            pInstance = me->GetInstanceScript();
        }

        EventMap events;
        SummonList summons;
        InstanceScript* pInstance;

        void Reset() override
        {
            BossAI::Reset();
            me->ApplySpellImmune(SPELL_INFECTED_WOUND, IMMUNITY_ID, SPELL_INFECTED_WOUND, true);
            events.Reset();
            summons.DespawnAll();
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!me->GetVictim() || me->GetVictim()->GetEntry() != NPC_ZOMBIE_CHOW)
            {
                if (who->GetEntry() == NPC_ZOMBIE_CHOW && me->IsWithinDistInMap(who, 6.5f))
                {
                    SetGazeOn(who);
                    Talk(EMOTE_SPOTS_ONE);
                }
                else
                    ScriptedAI::MoveInLineOfSight(who);
            }
        }

        void EnterCombat(Unit* who) override
        {
            BossAI::EnterCombat(who);
            me->SetInCombatWithZone();
            events.ScheduleEvent(EVENT_SPELL_MORTAL_WOUND, 10s);
            events.ScheduleEvent(EVENT_SPELL_ENRAGE, 30s);
            events.ScheduleEvent(EVENT_SPELL_DECIMATE, 105s);
            events.ScheduleEvent(EVENT_SPELL_BERSERK, 8min);
            events.ScheduleEvent(EVENT_SUMMON_ZOMBIE, 10s);
            events.ScheduleEvent(EVENT_CAN_EAT_ZOMBIE, 1s);
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_ZOMBIE_CHOW)
                summon->AI()->AttackStart(me);

            summons.Summon(summon);
        }

        void SummonedCreatureDies(Creature* cr, Unit*) override { summons.Despawn(cr); }

        void KilledUnit(Unit* who) override
        {
            if (me->IsAlive() && who->GetEntry() == NPC_ZOMBIE_CHOW)
                me->ModifyHealth(int32(me->GetMaxHealth() * 0.05f));

            if (who->GetTypeId() == TYPEID_PLAYER && pInstance)
                pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
        }

        void JustDied(Unit*  killer) override
        {
            BossAI::JustDied(killer);
            summons.DespawnAll();
        }

        bool SelectPlayerInRoom()
        {
            if (me->IsInCombat())
                return false;

            for (const auto& itr : me->GetMap()->GetPlayers())
            {
                Player* player = itr.GetSource();

                if (!player || !player->IsAlive())
                    continue;

                if (player->GetPositionZ() > 300.0f || me->GetExactDist(player) > 50.0f)
                    continue;

                AttackStart(player);
                return true;
            }

            return false;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictimWithGaze() && !SelectPlayerInRoom())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_BERSERK:
                    me->CastSpell(me, SPELL_BERSERK, true);
                    break;
                case EVENT_SPELL_ENRAGE:
                    Talk(EMOTE_ENRAGE);
                    me->CastSpell(me, RAID_MODE_HEROIC(SPELL_ENRAGE_10, SPELL_ENRAGE_25), true);
                    events.RepeatEvent(30s);
                    break;
                case EVENT_SPELL_MORTAL_WOUND:
                    me->CastSpell(me->GetVictim(), SPELL_MORTAL_WOUND, false);
                    events.RepeatEvent(10s);
                    break;
                case EVENT_SPELL_DECIMATE:
                    Talk(EMOTE_DECIMATE);
                    me->CastSpell(me, RAID_MODE_HEROIC(SPELL_DECIMATE_10, SPELL_DECIMATE_25), false);
                    events.RepeatEvent(105s);
                    break;
                case EVENT_SUMMON_ZOMBIE:
                {
                    // In 10 man raid, normal mode - should spawn only from mid gate
                    // \1 |0 /2 pos
                    // In 25 man raid - should spawn from all 3 gates
                    if (me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                        me->SummonCreature(NPC_ZOMBIE_CHOW, zombiePos[0]);
                    else if (me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                    {
                        me->SummonCreature(NPC_ZOMBIE_CHOW, zombiePos[1]);
                        me->SummonCreature(NPC_ZOMBIE_CHOW, zombiePos[0]);
                        me->SummonCreature(NPC_ZOMBIE_CHOW, zombiePos[2]);
                    }
                    else // for 10 man heroic and 25 man normal
                    {
                        me->SummonCreature(NPC_ZOMBIE_CHOW, zombiePos[1]);
                        me->SummonCreature(NPC_ZOMBIE_CHOW, zombiePos[2]);
                    }

                    Seconds _repeat = me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC ? 15s : 10s;

                    events.RepeatEvent(_repeat);
                    break;
                }
                case EVENT_CAN_EAT_ZOMBIE:
                    events.RepeatEvent(1s);

                    if (me->GetVictim()->GetEntry() == NPC_ZOMBIE_CHOW && me->IsWithinMeleeRange(me->GetVictim()))
                    {
                        me->CastCustomSpell(SPELL_CHOW_SEARCHER, SPELLVALUE_RADIUS_MOD, 20000, me, true);
                        Talk(EMOTE_DEVOURS_ALL);
                        return; // leave it to skip DoMeleeAttackIfReady
                    }

                    break;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class spell_gluth_decimate : public SpellScriptLoader
{
public:
    spell_gluth_decimate() : SpellScriptLoader("spell_gluth_decimate") { }

    class spell_gluth_decimate_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gluth_decimate_SpellScript);

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            if (Unit* unitTarget = GetHitUnit())
            {
                int32 damage = int32(unitTarget->GetHealth()) - int32(unitTarget->CountPctFromMaxHealth(5));
                if (damage <= 0)
                    return;

                if (Creature* cTarget = unitTarget->ToCreature())
                {
                    cTarget->SetWalk(true);
                    cTarget->GetMotionMaster()->MoveFollow(GetCaster(), 0.0f, 0.0f, MOTION_SLOT_CONTROLLED);
                    cTarget->SetReactState(REACT_PASSIVE);
                    Unit::DealDamage(GetCaster(), cTarget, damage);
                    return;
                }

                GetCaster()->CastCustomSpell(28375, SPELLVALUE_BASE_POINT0, damage, unitTarget);
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_gluth_decimate_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_gluth_decimate_SpellScript();
    }
};

void AddSC_boss_gluth()
{
    new boss_gluth();
    new spell_gluth_decimate();
}
