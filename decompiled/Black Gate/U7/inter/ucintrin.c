/* Black Gate U7.EXE, resident segment 21 (file offset 0x012912, 0 bytes): data but no code.
 * Borland C++ 2.0 -mm -O -G -P rebuilds its empty code segment as shipped.
 * Its data is DS:12A6-1502, between frameflg.c's and gtimer.c's, as link order places segment 21.
 */

#include "inter.h"
#include "uccomm1.h"
#include "uccomm11.h"
#include "uccomm13.h"
#include "uccomm14.h"
#include "uccomm2.h"
#include "uccomm3.h"
#include "uccomm4.h"
#include "uccomm5.h"
#include "uccomm6.h"
#include "uccomm7.h"
#include "uccomm8.h"
#include "uccomm9.h"

void far UC_GetItemFlag(Value *args, Value *ret);
void far UC_SetItemFlag(Value *args, Value *);
void far UC_ClearItemFlag(Value *args, Value *);
void far UC_HaltScheduled(Value *args);
void far UC_GetTime(Value *args, Value *ret);
void far UC_IsNPCNear(Value *args, Value *ret);

/* the engine calls, indexed by number */
EngineCall EngineCalls[ENGINE_CALL_COUNT] = {
	UC_Random,                            /* 0x00 */
	UC_Post,                              /* 0x01 */
	UC_PostInFuture,                      /* 0x02 */
	UC_SetSpeaker,                        /* 0x03 */
	UC_CloseSpeaker,                      /* 0x04 */
	UC_TurnOn,                            /* 0x05 */
	UC_TurnOff,                           /* 0x06 */
	UC_PushKeys,                          /* 0x07 */

	UC_PopKeys,                           /* 0x08 */
	UC_ClearKeys,                         /* 0x09 */
	UC_GetInput,                          /* 0x0a */
	UC_GetOrdInput,                       /* 0x0b */
	UC_InputNumericValue,                 /* 0x0c */
	UC_SetItemShape,                      /* 0x0d */
	UC_FindNearest,                       /* 0x0e */
	(EngineCall) UC_Sfx,                  /* 0x0f */

	UC_DieRoll,                           /* 0x10 */
	UC_GetItemShape,                      /* 0x11 */
	UC_GetItemFrame,                      /* 0x12 */
	(EngineCall) UC_SetItemFrame,         /* 0x13 */
	UC_GetQuality,                        /* 0x14 */
	UC_SetQuality,                        /* 0x15 */
	UC_GetItemQuantity,                   /* 0x16 */
	UC_SetQuantity,                       /* 0x17 */

	UC_GetCoord,                          /* 0x18 */
	UC_GetDist,                           /* 0x19 */
	UC_FindDirection,                     /* 0x1a */
	UC_GetNpcObject,                      /* 0x1b */
	UC_GetWorkType,                       /* 0x1c */
	UC_SetWorkType,                       /* 0x1d */
	UC_JoinParty,                         /* 0x1e */
	UC_LeaveParty,                        /* 0x1f */

	UC_GetNpcProp,                        /* 0x20 */
	UC_SetNpcProp,                        /* 0x21 */
	UC_GetAvatarRef,                      /* 0x22 */
	UC_GetPartyList,                      /* 0x23 */
	UC_CreateNewObject,                   /* 0x24 */
	UC_SetLastCreated,                    /* 0x25 */
	UC_PopToMap,                          /* 0x26 */
	UC_GetNPCName,                        /* 0x27 */

	UC_CountObjects,                      /* 0x28 */
	UC_FindObject,                        /* 0x29 */
	UC_GetContItems,                      /* 0x2a */
	UC_RemovePartyItems,                  /* 0x2b */
	UC_AddPartyItems,                     /* 0x2c */
	UC_GetMusicTrack,                     /* 0x2d */
	(EngineCall) UC_PlayMusic,            /* 0x2e */
	UC_IsNPCNear,                         /* 0x2f */

	UC_FindNearbyAvatar,                  /* 0x30 */
	UC_IsNpc,                             /* 0x31 */
	UC_DisplayRunes,                      /* 0x32 */
	UC_GetTarget,                         /* 0x33 */
	UC_ErrorMessage,                      /* 0x34 */
	UC_FindNearby,                        /* 0x35 */
	UC_PopToNPC,                          /* 0x36 */
	UC_IsDead,                            /* 0x37 */

	UC_GetHour,                           /* 0x38 */
	UC_GameMinute,                        /* 0x39 */
	UC_GetNpcNumber,                      /* 0x3a */
	UC_GetTime,                           /* 0x3b */
	UC_GetAlignment,                      /* 0x3c */
	(EngineCall) UC_SetAlignment,         /* 0x3d */
	(EngineCall) UC_MoveObject,           /* 0x3e */
	(EngineCall) UC_RemoveNpc,            /* 0x3f */

	UC_ItemSay,                           /* 0x40 */
	UC_SetToAttack,                       /* 0x41 */
	UC_GetLift,                           /* 0x42 */
	(EngineCall) UC_SetLift,              /* 0x43 */
	UC_GetWeather,                        /* 0x44 */
	UC_SetWeather,                        /* 0x45 */
	UC_SitDown,                           /* 0x46 */
	UC_Summon,                            /* 0x47 */

	UC_DisplayMap,                        /* 0x48 */
	(EngineCall) UC_KillNpc,              /* 0x49 */
	UC_RollToWin,                         /* 0x4a */
	UC_SetAttackMode,                     /* 0x4b */
	UC_SetOppressor,                      /* 0x4c */
	(EngineCall) UC_Clone,                /* 0x4d */
	(EngineCall) UC_DoNothing,                /* 0x4e */
	(EngineCall) UC_DisplayArea,          /* 0x4f */

	(EngineCall) UC_WizardEye,            /* 0x50 */
	UC_Resurrect,                         /* 0x51 */
	UC_AddSpell,                          /* 0x52 */
	(EngineCall) UC_SpriteEffect,         /* 0x53 */
	UC_AttackObject,                      /* 0x54 */
	UC_BookMode,                          /* 0x55 */
	(EngineCall) UC_StopTime,             /* 0x56 */
	(EngineCall) UC_CauseLight,           /* 0x57 */

	UC_GetBarge,                          /* 0x58 */
	(EngineCall) UC_Earthquake,           /* 0x59 */
	UC_AvatarSex,                         /* 0x5a */
	(EngineCall) UC_Armageddon,           /* 0x5b */
	(EngineCall) UC_HaltScheduled,        /* 0x5c */
	(EngineCall) UC_Lightning,            /* 0x5d */
	UC_GetArraySize,                      /* 0x5e */
	(EngineCall) UC_MarkVirtueStone,      /* 0x5f */

	(EngineCall) UC_RecallVirtueStone,    /* 0x60 */
	UC_ApplyDamage,                       /* 0x61 */
	UC_IsPcInside,                        /* 0x62 */
	(EngineCall) UC_SetOrrery,            /* 0x63 */
	(EngineCall) UC_SetPaletteFadedIn,    /* 0x64 */
	UC_GetClock,                          /* 0x65 */
	(EngineCall) UC_SetClock,             /* 0x66 */
	UC_WearingFellowship,                 /* 0x67 */

	UC_MouseExists,                       /* 0x68 */
	UC_GetSpeechTrack,                    /* 0x69 */
	UC_FlashMouse,                        /* 0x6a */
	UC_GetItemFrameRot,                   /* 0x6b */
	(EngineCall) UC_SetItemFrameRot,      /* 0x6c */
	UC_OnBarge,                           /* 0x6d */
	UC_GetContainer,                      /* 0x6e */
	(EngineCall) UC_RemoveItem,           /* 0x6f */

	(EngineCall) UC_RestoreMouseCursor,   /* 0x70 */
	(EngineCall) UC_ReduceHealth,         /* 0x71 */
	UC_IsReadied,                         /* 0x72 */
	(EngineCall) UC_RestartGame,          /* 0x73 */
	UC_StartSpeech,                       /* 0x74 */
	(EngineCall) UC_RunEndgame,           /* 0x75 */
	UC_FireProjectile,                    /* 0x76 */
	UC_NapTime,                           /* 0x77 */

	UC_AdvanceTime,                       /* 0x78 */
	UC_InUsecode,                         /* 0x79 */
	UC_CallGuards,                        /* 0x7a */
	(EngineCall) UC_ObjSpriteEffect,      /* 0x7b */
	UC_AttackAvatar,                      /* 0x7c */
	UC_PathRunUsecode,                    /* 0x7d */
	(EngineCall) UC_CloseGumps,           /* 0x7e */
	(EngineCall) UC_ItemSay2,             /* 0x7f */

	UC_CloseGump,                         /* 0x80 */
	UC_InGumpMode,                        /* 0x81 */
	(EngineCall) UC_SetLight,             /* 0x82 */
	(EngineCall) UC_ResetPalette,         /* 0x83 */
	(EngineCall) UC_SetTimePalette,       /* 0x84 */
	UC_IsNotBlocked,                      /* 0x85 */
	(EngineCall) UC_PlaySoundEffect2,     /* 0x86 */
	UC_DirectionFrom,                     /* 0x87 */

	UC_GetItemFlag,                       /* 0x88 */
	UC_SetItemFlag,                       /* 0x89 */
	UC_ClearItemFlag,                     /* 0x8a */
	UC_SetPathFailure,                    /* 0x8b */
	UC_FadePalette,                       /* 0x8c */
	UC_GetPartyList2,                     /* 0x8d */
	UC_InCombat,                          /* 0x8e */
	UC_StartBlockingSpeech,               /* 0x8f */

	UC_IsWater,                           /* 0x90 */
	UC_ResetConvFace,                     /* 0x91 */
	(EngineCall) UC_SetCamera,            /* 0x92 */
	UC_GetDeadParty,                      /* 0x93 */
	(EngineCall) UC_ViewTile,             /* 0x94 */
	(EngineCall) UC_Telekinesis,          /* 0x95 */
	UC_AOrAn                              /* 0x96 */
};
