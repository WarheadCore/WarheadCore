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
#include "SpellScript.h"
#include "molten_core.h"

enum Spells
{
    SPELL_ARCANE_EXPLOSION      = 19712,
    SPELL_SHAZZRAH_CURSE        = 19713,
    SPELL_MAGIC_GROUNDING       = 19714,
    SPELL_COUNTERSPELL          = 19715,
    SPELL_SHAZZRAH_GATE_DUMMY   = 23138, // Teleports to and attacks a random target.
    SPELL_SHAZZRAH_GATE         = 23139,
};

enum Events
{
    EVENT_ARCANE_EXPLOSION              = 1,
    EVENT_ARCANE_EXPLOSION_TRIGGERED    = 2,
    EVENT_SHAZZRAH_CURSE                = 3,
    EVENT_MAGIC_GROUNDING               = 4,
    EVENT_COUNTERSPELL                  = 5,
    EVENT_SHAZZRAH_GATE                 = 6,
};

class boss_shazzrah : public CreatureScript
{
public:
    boss_shazzrah() : CreatureScript("boss_shazzrah") { }

    struct boss_shazzrahAI : public BossAI
    {
        boss_shazzrahAI(Creature* creature) : BossAI(creature, BOSS_SHAZZRAH) { }

        void EnterCombat(Unit* target)
        {
            BossAI::EnterCombat(target);
            events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, 6000);
            events.ScheduleEvent(EVENT_SHAZZRAH_CURSE, 10000);
            events.ScheduleEvent(EVENT_MAGIC_GROUNDING, 24000);
            events.ScheduleEvent(EVENT_COUNTERSPELL, 15000);
            events.ScheduleEvent(EVENT_SHAZZRAH_GATE, 45000);
        }

        void EnterCombat(Unit* target)
        {
            BossAI::EnterCombat(target);
            events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, 6s);
            events.ScheduleEvent(EVENT_SHAZZRAH_CURSE, 10s);
            events.ScheduleEvent(EVENT_MAGIC_GROUNDING, 24s);
            events.ScheduleEvent(EVENT_COUNTERSPELL, 15s);
            events.ScheduleEvent(EVENT_SHAZZRAH_GATE, 45s);
        }

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                    switch (eventId)
                    {
                        case EVENT_ARCANE_EXPLOSION:
                            DoCastVictim(SPELL_ARCANE_EXPLOSION);
                            events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, 4s, 7s);
                            break;
                        // Triggered subsequent to using "Gate of Shazzrah".
                        case EVENT_ARCANE_EXPLOSION_TRIGGERED:
                            DoCastVictim(SPELL_ARCANE_EXPLOSION);
                            break;
                        case EVENT_SHAZZRAH_CURSE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, -SPELL_SHAZZRAH_CURSE))
                                DoCast(target, SPELL_SHAZZRAH_CURSE);
                            events.ScheduleEvent(EVENT_SHAZZRAH_CURSE, 25s, 30s);
                            break;
                        case EVENT_MAGIC_GROUNDING:
                            DoCast(me, SPELL_MAGIC_GROUNDING);
                            events.ScheduleEvent(EVENT_MAGIC_GROUNDING, 35s);
                            break;
                        case EVENT_COUNTERSPELL:
                            DoCastVictim(SPELL_COUNTERSPELL);
                            events.ScheduleEvent(EVENT_COUNTERSPELL, 16s, 20s);
                            break;
                        case EVENT_SHAZZRAH_GATE:
                            DoResetThreat();
                            DoCastAOE(SPELL_SHAZZRAH_GATE_DUMMY);
                            events.ScheduleEvent(EVENT_ARCANE_EXPLOSION_TRIGGERED, 2s);
                            events.RescheduleEvent(EVENT_ARCANE_EXPLOSION, 3s, 6s);
                            events.ScheduleEvent(EVENT_SHAZZRAH_GATE, 45s);
                            break;
                        default:
                            break;
                    }
            }
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI(Creature* creature) const
{
    return new boss_shazzrahAI(creature);
}
};

// 23138 - Gate of Shazzrah
class spell_shazzrah_gate_dummy : public SpellScriptLoader
{
public:
    spell_shazzrah_gate_dummy() : SpellScriptLoader("spell_shazzrah_gate_dummy") { }

    class spell_shazzrah_gate_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_shazzrah_gate_dummy_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SHAZZRAH_GATE))
                return false;
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (targets.empty())
                return;

            WorldObject* target = Warhead::Containers::SelectRandomContainerElement(targets);
            targets.clear();
            targets.push_back(target);
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetHitUnit())
            {
                target->CastSpell(GetCaster(), SPELL_SHAZZRAH_GATE, true);
                if (Creature* creature = GetCaster()->ToCreature())
                    creature->AI()->AttackStart(target); // Attack the target which caster will teleport to.
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_shazzrah_gate_dummy_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnEffectHitTarget += SpellEffectFn(spell_shazzrah_gate_dummy_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_shazzrah_gate_dummy_SpellScript();
    }
};

void AddSC_boss_shazzrah()
{
    new boss_shazzrah();
    new spell_shazzrah_gate_dummy();
}
