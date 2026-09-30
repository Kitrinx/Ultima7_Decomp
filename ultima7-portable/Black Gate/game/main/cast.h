#ifndef CAST_H
#define CAST_H

struct objref;

extern int16_t SpellCost;
extern objref ReagentItems[8];

uint8_t HasQuantityFrames(uint16_t type);
void Item_setQuantity(objref item, int16_t quantity, uint8_t keepEmpty);
uint8_t CanCastSpell(objref caster, int16_t spell, uint8_t useReagents, uint8_t useMana);
uint8_t TryToCastSpell(objref caster, int16_t spell, uint8_t useReagents, uint8_t useMana,
	uint8_t event);
uint8_t CanAvatarReach(objref target, uint8_t ignoreRange);

#endif
