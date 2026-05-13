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

enum {
    LV_NUMBERFLOW_MODE_NORMAL,          /** flow from bottom to top means increasing*/
    LV_NUMBERFLOW_MODE_REVERSE,         /** same as normal but flow down for increasing*/
};
typedef uint8_t lv_numberflow_mode_t;

enum {
    LV_NUMBERFLOW_DIR_DEFAULT,          /** rolling direction follows the overall trend of value changes, but doesn't consider the relationship between digits*/
    LV_NUMBERFLOW_DIR_CONTINUOUS,       /** makes the number transitions appear to pass through in-between numbers*/
    LV_NUMBERFLOW_DIR_NEAREST,          /** digits flow to the direction of the shortest flow distance*/
    LV_NUMBERFLOW_DIR_INCREASE,         /** force every digit to flow to larger number and loop if needed*/
    LV_NUMBERFLOW_DIR_DECREASE          /** force every digit to flow to smaller number and loop if needed*/
};
typedef uint8_t lv_numberflow_dir_t;

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
    int32_t anim_state;         /**< Animation value passing to draw function*/
    bool updated;               /**< Animation update flag to recalculate pos and blur*/
    lv_obj_t *numberflow;
} _lv_nf_anim_state_t;

typedef struct {
    int32_t begin;
    int32_t curr;
    int32_t end;
} _lv_nf_anim_dsc_t;

typedef struct {
    lv_coord_t ofs_y;           /**< Pixels to move this digit's Y down*/
    int8_t target_num;          /**< New number of this digit*/
    lv_opa_t target_opa;        /**< New opacity of this digit*/
} _lv_nf_slide_start_dsc_t;

typedef struct {
    int8_t num;
    lv_coord_t x;
    lv_coord_t y;
    uint16_t curr_width;
    lv_opa_t opa;
    uint8_t blur_level;
} _lv_nf_number_draw_dsc_t;

typedef struct {
    uint8_t modulus;            /**< Modulus number of this digit*/

    int8_t last_num;            /**< Number in the center when the animation ends*/

    lv_coord_t last_flow;       /**< Last frame's flow position, used to keep motion blur after animation update*/

    _lv_nf_anim_dsc_t flow;     /**< Flow position animation*/
    _lv_nf_anim_dsc_t opa;      /**< Opacity animation*/
    _lv_nf_anim_dsc_t width;    /**< Width animation for non-monospace fonts to flow smoothly*/

    _lv_nf_slide_start_dsc_t slide_start_dsc;   /**< New slide description to update*/
    _lv_nf_anim_state_t anim;   /**< Digit animation state*/

    _lv_nf_number_draw_dsc_t draw[2];
} _lv_nf_digit_t;

typedef struct {
    lv_obj_t obj;
    lv_numberflow_mode_t mode;
    lv_numberflow_dir_t dir;
    lv_anim_path_cb_t anim_path_flow;
    lv_anim_path_cb_t anim_path_size;
    int8_t integer_fix;           /**< Number of digits before the decimal point. 0 means dynamic length*/
    int8_t decimal_fix;           /**< Number of digits after the decimal point. 0 means no decimal part*/
    bool trim_decimal;            /**< Whether to trim trailing zeros after decimal point*/
    char ksep;                    /**< Thousand separator to use, '\0' means no separator*/
    char dsep;                    /**< Decimal point to use, can't be '\0'*/
    bool fade;                    /**< Change the opacity of the upper and lower numbers during the scrolling of numbers*/
    const lv_numberflow_blur_data_t *blur_data;

    int32_t value;                  /**< Last value, used for comparison*/
    int8_t digit_count;             /**< Count of allocated digits*/
    int8_t int_cnt;                 /**< Count of visible integer digits*/
    int8_t dec_cnt;                 /**< Count of visible decimal digits*/
    _lv_nf_digit_t *digits;         /**< Array of digit descriptors*/
    int8_t ones_place_idx;          /**< Index of the one's place in digits array*/

    _lv_nf_anim_dsc_t x_ofs;    /**< X offset animation of widget*/
    _lv_nf_anim_dsc_t width;    /**< Width animation of widget*/

    int32_t anim_state;         /**< Size animation state*/

    const lv_font_t *font;
    lv_coord_t height;          /**< The font line height. Also the content height of the widget*/
    lv_coord_t line_space;
    lv_coord_t letter_space;
    lv_coord_t number_height;   /**< height + line_gap, to calculate flow position*/
    int16_t x_adv[10];          /**< character x_adv cache*/
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
 * Set a new animation path callback for numbers flowing
 * @param obj       pointer to a numberflow object
 * @param path      new animation path, NULL for default path 
 */
void lv_numberflow_set_anim_path_flow(lv_obj_t * obj, lv_anim_path_cb_t path);

/**
 * Set a new animation path callback for size transition
 * @param obj       pointer to a numberflow object
 * @param path      new animation path, NULL for default path 
 */
void lv_numberflow_set_anim_path_size(lv_obj_t * obj, lv_anim_path_cb_t path);

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
