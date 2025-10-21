#include "modding.h"
#include "functions.h"
#include "variables.h"

RECOMP_IMPORT("*", int recomp_printf(const char* fmt, ...));

// Patches a function in the base game that's used to check if the player should tumble in the air.
RECOMP_PATCH bool player_isFallTumbling(void) {
    recomp_printf("Tumbling\n");
    /*s32 damage;

    if (player_isStable()) {
        return FALSE;
    }
    if (!bafalldamage_get_damage(&damage)) {
        return FALSE;
    }*/
    return TRUE;
}
