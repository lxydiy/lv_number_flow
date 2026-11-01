#ifndef NUMBERFLOW_CURVES_H
#define NUMBERFLOW_CURVES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Numberflow animation start value. (Not the real value just indicates process animation)*/
#define NUMBERFLOW_ANIM_STATE_START 0

/** Numberflow animation end value.  (Not the real value just indicates process animation)*/
#define NUMBERFLOW_ANIM_STATE_END   16384

/** log2(LV_NUMBERFLOW_ANIM_STATE_END) used to normalize data*/
#define NUMBERFLOW_ANIM_STATE_NORM  14

/** @note elapsed/duration are milliseconds; the result is Q14 progress (16384 at the end). */
typedef int32_t (*numberflow_anim_path_t)(uint32_t elapsed, uint32_t duration);

int32_t numberflow_anim_path_flow(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_ease_out(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_linear(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_ease_in(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_ease_in_out(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_overshoot(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_bounce(uint32_t elapsed, uint32_t duration);
int32_t numberflow_anim_path_step(uint32_t elapsed, uint32_t duration);

#ifdef __cplusplus
}
#endif
#endif
