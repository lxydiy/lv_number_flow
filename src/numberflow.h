#ifndef NUMBERFLOW_H
#define NUMBERFLOW_H

#include "curves.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum digits to display. E.g. uint32_t have 10 digits(4_294_967_296)*/
#ifndef NUMBERFLOW_MAX_DIGITS
#ifdef LV_NUMBERFLOW_MAX_DIGITS
#define NUMBERFLOW_MAX_DIGITS LV_NUMBERFLOW_MAX_DIGITS
#else
#define NUMBERFLOW_MAX_DIGITS 10
#endif
#endif

/**********************
 *      TYPEDEFS
 **********************/
enum {
    NUMBERFLOW_MODE_NORMAL,          /** flow from bottom to top means increasing*/
    NUMBERFLOW_MODE_REVERSE,         /** same as normal but flow down for increasing*/
};
typedef uint8_t numberflow_mode_t;

enum {
    NUMBERFLOW_DIR_DEFAULT,          /** rolling direction follows the overall trend of value changes, but doesn't consider the relationship between digits*/
    NUMBERFLOW_DIR_CONTINUOUS,       /** makes the number transitions appear to pass through in-between numbers*/
    NUMBERFLOW_DIR_NEAREST,          /** digits flow to the direction of the shortest flow distance*/
    NUMBERFLOW_DIR_INCREASE,         /** force every digit to flow to larger number and loop if needed*/
    NUMBERFLOW_DIR_DECREASE          /** force every digit to flow to smaller number and loop if needed*/
};
typedef uint8_t numberflow_dir_t;

/**
 * On/Off features controlling the numberflow's behavior.
 * Note that you should only set this to a numberflow widget, using lv_numberflow_add_flag
 * or lv_numberflow_clear_flag.
 * These are runtime-modifiable flags that preserve current animations when toggled.
 * OR-ed values are possible
 */
enum {
    NUMBERFLOW_FLAG_TRIM_ZEROS       = (1 << 0),     /**< Trim trailing zeros after decimal point*/
    NUMBERFLOW_FLAG_FADE_VERTICAL    = (1 << 1),     /**< Change the opacity of the upper and lower numbers during the vertical scrolling of numbers*/
    NUMBERFLOW_FLAG_TIGHT            = (1 << 2),     /**< Always update the width of widgets to their actual width. Enabling this feature can cause width to become stuck in place when rapidly updating values using non-monospace fonts*/
    NUMBERFLOW_FLAG_SHOW_POSITIVE    = (1 << 3),     /**< Show positive symbol ('+') before numbers when number is positive*/
};
typedef uint8_t numberflow_flag_t;

typedef struct {
    int32_t anim_state;         /**< Animation value passing to draw function*/
    uint32_t started_at;
    uint32_t duration;
    bool updated;               /**< Animation update flag to recalculate pos and blur*/
    bool pending;
    numberflow_anim_path_t path;
} _nf_anim_state_t;

typedef struct {
    int32_t begin;
    int32_t curr;
    int32_t end;
} _nf_anim_dsc_t;

typedef struct {
    int32_t ofs_y;           /**< Pixels to move this digit's Y down*/
    int8_t target_num;          /**< New number of this digit*/
    uint8_t target_opa;        /**< New opacity of this digit*/
    bool available;             /**< True if this slide should be applied*/
} _nf_slide_start_dsc_t;

typedef struct {
    int8_t num;
    int32_t x;
    int32_t y;
    uint16_t curr_width;
    uint8_t opa;
    uint8_t blur_level;
} _nf_number_draw_dsc_t;

typedef struct {
    uint8_t modulus;            /**< Modulus number of this digit. Store a copy for animation callback*/

    int8_t last_num;            /**< Number in the center when the animation ends*/

    int32_t last_flow;       /**< Last frame's flow position, used to keep motion blur after animation update*/

    _nf_anim_dsc_t flow;     /**< Flow position animation*/
    _nf_anim_dsc_t opa;      /**< Opacity animation*/
    _nf_anim_dsc_t width;    /**< Width animation for non-monospace fonts to flow smoothly*/

    _nf_slide_start_dsc_t slide_start_dsc;   /**< New slide description to update*/
    _nf_anim_state_t anim;   /**< Digit animation state*/

    _nf_number_draw_dsc_t draw[2];
} _nf_digit_t;

