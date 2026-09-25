#include "modding.h"
#include "ultra64.h"
#include "enums.h"
#include "common_structs.h"
#include "recompconfig.h"

typedef struct {
    Actor* unk0;
    s32 unk4;
} GlobalASMStruct53;

typedef struct {
    s32 unk0;
    s32 unk4;
    s16 unk8;
    s16 barrel_index;
    u8 unkC;
    u8 destroy_timer;
} BonusAAD;

extern GlobalASMStruct53 D_global_asm_807FB930[];
extern u16 D_global_asm_807FBB34;
int gameIsInDKTVMode(void);
extern Maps current_map;
void func_global_asm_8067DCC0(void);
s16 func_global_asm_8073195C(s16 arg0);
u8 func_global_asm_80680908(void);
void func_global_asm_806A5DF0(s16 arg0, f32 x, f32 y, f32 z, s16 arg4, u8 arg5, s16 arg6, s32 arg7);
s32 deleteActor(Actor*);
void playSong(MUSIC_E arg0, f32 arg1);
s16 func_global_asm_80688E68(Actor *arg0);

void completeBonus(Actor *bonus) {
    BonusAAD *aad;

    if ((bonus->control_state != 0) && (bonus->control_state != 0x15)) return;
    if (gameIsInDKTVMode()) return;
    switch (bonus->control_state) {
        case 0:
        case 0x15:
            bonus->control_state = 12;
            bonus->control_state_progress = 0;
            bonus->noclip_byte = 1;
            aad = bonus->AAD_as_array[0];
            aad->destroy_timer = 3;
            break;
    }
}

u8 getRewardSpawnType(Actor *bonus) {
    u8 phi_v1 = 0x64;
    switch (current_map) {
        case MAP_FACTORY:
            if (func_global_asm_80688E68(bonus) == 0xD) {
                phi_v1 = 2;
            }
            break;
        case MAP_FUNGI_DK_BARN:
            if (func_global_asm_80688E68(bonus) == 3) {
                phi_v1 = 2;
            }
            break;
        case MAP_AZTEC_LOBBY:
        case MAP_DK_ISLES_SNIDES_ROOM:
            phi_v1 = 2;
            break;
        default:
            break;
    }
    return phi_v1;
}

void handleSpawning(Actor *bonus) {
    BonusAAD *aad;
    s16 flag;
    u8 spawn_type;
    f32 spawn_delta;

    if (gameIsInDKTVMode()) return;
    if (bonus->control_state != 12) return;
    aad = bonus->AAD_as_array[0];
    if (aad->destroy_timer != 1) return;
    if (recomp_get_config_u32("show_explosion")) {
        func_global_asm_8067DCC0();
    }
    flag = -1;
    if (aad->barrel_index) {
        flag = func_global_asm_8073195C(aad->barrel_index);
    }
    spawn_type = getRewardSpawnType(bonus);
    spawn_delta = -8.0f;
    if (current_map == MAP_AZTEC_LOBBY) {
        spawn_delta = 50.0f;
    }
    func_global_asm_806A5DF0(0x2D, bonus->x_position, bonus->y_position + spawn_delta, bonus->z_position, 0, spawn_type, flag, 0);
    if (recomp_get_config_u32("play_oh_banana")) {
        playSong(MUSIC_20_OH_BANANA, 1.0f);
    }
    deleteActor(bonus);
}

RECOMP_CALLBACK("*", dk64recomp_every_frame)
void autocompleteLoop(void) {
    s32 i;
    Actor *ac;
    
    for (i = 0; i < D_global_asm_807FBB34; i++) {
        ac = D_global_asm_807FB930[i].unk0;
        if (ac) {
            if (ac->unk58 == ACTOR_BONUS_BARREL) {
                completeBonus(ac);
                handleSpawning(ac);
            }
        }
    }
}

typedef struct character_progress {
    u8 moves; // at 0x00
    u8 simian_slam; // at 0x01
    u8 weapon; // at 0x02, bitfield, xxxxxshw
    u8 ammo_belt; // at 0x03, see ScriptHawk's Game.getMaxStandardAmmo() for formula
    u8 instrument; // at 0x04, bitfield, xxxx321i
    u8 unk5;
    u16 coins; // at 0x06
    u16 instrument_ammo; // at 0x08, also used as lives in multiplayer
    u16 coloured_bananas[14]; // TODO: Better datatype?
    u16 coloured_bananas_fed_to_tns[14]; // TODO: Better datatype?
    u16 golden_bananas[14]; // TODO: Better datatype?
} CharacterProgress;

typedef struct PlayerProgress {
    union {
        CharacterProgress character_progress[6]; // 0x5E * 6 (5 Kongs + Krusha)
        u8 character_progress_as_bytes[6][0x5E]; // Note: Can't use sizeof(CharacterProgress) because mips_to_c can't do struct maths yet
        u16 character_progress_as_shorts[6][0x2F]; // Note: Can't use sizeof(CharacterProgress) because mips_to_c can't do struct maths yet
    };
    u8 unk234[0x2F0 - 0x234];
    u16 standardAmmo; // 0x2F0
    u16 homingAmmo; // 0x2F2
    u16 oranges; // 0x2F4
    u16 crystals; // 0x2F6 // Note: Multiplied by 150 compared to on screen counter
    u16 film; // 0x2F8
    s8 unk2FA;
    s8 health; // 0x2FB
    u8 melons; // 0x2FC
    s8 unk2FD; // Something to do with health... hmm
    u16 unk2FE[(0x306 - 0x2FE) / 2];
} PlayerProgress;

extern PlayerProgress D_global_asm_807FC950[4];
extern s32 D_global_asm_807552EC;
void func_multiplayer_80026E20(u8 playerIndex, s8 arg1);
void func_global_asm_8060E7EC(u8 arg0, u8 arg1, s32 arg2);
extern s32 D_global_asm_807FBB64;
extern u8 cc_number_of_players;

// When many GBs spawn simultaneously, it can result in the damage overflowing, giving the player 0 health
RECOMP_PATCH s32 func_global_asm_806C9974(u8 playerIndex, s8 arg1) {
    CharacterChange *temp2 = &character_change_array[playerIndex];
    PlayerProgress *temp = &D_global_asm_807FC950[playerIndex];

    temp2->unk2DC.unk6 |= 0x11;
    if ((cc_number_of_players >= 2) && (arg1 < 0) && (D_global_asm_807552EC == 1)) {
        func_multiplayer_80026E20(playerIndex, arg1);
    }
    if (arg1 < 0) {
        func_global_asm_8060E7EC(playerIndex, 0xFF, 5);
    }
    if (!(D_global_asm_807FBB64 & 0x800)) {
        temp->unk2FD += arg1;
        temp->unk2FD = MIN(temp->unk2FD, 12); // Always ensure this never goes above 12
    }
    if ((temp->health + temp->unk2FD) <= 0) {
        temp2->unk2DC.unk6 |= 0xC0;
        return TRUE;
    } else {
        return FALSE;
    }
}