#include "KindlingFuryHandler.h"

const uint32 KindlingFuryHandler::SPELL_IDS[3] = { 80040, 920353, 80040 };
const int KindlingFuryHandler::REQUIRED_AURA_STACKS = 14;

void KindlingFuryHandler::OnSpellCast(Player* player, Spell* spell, bool skipCheck)
{
    if (spell->GetSpellInfo()->Id == SPELL_IDS[0]) { // Updated line
        Aura* aura = player->GetAura(SPELL_IDS[2]);
        if (aura && aura->GetStackAmount() >= REQUIRED_AURA_STACKS) {
            player->CastSpell(player, SPELL_IDS[1], true);
            player->RemoveAura(SPELL_IDS[2]);
        }
    }
}

void AddSC_KindlingFuryHandler()
{
    new KindlingFuryHandler();
}