typedef struct numberflow {
    numberflow_mode_t mode;
    numberflow_dir_t dir;
    numberflow_anim_path_t anim_path_flow;
    numberflow_anim_path_t anim_path_size;
    int8_t int_fix;             /**< Number of digits before the decimal point. 0 means dynamic length*/
    int8_t dec_fix;             /**< Number of digits after the decimal point. 0 means no decimal part*/
    char ksep;                  /**< Thousand separator to use, '\0' means no separator*/
    char dsep;                  /**< Decimal point to use, can't be '\0'*/
    uint8_t modulus[NUMBERFLOW_MAX_DIGITS * 2]; /**< Stores the modulus for each pos, [0] for least significant digit whose weight is the smallest.*/
    numberflow_flag_t flags; /**< Flags of numberflow, check ::lv_numberflow_flag_t*/
    uint8_t blur_levels;

    int32_t value;              /**< Last value, used for comparison*/
    int8_t digit_count;         /**< Count of allocated digits*/
    int8_t int_cnt;             /**< Count of visible integer digits*/
    int8_t dec_cnt;             /**< Count of visible decimal digits*/
    _nf_digit_t digits[NUMBERFLOW_MAX_DIGITS];     /**< Array of digit descriptors*/
    int8_t ones_place_idx;      /**< Index of the one's place in digits array*/

    _nf_anim_dsc_t x_ofs;    /**< X offset animation of numbers*/
    _nf_anim_dsc_t width;    /**< Width animation of numbers*/

    char sym_old;               /**< Symbol before numbers when animation starts*/
    char sym_curr;              /**< Symbol before numbers when animation ends*/
    _nf_anim_dsc_t sym_opa_old; /**< Opacity animation of old symbol*/
    _nf_anim_dsc_t sym_opa_curr; /**< Opacity animation of current symbol*/
    _nf_anim_dsc_t sym_width; /**< Width animation of symbol*/

    _nf_anim_state_t anim;     /**< Size animation state*/

    void *userdata;
    uint16_t (*get_glyph_width)(void *userdata, char glyph);
    uint32_t now_ms;
    uint32_t anim_time;
    int32_t height;          /**< The font line height. Also the content height of the widget*/
    int32_t line_space;
    int32_t letter_space;
    int32_t number_height;   /**< height + line_gap, to calculate flow position*/
} numberflow_t;

void numberflow_init(numberflow_t *numberflow, void *userdata,
                     uint16_t (*get_glyph_width)(void *userdata, char glyph));
void numberflow_set_value(numberflow_t *numberflow, int32_t value, bool anim);
void numberflow_set_mode(numberflow_t *numberflow, numberflow_mode_t mode);
void numberflow_set_dir(numberflow_t *numberflow, numberflow_dir_t dir);
void numberflow_set_format(numberflow_t *numberflow, uint8_t int_fix, uint8_t dec_fix);
void numberflow_set_separators(numberflow_t *numberflow, char ksep, char dsep);
void numberflow_set_modulus(numberflow_t *numberflow, int8_t pos, uint8_t modulus);
void numberflow_add_flag(numberflow_t *numberflow, numberflow_flag_t flags);
void numberflow_clear_flag(numberflow_t *numberflow, numberflow_flag_t flags);
void numberflow_set_anim_path_flow(numberflow_t *numberflow, numberflow_anim_path_t path);
void numberflow_set_anim_path_size(numberflow_t *numberflow, numberflow_anim_path_t path);
int32_t numberflow_get_value(const numberflow_t *numberflow);
numberflow_anim_path_t numberflow_get_anim_path_flow(const numberflow_t *numberflow);
numberflow_anim_path_t numberflow_get_anim_path_size(const numberflow_t *numberflow);

/** @note Update now_ms before setting values; update does not perform layout. */
bool numberflow_update(numberflow_t *numberflow, uint32_t now_ms);
bool numberflow_is_animating(const numberflow_t *numberflow);
void numberflow_layout(numberflow_t *numberflow);
void numberflow_layout_size(numberflow_t *numberflow);

#ifdef __cplusplus
}
#endif
#endif
