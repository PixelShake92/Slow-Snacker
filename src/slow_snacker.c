#include "modding.h"
#include "functions.h"
#include "variables.h"

RECOMP_IMPORT("*", int recomp_printf(const char* fmt, ...));

// Adjust this value to control Snacker's speed
// 1.0 = normal, 0.5 = half speed, 0.25 = quarter speed
#define SNACKER_SPEED_MULTIPLIER 0.25f

// Only declare functions not in functions.h
extern f32 func_80309B24(f32[3]);
extern void func_80328FF0(Actor *, f32);
extern void func_80328FB0(Actor *, f32);
extern void mapSpecificFlags_setN(s32, s32, s32);
extern void func_80328CEC(Actor *, s32, s32, s32);
extern void func_8032CA80(Actor *, s32);
extern void func_8032BB88(Actor *, s32, s32);
extern void func_803297FC(Actor *, f32 *, f32 *);
extern void actor_setOpacity(Actor *, s32);

// Type definitions
typedef enum {
    CH_SNACKER_OPA_0_APPEAR,
    CH_SNACKER_OPA_1_ACTIVE,
    CH_SNACKER_OPA_2_FADE
} ChSnackerOpacityState;

typedef enum {
    CH_SNACKER_STATE_1 = 1,
    CH_SNACKER_STATE_2,
    CH_SNACKER_STATE_3,
    CH_SNACKER_STATE_4,
    CH_SNACKER_STATE_5_EATING,
    CH_SNACKER_STATE_6,
    CH_SNACKER_STATE_7,
    CH_SNACKER_STATE_8_HURT,
    CH_SNACKER_STATE_9_DEAD
} ChSnackerState;

typedef struct {
    s32 ctl;
    s32 opa;
    f32 unk8;
} ChSnackerLocal;

typedef enum {
    SNACKER_CTL_STATE_0_INACTIVE,
    SNACKER_CTL_STATE_1_RBB,
    SNACKER_CTL_STATE_2_TTC
} SnackerCtlState;

// External variables
extern s32 s_chSnacker_inRbb;
extern f32 s_chSnacker_respawnDelay_s;
extern s32 D_8037E630;

// External functions needed
extern SnackerCtlState snackerctl_get_state(void);
extern bool func_802E0DC0(f32[3]);
extern void func_802E0EC8(void);
extern void func_802E0CD0(Actor *);
extern void __chsnacker_start_dialog(Actor *);
extern void subaddie_set_state_with_direction(Actor *, s32, f32, s32);
extern void func_802E0E88(Actor *);
extern f32 func_802E10F0(f32);
extern void __chsnacker_ow(ActorMarker *, ActorMarker *);
extern void func_802E1010(ActorMarker *, ActorMarker *);
extern void __chsnacker_die(ActorMarker *, ActorMarker *);

