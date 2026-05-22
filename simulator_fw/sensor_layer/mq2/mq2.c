/**
 * @file    mq2.c
 * @brief   MQ-2 Pure Logic Simulation implementation.
 */

#include "mq2.h"

/* Private function prototypes ---------------------------------------------*/

static uint16_t prv_scene_to_mv(mq2_scene_t scene);

/* Public functions --------------------------------------------------------*/

void mq2_reset(mq2_t *dev)
{
    if (dev == NULL) return;
    dev->target_mv             = prv_scene_to_mv(MQ2_SCENE_CLEAN_AIR);
    dev->current_mv            = dev->target_mv;
    dev->slew_rate_mv_per_tick = 50u; /* Default slew rate */
}

void mq2_set_scene(mq2_t *dev, mq2_scene_t scene)
{
    if (dev == NULL) return;
    dev->target_mv = prv_scene_to_mv(scene);
}

void mq2_tick(mq2_t *dev)
{
    if (dev == NULL) return;

    if (dev->current_mv < dev->target_mv)
    {
        if (dev->current_mv + dev->slew_rate_mv_per_tick > dev->target_mv)
            dev->current_mv = dev->target_mv;
        else
            dev->current_mv += dev->slew_rate_mv_per_tick;
    }
    else if (dev->current_mv > dev->target_mv)
    {
        if (dev->current_mv < dev->slew_rate_mv_per_tick + dev->target_mv)
            dev->current_mv = dev->target_mv;
        else
            dev->current_mv -= dev->slew_rate_mv_per_tick;
    }
}

uint16_t mq2_get(mq2_t *dev)
{
    return (dev != NULL) ? dev->current_mv : 0u;
}

/* Private functions -------------------------------------------------------*/

static uint16_t prv_scene_to_mv(mq2_scene_t scene)
{
    switch (scene)
    {
        case MQ2_SCENE_CLEAN_AIR: return 400u;
        case MQ2_SCENE_SMOKE:     return 2500u;
        case MQ2_SCENE_LPG:       return 3000u;
        case MQ2_SCENE_METHANE:   return 2800u;
        case MQ2_SCENE_ALCOHOL:   return 1500u;
        default:                  return 400u;
    }
}
