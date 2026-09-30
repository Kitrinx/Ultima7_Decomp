#ifndef CAST_H
#define CAST_H

struct objref;

extern int SpellCost;
extern objref ReagentItems[8];

unsigned char HasQuantityFrames(unsigned type);
void Item_setQuantity(objref item, int quantity, unsigned char keepEmpty);
unsigned char CanCastSpell(objref caster, int spell, unsigned char useReagents, unsigned char useMana);
unsigned char TryToCastSpell(objref caster, int spell, unsigned char useReagents, unsigned char useMana,
	unsigned char event);
unsigned char CanAvatarReach(objref target, unsigned char ignoreRange);

#endif