RECOMP_PATCH void chsnacker_update(Actor *this) {
    f32 dt;
    ChSnackerLocal *local;
    SnackerCtlState controller_state;
    f32 player_position[3];
    f32 sp44;
    f32 sp40;

    dt = time_getDelta();
    local = (ChSnackerLocal *)&this->local;

    if (!this->initialized) {
        this->initialized = TRUE;
        this->marker->propPtr->unk8_3 = TRUE;
        this->unk138_25 = TRUE;
        this->unk154 = 0x085E0000;
        marker_setCollisionScripts(this->marker, __chsnacker_ow, func_802E1010, __chsnacker_die);
    }
    _player_getPosition(player_position);
    controller_state = snackerctl_get_state();
    
    if(func_802E0DC0(this->position) || ((controller_state != SNACKER_CTL_STATE_1_RBB) && (controller_state != SNACKER_CTL_STATE_2_TTC))) {
        local->unk8 = MIN(3.5, local->unk8 + dt);
        if (local->unk8 == 3.5) {
            func_802E0EC8();
        }
    } else {
        local->unk8 = 0.0f;
    }
    
    switch(this->state){
        case 1:
            if (subaddie_maybe_set_state_position_direction(this, 2, 0.0f, 1, 0.03f) != 0) {
                func_802E0CD0(this);
            }
            __chsnacker_start_dialog(this);
            break;

        case 2:
            // MODIFIED: Apply speed multiplier
            func_80328FB0(this, 3.0f * SNACKER_SPEED_MULTIPLIER);
            func_80328FF0(this, 3.0f * SNACKER_SPEED_MULTIPLIER);
            func_8032CA80(this, (s_chSnacker_inRbb) ? 15 : 9);
            if (func_80329480(this) != 0) {
                func_80328CEC(this, (s32) this->yaw, 0x5A, 0x96);
            }
            subaddie_maybe_set_state_position_direction(this, 1, 0.0f, 1, 0.0075f);
            __chsnacker_start_dialog(this);
            break;

        case 3:
            func_803297FC(this, &sp44, &sp40);
            this->yaw_ideal = sp40;
            this->unk6C = func_802E10F0(sp44);
            // MODIFIED: Apply speed multiplier
            func_80328FB0(this, 4.0f * SNACKER_SPEED_MULTIPLIER);
            func_80328FF0(this, 3.0f * SNACKER_SPEED_MULTIPLIER);
            if (func_80329480(this)) {
                subaddie_set_state_with_direction(this, 4, 0.0f, 1);
                this->actor_specific_1_f = 9.0f;
            }
            break;

        case 4:
            func_803297FC(this, &sp44, &sp40);
            this->yaw_ideal = sp40;
            this->unk6C = func_802E10F0(sp44);
            // MODIFIED: Apply speed multiplier
            func_80328FB0(this, (this->actor_specific_1_f / 2) * SNACKER_SPEED_MULTIPLIER);
            func_80328FF0(this, (this->actor_specific_1_f / 2) * SNACKER_SPEED_MULTIPLIER);
            // FIXED: Also slow down the acceleration rate!
            this->actor_specific_1_f = MIN(50.0 * SNACKER_SPEED_MULTIPLIER, this->actor_specific_1_f + (dt * SNACKER_SPEED_MULTIPLIER));
            func_8032CA80(this, (s_chSnacker_inRbb) ? 15 : 9);
            break;

        case CH_SNACKER_STATE_5_EATING:
            if (actor_animationIsAt(this, 0.25f)) {
                // FIXED: Removed 'this' parameter - only 2 args
                sfxsource_playSfxAtVolume(SFX_6D_CROC_BITE, 1.0f);
            }
            if (actor_animationIsAt(this, 0.99f)) {
                func_802E0CD0(this);
                func_80328CEC(this, (s32) this->yaw_ideal, 0x87, 0xAF);
                this->unk38_31 = 0x78;
                subaddie_set_state_with_direction(this, 2, 0.0f, 1);
                actor_loopAnimation(this);
            }
            func_8032CA80(this, (s_chSnacker_inRbb) ? 15 : 9);
            break;

        case CH_SNACKER_STATE_8_HURT:
            if (anctrl_isStopped(this->anctrl)) {
                func_802E0CD0(this);
                subaddie_set_state_with_direction(this, 2, 0.0f, 1);
                actor_loopAnimation(this);
            }
            break;

        case CH_SNACKER_STATE_9_DEAD:
            if (anctrl_isStopped(this->anctrl)) {
                s_chSnacker_respawnDelay_s = 60.0f;
                D_8037E630 = 0x63;
                func_802E0EC8();
            }
            break;
        default:
            break;
    }

    local = (ChSnackerLocal *)&this->local;
    switch(local->ctl){
        case CH_SNACKER_OPA_0_APPEAR:
            local->opa = MIN(0xFF, local->opa + 8);
            if(local->opa >= 0x81){
                this->marker->collidable = TRUE;
            }
            if (local->opa == 0xFF) {
                local->ctl = CH_SNACKER_OPA_1_ACTIVE;
            }
            break;

        case CH_SNACKER_OPA_1_ACTIVE:
            break;

        case CH_SNACKER_OPA_2_FADE:
            local->opa = MAX(0, local->opa - 8);
            if(local->opa < 0x80){
                this->marker->collidable = FALSE;
            }
            if (local->opa == 0) {
                marker_despawn(this->marker);
            }
            break;
    }

    // FIXED: Use actor_setOpacity function instead of unk60_0
    actor_setOpacity(this, local->opa);
    this->depth_mode = (255.0 == local->opa) ? MODEL_RENDER_DEPTH_FULL : MODEL_RENDER_DEPTH_COMPARE;
}