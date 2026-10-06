/* Serpent Isle SI.EXE, resident segment 101 (file offset 0x036b5a, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G -P rebuilds its empty code segment as shipped.
 * Its data is DS:46A6-49BE, between keywords.c's and combg_ov.c's, as link order places segment
 * 101: the 198 engine calls usecode reaches by number.
 */

#include "inter.h"
#include "uccomm5.h"
#include "uccomm7.h"
#include "uccomm12.h"

void far UC_Random(Value *args, Value *ret);
void far UC_Post(Value *args, Value *ret);
void far UC_PostInFuture(Value *args, Value *ret);
void far UC_SetSpeaker(Value *args, Value *ret);
void far UC_CloseSpeaker(Value *args, Value *ret);
void far UC_SetSpeaker0(Value *args, Value *ret);
void far UC_SetSpeaker1(Value *args, Value *ret);
void far UC_CloseSpeaker0(Value *args, Value *ret);
void far UC_CloseSpeaker1(Value *args, Value *ret);
void far UC_SetSpeakerSlot(Value *args, Value *ret);
void far UC_ChangeNpcFace0(Value *args, Value *ret);
void far UC_ChangeNpcFace1(Value *args, Value *ret);
void far UC_TurnOn(Value *args, Value *ret);
void far UC_TurnOff(Value *args, Value *ret);
void far UC_PushKeys(Value *args, Value *ret);
void far UC_PopKeys(Value *args, Value *ret);
void far UC_ClearKeys(Value *args, Value *ret);
void far UC_GetInput(Value *args, Value *ret);
void far UC_GetOrdInput(Value *args, Value *ret);
void far UC_InputNumericValue(Value *args, Value *ret);
void far UC_SetItemShape(Value *args, Value *ret);
void far UC_FindNearest(Value *args, Value *ret);
void far UC_Sfx(Value *args);
void far UC_DieRoll(Value *args, Value *ret);
void far UC_GetItemShape(Value *args, Value *ret);
void far UC_GetItemWeight(Value *args, Value *ret);
void far UC_GetItemFrame(Value *args, Value *ret);
void far UC_SetItemFrame(Value *args);
void far UC_GetQuality(Value *args, Value *ret);
void far UC_SetQuality(Value *args, Value *ret);
void far UC_GetItemQuantity(Value *args, Value *ret);
void far UC_SetQuantity(Value *args, Value *ret);
void far UC_GetCoord(Value *args, Value *ret);
void far UC_GetDist(Value *args, Value *ret);
void far UC_FindDirection(Value *args, Value *ret);
void far UC_GetNpcObject(Value *args, Value *ret);
void far UC_GetWorkType(Value *args, Value *ret);
void far UC_SetWorkType(Value *args, Value *ret);
void far UC_JoinParty(Value *args, Value *ret);
void far UC_LeaveParty(Value *args, Value *ret);
void far UC_GetNpcProp(Value *args, Value *ret);
void far UC_SetNpcProp(Value *args, Value *ret);
void far UC_GetAvatarRef(Value *args, Value *ret);
void far UC_GetPartyList(Value *args, Value *ret);
void far UC_CreateNewObject(Value *args, Value *ret);
void far UC_CreateCreature(Value *args, Value *ret);
void far UC_SetLastCreated(Value *args, Value *ret);
void far UC_PopToMap(Value *args, Value *ret);
void far UC_GetNPCName(Value *args, Value *ret);
void far UC_CountObjects(Value *args, Value *ret);
void far UC_FindObject(Value *args, Value *ret);
void far UC_GetContItems(Value *args, Value *ret);
void far UC_RemovePartyItems(Value *args, Value *ret);
void far UC_AddPartyItems(Value *args, Value *ret);
void far UC_GiveToCont(Value *args, Value *ret);
void far UC_TakeFromCont(Value *args, Value *ret);
void far UC_FindNearbyAvatar(Value *args, Value *ret);
void far UC_IsNpc(Value *args, Value *ret);
void far UC_DisplayRunes(Value *args, Value *ret);
void far UC_GetTarget(Value *args, Value *ret);
void far UC_ErrorMessage(Value *args, Value *ret);
void far UC_FindNearby(Value *args, Value *ret);
void far UC_PopToNPC(Value *args, Value *ret);
void far UC_PopToEnd(Value *args, Value *ret);
void far UC_IsDead(Value *args, Value *ret);
void far UC_GetHour(Value *args, Value *ret);
void far UC_GameMinute(Value *args, Value *ret);
void far UC_GetNpcNumber(Value *args, Value *ret);
void far UC_RemoveNpc(Value *args);
void far UC_ItemSay(Value *args, Value *ret);
void far UC_ClearBark(Value *args, Value *ret);
void far UC_SetToAttack(Value *args, Value *ret);
void far UC_GetLift(Value *args, Value *ret);
void far UC_SetLift(Value *args);
void far UC_GetWeather(Value *args, Value *ret);
void far UC_SetWeather(Value *args, Value *ret);
void far UC_SitDown(Value *args, Value *ret);
void far UC_DisplayMap(Value *args, Value *ret);
void far UC_KillNpc(Value *args);
void far UC_RollToWin(Value *args, Value *ret);
void far UC_SetAttackMode(Value *args, Value *ret);
void far UC_GetAttackMode(Value *args, Value *ret);
void far UC_SetOppressor(Value *args, Value *ret);
void far UC_GetAttacker(Value *args, Value *ret);
void far UC_GetWeapon(Value *args, Value *ret);
void far UC_SetTarget(Value *args, Value *ret);
void far UC_Clone(Value *args);
void far UC_DisplayArea(Value *args);
void far UC_WizardEye(Value *args);
void far UC_Resurrect(Value *args, Value *ret);
void far UC_ResurrectNPC(Value *args);
void far UC_BodyToNPC(Value *args, Value *ret);
void far UC_AddSpell(Value *args, Value *ret);
void far UC_ClearSpells(Value *args);
void far UC_SpriteEffect(Value *args);
void far UC_BookMode(Value *args, Value *ret);
void far UC_StopTime(Value *args);
void far UC_CauseLight(Value *args);
void far UC_GetBarge(Value *args, Value *ret);
void far UC_Earthquake(Value *args);
void far UC_AvatarSex(Value *args, Value *ret);
void far UC_Armageddon(void);
void far UC_Lightning(void);
void far UC_GetArraySize(Value *args, Value *ret);
void far UC_SaveCoord(void);
void far UC_RecallCoord(void);
void far UC_ApplyDamage(Value *args, Value *ret);
void far UC_GetClock(Value *args, Value *ret);
void far UC_SetClock(Value *args);
void far UC_GetSpeechTrack(Value *args, Value *ret);
void far UC_GetContainer(Value *args, Value *ret);
void far UC_RemoveItem(Value *args);
void far UC_RestoreMouseCursor(void);
void far UC_SelectNoArrowCursor(void);
void far UC_ReduceHealth(Value *args);
void far UC_IsReadied(Value *args, Value *ret);
void far UC_RestartGame(void);
void far UC_RunEndgame(Value *args);
void far UC_InUsecode(Value *args, Value *ret);
void far UC_ObjSpriteEffect(Value *args);
void far UC_PathRunUsecode(Value *args, Value *ret);
void far UC_UnusedItemWalk(Value *args, Value *ret);
void far UC_CanAvatarReach(Value *args, Value *ret);
void far UC_UnusedWalk(Value *args, Value *ret);
void far UC_CloseGumps(void);
void far UC_ItemSay2(Value *args);
void far UC_CloseGump(Value *args, Value *ret);
void far UC_InGumpMode(Value *args, Value *ret);
void far UC_ResetPalette(void);
void far UC_SetTimePalette(void);
void far UC_SetAmbientLight(Value *args);
void far UC_PlaySoundEffect2(Value *args);
void far UC_DirectionFrom(Value *args, Value *ret);
void far UC_GetItemFlag(Value *args, Value *ret);
void far UC_SetItemFlag(Value *args, Value *ret);
void far UC_ClearItemFlag(Value *args, Value *ret);
void far UC_AvatarSkin(Value *args, Value *ret);
void far UC_SetPathFailure(Value *args, Value *ret);
void far UC_GetPartyList2(Value *args, Value *ret);
void far UC_ResetConvFace(Value *args, Value *ret);
void far UC_SetCamera(Value *args);
void far UC_GetDeadParty(Value *args, Value *ret);
void far UC_ViewTile(Value *args);
void far UC_AOrAn(Value *args, Value *ret);
void far UC_GetTemperature(Value *args, Value *ret);
void far UC_GetTemperatureZone(Value *args, Value *ret);
void far UC_SetTemperature(Value *args, Value *ret);
void far UC_GetNpcWarmth(Value *args, Value *ret);
void far UC_GetNPCID(Value *args, Value *ret);
void far UC_SetNPCID(Value *args, Value *ret);
void far UC_GetEquip(Value *args, Value *ret);
void far UC_ApproachAvatar(Value *args, Value *ret);
void far UC_SetBargeDir(Value *args);
void far UC_PathfindNPC(Value *args, Value *ret);

