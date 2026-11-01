/**
 * @file lv_numberflow.h
 *
 */

#ifndef LV_NUMBERFLOW_H
#define LV_NUMBERFLOW_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#include "numberflow.h"

#ifndef LV_NUMBERFLOW_MAX_DIGITS
#define LV_NUMBERFLOW_MAX_DIGITS NUMBERFLOW_MAX_DIGITS
#elif LV_NUMBERFLOW_MAX_DIGITS != NUMBERFLOW_MAX_DIGITS
#error "LV_NUMBERFLOW_MAX_DIGITS and NUMBERFLOW_MAX_DIGITS must match"
#endif
typedef numberflow_mode_t lv_numberflow_mode_t;
typedef numberflow_dir_t lv_numberflow_dir_t;
typedef numberflow_flag_t lv_numberflow_flag_t;
#define LV_NUMBERFLOW_FLAG_TRIM_ZEROS NUMBERFLOW_FLAG_TRIM_ZEROS
#define LV_NUMBERFLOW_FLAG_FADE_VERTICAL NUMBERFLOW_FLAG_FADE_VERTICAL
#define LV_NUMBERFLOW_FLAG_TIGHT NUMBERFLOW_FLAG_TIGHT
#define LV_NUMBERFLOW_FLAG_SHOW_POSITIVE NUMBERFLOW_FLAG_SHOW_POSITIVE
#define LV_NUMBERFLOW_MODE_NORMAL NUMBERFLOW_MODE_NORMAL
#define LV_NUMBERFLOW_MODE_REVERSE NUMBERFLOW_MODE_REVERSE
#define LV_NUMBERFLOW_DIR_DEFAULT NUMBERFLOW_DIR_DEFAULT
#define LV_NUMBERFLOW_DIR_CONTINUOUS NUMBERFLOW_DIR_CONTINUOUS
#define LV_NUMBERFLOW_DIR_NEAREST NUMBERFLOW_DIR_NEAREST
#define LV_NUMBERFLOW_DIR_INCREASE NUMBERFLOW_DIR_INCREASE
#define LV_NUMBERFLOW_DIR_DECREASE NUMBERFLOW_DIR_DECREASE

typedef struct {
    int16_t ofs_x;              /**< X offset from origin to image top-left*/
    int16_t ofs_y;              /**< Y offset from origin to image top-left*/
    uint16_t box_w;             /**< image width*/
    uint16_t box_h;             /**< image height*/
    uint32_t start;             /**< Start offset of glyph blob*/
} lv_nf_glyph_blur_dsc_t;

typedef struct {
    const lv_nf_glyph_blur_dsc_t* blurs;    /**< Blurs of this character*/
    int16_t x_adv[10];                      /**< Draw next character after this distance*/
    const uint8_t* glyph_blob;
    uint8_t blur_level;
    uint16_t line_height;
} lv_numberflow_blur_data_t;

typedef struct {
    lv_obj_t obj;
    numberflow_t core;
    const lv_font_t *font;
    const lv_numberflow_blur_data_t *blur_data;
    int16_t x_adv[10];          /**< character x_adv cache*/
    lv_timer_t *timer;
} lv_numberflow_t;

extern const lv_obj_class_t lv_numberflow_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a numberflow object
 * @param parent    pointer to an object, it will be the parent of the new numberflow
 * @return          pointer to the created numberflow
 */
lv_obj_t * lv_numberflow_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set a new value on the numberflow
 * @param obj       pointer to a numberflow object
 * @param value     new value
 * @param anim      LV_ANIM_ON: set the value with an animation; LV_ANIM_OFF: change the value immediately
 */
void lv_numberflow_set_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim);

/**
 * Set the mode of numberflow
 * @param obj       pointer to a numberflow object
 * @param mode      numberflow mode from ::lv_numberflow_mode_t
 */
void lv_numberflow_set_mode(lv_obj_t * obj, lv_numberflow_mode_t mode);

/**
 * Set the direction of numberflow
 * @param obj       pointer to a numberflow object
 * @param dir       direction from ::lv_numberflow_dir_t
 */
void lv_numberflow_set_dir(lv_obj_t * obj, lv_numberflow_dir_t dir);

/**
 * Set the digit format of numberflow
 * @param obj       pointer to a numberflow object
 * @param int_fix   minimal count of number of integer part. If 0, will not limit
 * the minimal length
 * @param dec_fix   the count of number of decimal part. If 0, decimal point is not
 * shown
 */
void lv_numberflow_set_format(lv_obj_t * obj, uint8_t int_fix, uint8_t dec_fix);

/**
 * Set the separators of numberflow
 * @param obj       pointer to a numberflow object
 * @param ksep      thousand separator. Can be '\0' if not used
 * @param dsep      decimal separator. Can't be '\0'. Set dec_fix to 0 if you
 * don't need decimal part
 */
void lv_numberflow_set_separators(lv_obj_t * obj, char ksep, char dsep);

/**
 * Set the modulus of digit
 * @param obj       pointer to a numberflow object
 * @param pos       digit position expressed as an exponent of 10 (e.g., 2 for
 * hundreds, 0 for units, -1 for tenths)
 * @param modulus   modulus of this digit. Should not exceed 10
 */
void lv_numberflow_set_modulus(lv_obj_t * obj, int8_t pos, uint8_t modulus);

/**
 * Set one or more flags for numberflow
 * @param obj       pointer to a numberflow object
 * @param flags     OR-ed values of ::lv_numberflow_flag_t to set
 */
void lv_numberflow_add_flag(lv_obj_t * obj, lv_numberflow_flag_t flags);

/**
 * Clear one or more flags for numberflow
 * @param obj   pointer to an object
 * @param flags     OR-ed values from ::lv_numberflow_flag_t to clear.
 */
void lv_numberflow_clear_flag(lv_obj_t * obj, lv_numberflow_flag_t flags);

/**
 * Set a new animation path callback for numbers flowing
 * @param obj       pointer to a numberflow object
 * @param path      new animation path, NULL for default path
 */
void lv_numberflow_set_anim_path_flow(lv_obj_t * obj, numberflow_anim_path_t path);

/**
 * Set a new animation path callback for size transition
 * @param obj       pointer to a numberflow object
 * @param path      new animation path, NULL for default path
 */
void lv_numberflow_set_anim_path_size(lv_obj_t * obj, numberflow_anim_path_t path);

/**
 * Set the blur data of the numberflow.
 * @param obj       pointer to a numberflow object
 * @param blur_data pointer to the blur data. if NULL, will use font in style to render numbers.
 */
void lv_numberflow_set_blur_data(lv_obj_t * obj, const lv_numberflow_blur_data_t * blur_data);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the value of a numberflow
 * @param obj       pointer to a numberflow object
 * @return          the value of the numberflow
 */
int32_t lv_numberflow_get_value(const lv_obj_t * obj);

/**
 * Get the animation path callback of a numberflow
 * @param obj       pointer to a numberflow object
 * @return          the animation path callback of the numberflow
 */
numberflow_anim_path_t lv_numberflow_get_anim_path(const lv_obj_t * obj);
numberflow_anim_path_t lv_numberflow_get_anim_path_flow(const lv_obj_t * obj);
numberflow_anim_path_t lv_numberflow_get_anim_path_size(const lv_obj_t * obj);

/**
 * Get the blur data of a numberflow
 * @param obj       pointer to a numberflow object
 * @return          pointer to the blur data of the numberflow
 */
const lv_numberflow_blur_data_t * lv_numberflow_get_blur_data(const lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_NUMBERFLOW_H*/
