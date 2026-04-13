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

/*********************
 *      DEFINES
 *********************/

/** Maximum digits to display. E.g. uint32_t have 10 digits(4_294_967_296)*/
#define LV_NUMBERFLOW_MAX_DIGITS 10

/**********************
 *      TYPEDEFS
 **********************/

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
    int16_t anim_state;         /**< Animation value passing to draw function*/
    lv_obj_t *numberflow;
} _lv_nf_anim_state_t;

typedef struct {
    int32_t begin;
    int32_t curr;
    int32_t end;
} _lv_nf_anim_dsc_t;

typedef struct {
    int8_t last_num;       // 当前动画目标number

    lv_coord_t last_flow;   // 前一帧循环坐标，动态更新，范围同start_px

    _lv_nf_anim_dsc_t flow;
    _lv_nf_anim_dsc_t opa;
    _lv_nf_anim_dsc_t width;
} _lv_nf_slide_dsc_t;

typedef struct {
    lv_coord_t ofs_y;        /**< Pixels to move this digit's Y down*/
    int8_t target_num;         /**< New number of this digit*/
    lv_opa_t target_opa;        /**< New opacity of this digit*/
} _lv_nf_slide_start_dsc_t;

typedef struct {
    uint8_t modulus;                            /**< Modulus number of this digit*/
    _lv_nf_slide_dsc_t slide_dsc;               /**< Digit count of numbers*/
    _lv_nf_slide_start_dsc_t slide_start_dsc;   /**< New slide description to update*/
    _lv_nf_anim_state_t anim;
} _lv_nf_digit_t;

typedef struct {
    lv_obj_t obj;
    lv_anim_path_cb_t anim_path;

    const lv_numberflow_blur_data_t *blur_data;

    int32_t value;              /**< Last value set by lv_numberflow_set_value*/
    uint8_t digit_count;        /**< Total digit count allocated*/
    uint8_t visible_digit_cnt_prev;   /**< Visible digits after last number change*/
    _lv_nf_digit_t *digits;     /**< Array of digits descriptions, [0] is the one's place*/
    _lv_nf_digit_t *digits_ptr_static[LV_NUMBERFLOW_MAX_DIGITS];

    _lv_nf_anim_dsc_t x_ofs;
    _lv_nf_anim_dsc_t width;

    int16_t anim_state;         /**< Size animation state*/

    lv_coord_t height;          /**< Line height of font*/
    lv_coord_t line_space;
    lv_coord_t letter_space;
    lv_coord_t number_height;   /**< height + line_gap*/
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
 * Set a new animation path callback on the numberflow
 * @param obj       pointer to a numberflow object
 * @param path      new animation path callback
 */
void lv_numberflow_set_anim_path(lv_obj_t * obj, lv_anim_path_cb_t path);

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
lv_anim_path_cb_t lv_numberflow_get_anim_path(const lv_obj_t * obj);

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