/* the engine calls, indexed by number */
EngineCall EngineCalls[] = {
	UC_Random,                         /* 0x00 */
	UC_Post,                           /* 0x01 */
	UC_PostInFuture,                   /* 0x02 */
	UC_SetSpeaker,                     /* 0x03 */
	UC_CloseSpeaker,                   /* 0x04 */
	UC_SetSpeaker0,                    /* 0x05 */
	UC_SetSpeaker1,                    /* 0x06 */
	UC_CloseSpeaker0,                  /* 0x07 */

	UC_CloseSpeaker1,                  /* 0x08 */
	UC_SetSpeakerSlot,                 /* 0x09 */
	UC_ChangeNpcFace0,                 /* 0x0a */
	UC_ChangeNpcFace1,                 /* 0x0b */
	UC_TurnOn,                         /* 0x0c */
	UC_TurnOff,                        /* 0x0d */
	UC_PushKeys,                       /* 0x0e */
	UC_PopKeys,                        /* 0x0f */

	UC_ClearKeys,                      /* 0x10 */
	UC_GetInput,                       /* 0x11 */
	UC_GetOrdInput,                    /* 0x12 */
	UC_InputNumericValue,              /* 0x13 */
	UC_SetItemShape,                   /* 0x14 */
	UC_FindNearest,                    /* 0x15 */
	(EngineCall) UC_Sfx,               /* 0x16 */
	UC_DieRoll,                        /* 0x17 */

	UC_GetItemShape,                   /* 0x18 */
	UC_GetItemWeight,                  /* 0x19 */
	UC_GetItemFrame,                   /* 0x1a */
	(EngineCall) UC_SetItemFrame,      /* 0x1b */
	UC_GetQuality,                     /* 0x1c */
	UC_SetQuality,                     /* 0x1d */
	UC_GetItemQuantity,                /* 0x1e */
	UC_SetQuantity,                    /* 0x1f */

	UC_GetCoord,                       /* 0x20 */
	UC_GetDist,                        /* 0x21 */
	UC_FindDirection,                  /* 0x22 */
	UC_GetNpcObject,                   /* 0x23 */
	UC_GetWorkType,                    /* 0x24 */
	UC_SetWorkType,                    /* 0x25 */
	UC_JoinParty,                      /* 0x26 */
	UC_LeaveParty,                     /* 0x27 */

	UC_GetNpcProp,                     /* 0x28 */
	UC_SetNpcProp,                     /* 0x29 */
	UC_GetAvatarRef,                   /* 0x2a */
	UC_GetPartyList,                   /* 0x2b */
	UC_CreateNewObject,                /* 0x2c */
	UC_CreateCreature,                 /* 0x2d */
	UC_SetLastCreated,                 /* 0x2e */
	UC_PopToMap,                       /* 0x2f */

	UC_GetNPCName,                     /* 0x30 */
	UC_CountObjects,                   /* 0x31 */
	UC_FindObject,                     /* 0x32 */
	UC_GetContItems,                   /* 0x33 */
	UC_RemovePartyItems,               /* 0x34 */
	UC_AddPartyItems,                  /* 0x35 */
	UC_GiveToCont,                     /* 0x36 */
	UC_TakeFromCont,                   /* 0x37 */

	UC_GetMusicTrack,                  /* 0x38 */
	(EngineCall) UC_PlayMusic,         /* 0x39 */
	UC_IsNPCNear,                      /* 0x3a */
	UC_CanVisit,                       /* 0x3b */
	UC_FindNearbyAvatar,               /* 0x3c */
	UC_IsNpc,                          /* 0x3d */
	UC_DisplayRunes,                   /* 0x3e */
	UC_GetTarget,                      /* 0x3f */

	UC_ErrorMessage,                   /* 0x40 */
	UC_FindNearby,                     /* 0x41 */
	UC_PopToNPC,                       /* 0x42 */
	UC_PopToEnd,                       /* 0x43 */
	UC_IsDead,                         /* 0x44 */
	UC_GetHour,                        /* 0x45 */
	UC_GameMinute,                     /* 0x46 */
	UC_GetNpcNumber,                   /* 0x47 */

	UC_GetTime,                        /* 0x48 */
	UC_GetAlignment,                   /* 0x49 */
	(EngineCall) UC_SetAlignment,      /* 0x4a */
	(EngineCall) UC_MoveObject,        /* 0x4b */
	(EngineCall) UC_RemoveNpc,         /* 0x4c */
	UC_ItemSay,                        /* 0x4d */
	UC_ClearBark,                      /* 0x4e */
	UC_SetToAttack,                    /* 0x4f */

	UC_GetLift,                        /* 0x50 */
	(EngineCall) UC_SetLift,           /* 0x51 */
	UC_GetWeather,                     /* 0x52 */
	UC_SetWeather,                     /* 0x53 */
	UC_SitDown,                        /* 0x54 */
	UC_Summon,                         /* 0x55 */
	UC_DisplayMap,                     /* 0x56 */
	(EngineCall) UC_KillNpc,           /* 0x57 */

	UC_RollToWin,                      /* 0x58 */
	UC_SetAttackMode,                  /* 0x59 */
	UC_GetAttackMode,                  /* 0x5a */
	UC_SetOppressor,                   /* 0x5b */
	UC_GetAttacker,                    /* 0x5c */
	UC_GetWeapon,                      /* 0x5d */
	UC_SetTarget,                      /* 0x5e */
	(EngineCall) UC_Clone,             /* 0x5f */

	(EngineCall) UC_DoNothing,         /* 0x60 */
	(EngineCall) UC_DisplayArea,       /* 0x61 */
	(EngineCall) UC_WizardEye,         /* 0x62 */
	UC_Resurrect,                      /* 0x63 */
	(EngineCall) UC_ResurrectNPC,      /* 0x64 */
	UC_BodyToNPC,                      /* 0x65 */
	UC_AddSpell,                       /* 0x66 */
	(EngineCall) UC_ClearSpells,       /* 0x67 */

	(EngineCall) UC_SpriteEffect,      /* 0x68 */
	UC_AttackObject,                   /* 0x69 */
	UC_BookMode,                       /* 0x6a */
	(EngineCall) UC_StopTime,          /* 0x6b */
	(EngineCall) UC_CauseLight,        /* 0x6c */
	UC_GetBarge,                       /* 0x6d */
	(EngineCall) UC_Earthquake,        /* 0x6e */
	UC_AvatarSex,                      /* 0x6f */

	(EngineCall) UC_Armageddon,        /* 0x70 */
	(EngineCall) UC_HaltScheduled,     /* 0x71 */
	(EngineCall) UC_Lightning,         /* 0x72 */
	UC_GetArraySize,                   /* 0x73 */
	(EngineCall) UC_SaveCoord,         /* 0x74 */
	(EngineCall) UC_RecallCoord,       /* 0x75 */
	UC_ApplyDamage,                    /* 0x76 */
	UC_IsPcInside,                     /* 0x77 */

	(EngineCall) UC_SetOrrery,         /* 0x78 */
	(EngineCall) UC_SetPaletteFadedIn, /* 0x79 */
	UC_GetClock,                       /* 0x7a */
	(EngineCall) UC_SetClock,          /* 0x7b */
	UC_WearingFellowship,              /* 0x7c */
	UC_MouseExists,                    /* 0x7d */
	UC_GetSpeechTrack,                 /* 0x7e */
	UC_FlashMouse,                     /* 0x7f */

	UC_GetItemFrameRot,                /* 0x80 */
	(EngineCall) UC_SetItemFrameRot,   /* 0x81 */
	UC_OnBarge,                        /* 0x82 */
	UC_GetContainer,                   /* 0x83 */
	(EngineCall) UC_RemoveItem,        /* 0x84 */
	(EngineCall) UC_RestoreMouseCursor, /* 0x85 */
	(EngineCall) UC_SelectNoArrowCursor, /* 0x86 */
	(EngineCall) UC_ReduceHealth,      /* 0x87 */

	UC_IsReadied,                      /* 0x88 */
	(EngineCall) UC_RestartGame,       /* 0x89 */
	UC_StartSpeech,                    /* 0x8a */
	(EngineCall) UC_RunEndgame,        /* 0x8b */
	UC_FireProjectile,                 /* 0x8c */
	UC_NapTime,                        /* 0x8d */
	UC_AdvanceTime,                    /* 0x8e */
	UC_InUsecode,                      /* 0x8f */

	UC_CallGuards,                     /* 0x90 */
	(EngineCall) UC_ObjSpriteEffect,   /* 0x91 */
	UC_AttackAvatar,                   /* 0x92 */
	UC_StopArrest,                     /* 0x93 */
	UC_PathRunUsecode,                 /* 0x94 */
	UC_UnusedItemWalk,                 /* 0x95 */
	UC_CanAvatarReach,                 /* 0x96 */
	UC_UnusedWalk,                     /* 0x97 */

	(EngineCall) UC_CloseGumps,        /* 0x98 */
	(EngineCall) UC_ItemSay2,          /* 0x99 */
	UC_CloseGump,                      /* 0x9a */
	UC_InGumpMode,                     /* 0x9b */
	(EngineCall) UC_SetLight,          /* 0x9c */
	(EngineCall) UC_ResetPalette,      /* 0x9d */
	(EngineCall) UC_SetTimePalette,    /* 0x9e */
	(EngineCall) UC_SetAmbientLight,   /* 0x9f */

	UC_IsNotBlocked,                   /* 0xa0 */
	(EngineCall) UC_PlaySoundEffect2,  /* 0xa1 */
	UC_DirectionFrom,                  /* 0xa2 */
	UC_GetItemFlag,                    /* 0xa3 */
	UC_SetItemFlag,                    /* 0xa4 */
	UC_ClearItemFlag,                  /* 0xa5 */
	UC_AvatarSkin,                     /* 0xa6 */
	UC_SetPathFailure,                 /* 0xa7 */

	UC_FadePalette,                    /* 0xa8 */
	UC_FadeForSleep,                   /* 0xa9 */
	UC_GetPartyList2,                  /* 0xaa */
	UC_InCombat,                       /* 0xab */
	UC_IsWater,                        /* 0xac */
	UC_ResetConvFace,                  /* 0xad */
	(EngineCall) UC_SetCamera,         /* 0xae */
	UC_GetDeadParty,                   /* 0xaf */

	(EngineCall) UC_ViewTile,          /* 0xb0 */
	(EngineCall) UC_Telekinesis,       /* 0xb1 */
	UC_AOrAn,                          /* 0xb2 */
	UC_Polymorph,                      /* 0xb3 */
	UC_RevertSchedule,                 /* 0xb4 */
	UC_ChangeSchedule,                 /* 0xb5 */
	UC_NewSchedule,                    /* 0xb6 */
	UC_Schedule,                       /* 0xb7 */

	UC_GetTemperature,                 /* 0xb8 */
	UC_GetTemperatureZone,             /* 0xb9 */
	UC_SetTemperature,                 /* 0xba */
	UC_GetNpcWarmth,                   /* 0xbb */
	UC_GetNPCID,                       /* 0xbc */
	UC_SetNPCID,                       /* 0xbd */
	UC_GetEquip,                       /* 0xbe */
	UC_ApproachAvatar,                 /* 0xbf */

	(EngineCall) UC_SetBargeDir,       /* 0xc0 */
	UC_PathfindNPC,                    /* 0xc1 */
	UC_KeyringContains,                /* 0xc2 */
	(EngineCall) UC_KeyringAdd,        /* 0xc3 */
	(EngineCall) UC_RemoveItemsInArea, /* 0xc4 */
	(EngineCall) UC_SetInfravision     /* 0xc5 */
};
