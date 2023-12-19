#ifndef KINDLING_FURY_HANDLER_H
#define KINDLING_FURY_HANDLER_H

#include "Player.h"
#include "ScriptMgr.h"

class KindlingFuryHandler : public PlayerScript
{
public:
    KindlingFuryHandler() : PlayerScript("KindlingFuryHandler") {}

    void OnSpellCast(Player* player, Spell* spell, bool skipCheck) override;

private:
    static const uint32 SPELL_IDS[3];
    static const int REQUIRED_AURA_STACKS;
};

#endif // KINDLING_FURY_HANDLER_H
