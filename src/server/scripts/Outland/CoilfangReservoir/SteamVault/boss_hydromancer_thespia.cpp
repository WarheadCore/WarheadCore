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
#include "steam_vault.h"

enum HydromancerThespia
{
    SAY_SUMMON                  = 0,
    SAY_AGGRO                   = 1,
    SAY_SLAY                    = 2,
    SAY_DEAD                    = 3,

    SPELL_LIGHTNING_CLOUD       = 25033,
    SPELL_LUNG_BURST            = 31481,
    SPELL_ENVELOPING_WINDS      = 31718,

    EVENT_SPELL_LIGHTNING       = 1,
    EVENT_SPELL_LUNG            = 2,
    EVENT_SPELL_ENVELOPING      = 3
};

class boss_hydromancer_thespia : public CreatureScript
{
public:
    boss_hydromancer_thespia() : CreatureScript("boss_hydromancer_thespia") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_thespiaAI (creature);
    }

    struct boss_thespiaAI : public ScriptedAI
    {
        boss_thespiaAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
            if (instance)
                instance->SetData(TYPE_HYDROMANCER_THESPIA, NOT_STARTED);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEAD);
            if (instance)
                instance->SetData(TYPE_HYDROMANCER_THESPIA, DONE);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SPELL_LIGHTNING, 15s);
            events.ScheduleEvent(EVENT_SPELL_LUNG, 7s);
            events.ScheduleEvent(EVENT_SPELL_ENVELOPING, 9s);

            if (instance)
                instance->SetData(TYPE_HYDROMANCER_THESPIA, IN_PROGRESS);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_LIGHTNING:
                    for (uint8 i = 0; i < DUNGEON_MODE(1, 2); ++i)
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            me->CastSpell(target, SPELL_LIGHTNING_CLOUD, false);
                    events.RepeatEvent(15s, 25s);
                    break;
                case EVENT_SPELL_LUNG:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_LUNG_BURST);
                    events.RepeatEvent(7s, 12s);
                    break;
                case EVENT_SPELL_ENVELOPING:
                    for (uint8 i = 0; i < DUNGEON_MODE(1, 2); ++i)
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            me->CastSpell(target, SPELL_ENVELOPING_WINDS, false);
                    events.RepeatEvent(10s, 15s);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_hydromancer_thespia()
{
    new boss_hydromancer_thespia();
}
