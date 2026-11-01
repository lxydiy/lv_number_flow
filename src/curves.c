#include "curves.h"

/** maximum value in number_flow_ease_lut*/
#define NUMBERFLOW_ANIM_RESOLUTION 16384

/** log2(LV_NUMBERFLOW_ANIM_RESOLUTION) used to normalize data*/
#define NUMBERFLOW_ANIM_RES_SHIFT 14

/** Element count of number_flow_ease_lut*/
#define NUMBERFLOW_ANIM_LUT_SIZE 90

/**********************
 *  STATIC VARIABLES
 **********************/
/** To be more consistent with the original animation of NumberFlow, a custom
 *  animation curve is used. This animation curve below is mapped from:
 *  https://github.com/barvian/number-flow/blob/7ab1a7164ef1b088267f789b07581fefef064b68/packages/number-flow/src/lite.ts#L73 */
static const uint16_t number_flow_ease_lut[NUMBERFLOW_ANIM_LUT_SIZE] = {
      0,     82,    311,    639,   1081,   1573,   2114,   2703,   3310,   3932,   4555,   5177,   5800,   6390,   6980,   7553,
   8094,   8618,   9126,   9601,  10060,  10486,  10895,  11289,  11649,  11977,  12304,  12599,  12878,  13140,  13386,  13615,
  13828,  14025,  14205,  14369,  14533,  14680,  14811,  14942,  15057,  15155,  15254,  15352,  15434,  15516,  15581,  15647,
  15712,  15761,  15811,  15860,  15909,  15942,  15991,  16024,  16056,  16073,  16105,  16122,  16155,  16171,  16187,  16204,
  16220,  16237,  16253,  16253,  16269,  16286,  16286,  16302,  16302,  16318,  16318,  16323,  16330,  16333,  16338,  16343,
  16346,  16350,  16353,  16355,  16358,  16359,  16363,  16364,  16366,  16384
};

/*Simple integer linear interpolator for animation curve*/
int32_t numberflow_anim_path_flow(uint32_t elapsed, uint32_t duration)
{
    if (elapsed >= duration) return NUMBERFLOW_ANIM_STATE_END;
    /*Calculate the current step*/
    uint32_t t = (uint32_t)((uint64_t)elapsed * NUMBERFLOW_ANIM_RESOLUTION / duration);

    uint32_t pos = (uint32_t)t * (NUMBERFLOW_ANIM_LUT_SIZE - 1);

    uint32_t index = pos / NUMBERFLOW_ANIM_RESOLUTION;
    uint32_t frac  = pos % NUMBERFLOW_ANIM_RESOLUTION;

    uint32_t a = number_flow_ease_lut[index];
    uint32_t b = number_flow_ease_lut[index + 1];

    int32_t step = a + (((b - a) * frac) >> NUMBERFLOW_ANIM_RES_SHIFT);

    return step;
}

static uint32_t bezier_step(uint32_t t, uint32_t first, uint32_t second,
                            uint32_t third, uint32_t last)
{
    uint32_t remaining = 1024u - t;
    uint32_t remaining2 = remaining * remaining >> 10;
    uint32_t remaining3 = remaining2 * remaining >> 10;
    uint32_t t2 = t * t >> 10;
    uint32_t t3 = t2 * t >> 10;
    return (remaining3 * first >> 10) +
           (3u * remaining2 * t * second >> 20) +
           (3u * remaining * t2 * third >> 20) + (t3 * last >> 10);
}

static int32_t path_bezier(uint32_t elapsed, uint32_t duration,
                           uint32_t second, uint32_t third)
{
    if (elapsed >= duration) return NUMBERFLOW_ANIM_STATE_END;
    uint32_t t = (uint32_t)((uint64_t)elapsed * 1024u / duration);
    uint32_t step = bezier_step(t, 0, second, third, 1024);
    return (int32_t)(step * NUMBERFLOW_ANIM_STATE_END >> 10);
}

int32_t numberflow_anim_path_ease_out(uint32_t elapsed, uint32_t duration)
{
    return path_bezier(elapsed, duration, 900, 950);
}

int32_t numberflow_anim_path_ease_in(uint32_t elapsed, uint32_t duration)
{
    return path_bezier(elapsed, duration, 50, 100);
}

int32_t numberflow_anim_path_ease_in_out(uint32_t elapsed, uint32_t duration)
{
    return path_bezier(elapsed, duration, 50, 952);
}

int32_t numberflow_anim_path_overshoot(uint32_t elapsed, uint32_t duration)
{
    return path_bezier(elapsed, duration, 1000, 1300);
}

int32_t numberflow_anim_path_step(uint32_t elapsed, uint32_t duration)
{
    return elapsed >= duration ? NUMBERFLOW_ANIM_STATE_END : NUMBERFLOW_ANIM_STATE_START;
}

int32_t numberflow_anim_path_bounce(uint32_t elapsed, uint32_t duration)
{
    if (elapsed >= duration) return NUMBERFLOW_ANIM_STATE_END;
    int32_t t = (int32_t)((uint64_t)elapsed * 1024u / duration);
    uint32_t distance = NUMBERFLOW_ANIM_STATE_END;
    if (t < 408) {
        t = t * 2500 >> 10;
    }
    else if (t < 614) {
        t = 1024 - (t - 408) * 5;
        distance /= 20u;
    }
    else if (t < 819) {
        t = (t - 614) * 5;
        distance /= 20u;
    }
    else if (t < 921) {
        t = 1024 - (t - 819) * 10;
        distance /= 40u;
    }
    else {
        t = (t - 921) * 10;
        distance /= 40u;
    }
    if (t > 1024)
        t = 1024;
    if (t < 0)
        t = 0;
    uint32_t step = bezier_step(t, 1024, 800, 500, 0);
    return NUMBERFLOW_ANIM_STATE_END - (int32_t)(step * distance >> 10);
}

int32_t numberflow_anim_path_linear(uint32_t elapsed, uint32_t duration)
{
    if (elapsed >= duration) return NUMBERFLOW_ANIM_STATE_END;
    uint32_t step = (uint32_t)((uint64_t)elapsed * 1024u / duration);
    return (int32_t)(step * NUMBERFLOW_ANIM_STATE_END >> 10);
}
