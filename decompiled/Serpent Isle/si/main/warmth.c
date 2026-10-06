/* Serpent Isle SI.EXE, overlay segment 253 (file offsets 0x06e200 to 0x06e54f, 847 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "itemrec.h"
#include "wihh.h"
#include "itable.h"

/* How well an NPC's clothes keep out the cold; an NPC with nothing on its body loses 75. */
int GetClothingWarmth(objref npc)
{
	int warmth = 0;

	switch (objref(GetItemInSlot(npc, 12)).type()) {
	case 569:
		warmth += 20;
		break;
	case 573:
		warmth -= 10;
		break;
	case 836:
		warmth += 5;
		break;
	case 419:
		warmth -= 9;
		break;
	case 570:
		warmth -= 8;
		break;
	case 571:
		warmth -= 5;
		break;
	case 666:
		warmth += 5;
		break;
	}
	if (objref(GetItemInSlot(npc, 2)).valid())
		switch (objref(GetItemInSlot(npc, 2)).frame()) {
		case 1:
		case 2:
		case 3:
		case 4:
			warmth += 70;
			break;
		case 0:
		case 5:
			warmth += 30;
			break;
		}
	switch (objref(GetItemInSlot(npc, 5)).type()) {
	case 579:
		if (objref(GetItemInSlot(npc, 5)).frame() > 0)
			warmth += 20;
		else
			warmth += 7;
		break;
	case 580:
		warmth -= 5;
		break;
	}
	switch (objref(GetItemInSlot(npc, 4)).type()) {
	case 1004:
		switch (objref(GetItemInSlot(npc, 4)).frame()) {
		case 0:
			warmth += 10;
			break;
		case 1:
			warmth += 35;
			break;
		case 2:
			warmth -= 8;
			break;
		case 3:
			warmth += 20;
			break;
		case 4:
			warmth += 35;
			break;
		}
		break;
	case 383:
	case 541:
		warmth += 5;
		break;
	case 1013:
		warmth += 10;
		break;
	case 542: {
		switch (objref(GetItemInSlot(npc, 4)).frame()) {
		case 0:
			warmth += 5;
			break;
		case 1:
			warmth += 5;
			break;
		case 2:
			warmth += 15;
			break;
		}
		break;
	}
	}
	if (!objref(GetItemInSlot(npc, 13)).valid())
		warmth -= 75;
	else
		switch (objref(GetItemInSlot(npc, 13)).type()) {
		case 587:
			switch (objref(GetItemInSlot(npc, 13)).frame()) {
			case 0:
			case 4:
				warmth += 10;
				break;
			case 2:
				warmth -= 25;
				break;
			case 1:
				warmth -= 10;
				break;
			case 3:
				warmth += 20;
				break;
			case 6:
				warmth += 5;
				break;
			case 5:
				break;
			}
			break;
		}
	switch (objref(GetItemInSlot(npc, 14)).type()) {
	case 574:
		warmth += 10;
		break;
	case 686:
		warmth += 5;
		break;
	case 677:
		warmth += 4;
		break;
	case 575:
	case 576:
		warmth -= 5;
		break;
	}
	return warmth;
}
