/**
 * @file lv_numberflow.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_numberflow.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_numberflow_class

#define LV_NUMBERFLOW_IS_ANIMATING(anim_struct) (((anim_struct).anim_state) != LV_NUMBERFLOW_ANIM_STATE_INV)

/** Numberflow animation start value. (Not the real value just indicates process animation)*/
#define LV_NUMBERFLOW_ANIM_STATE_START 0

/** Numberflow animation end value.  (Not the real value just indicates process animation)*/
#define LV_NUMBERFLOW_ANIM_STATE_END   16384

/** Mark no animation is in progress*/
#define LV_NUMBERFLOW_ANIM_STATE_INV   -1

/** log2(LV_NUMBERFLOW_ANIM_STATE_END) used to normalize data*/
#define LV_NUMBERFLOW_ANIM_STATE_NORM  14

/** maximum value in number_flow_ease_lut*/
#define LV_NUMBERFLOW_ANIM_RESOLUTION 16384

/** log2(LV_NUMBERFLOW_ANIM_RESOLUTION) used to normalize data*/
#define LV_NUMBERFLOW_ANIM_RES_SHIFT 14

/** Element count of number_flow_ease_lut*/
#define LV_NUMBERFLOW_ANIM_LUT_SIZE 90

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline int8_t expo_pos_to_digit_index(lv_numberflow_t *numberflow, int8_t pos);
static inline uint8_t digit_index_to_mod_index(lv_numberflow_t *numberflow, int8_t idx);
static void lv_numberflow_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_numberflow_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static int32_t lv_numberflow_ease_curve(const lv_anim_t * anim);
static void reset_animations(lv_numberflow_t *numberflow);
static void recalculate_style(lv_numberflow_t *numberflow);
static void lv_numberflow_event(const lv_obj_class_t * class_p, lv_event_t * e);
static uint16_t get_glyph_width(lv_numberflow_t *numberflow, char glyph);
static void lv_numberflow_update_with_anim(lv_obj_t * obj, int8_t *nums, int8_t int_cnt, int8_t dec_cnt, int8_t trend, lv_anim_enable_t en);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_numberflow_class = {
    .constructor_cb = lv_numberflow_constructor,
    .destructor_cb = lv_numberflow_destructor,
    .event_cb = lv_numberflow_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_numberflow_t),
    .base_class = &lv_obj_class
};

/** To be more consistent with the original animation of NumberFlow, a custom
 *  animation curve is used. This animation curve below is mapped from:
 *  https://github.com/barvian/number-flow/blob/7ab1a7164ef1b088267f789b07581fefef064b68/packages/number-flow/src/lite.ts#L73 */
static const uint16_t number_flow_ease_lut[LV_NUMBERFLOW_ANIM_LUT_SIZE] = {
      0,     82,    311,    639,   1081,   1573,   2114,   2703,   3310,   3932,   4555,   5177,   5800,   6390,   6980,   7553,
   8094,   8618,   9126,   9601,  10060,  10486,  10895,  11289,  11649,  11977,  12304,  12599,  12878,  13140,  13386,  13615,
  13828,  14025,  14205,  14369,  14533,  14680,  14811,  14942,  15057,  15155,  15254,  15352,  15434,  15516,  15581,  15647,
  15712,  15761,  15811,  15860,  15909,  15942,  15991,  16024,  16056,  16073,  16105,  16122,  16155,  16171,  16187,  16204,
  16220,  16237,  16253,  16253,  16269,  16286,  16286,  16302,  16302,  16318,  16318,  16323,  16330,  16333,  16338,  16343,
  16346,  16350,  16353,  16355,  16358,  16359,  16363,  16364,  16366,  16384
};

/**********************
 *      MACROS
 **********************/

/** Calculate current animation value*/
#define ANIM_LERP(state, dsc) \
    ( LV_NUMBERFLOW_ANIM_STATE_INV != (state) ? \
    ((dsc.begin) + ((((dsc.end) - (dsc.begin)) * (state)) >> LV_NUMBERFLOW_ANIM_STATE_NORM)) \
    : (dsc.end))

/** Update end, but keep current value as begin to implement continuous animation*/
#define ANIM_UPDATE(dsc, target) \
    do { \
        dsc.begin = dsc.curr; \
        dsc.end = target; \
    } while(0)

/** Update target without animation*/
#define ANIM_UPDATE_STOP(dsc, target) \
    do { \
        dsc.begin = target; \
        dsc.curr = target; \
        dsc.end = target; \
    } while(0)

#define MAP(r, rmin, rmax, tmin, tmax) \
    ((tmin) + ((r) - (rmin)) * ((tmax) - (tmin)) / ((rmax) - (rmin)))

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_numberflow_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_numberflow_set_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    int8_t trend;
    if (value > numberflow->value) {
        trend = 1;
    }
    else if (value < numberflow->value) {
        trend = -1;
    }
    else {
        trend = 0;
    }

    char nums[LV_NUMBERFLOW_MAX_DIGITS];
    lv_memset_00(nums, sizeof(nums));

    int8_t nums_idx = 0;
    int32_t target = value;
    while (target > 0) {
        nums[nums_idx++] = target % 10;
        target /= 10;
    }

    /*Reverse nums array*/
    for (int8_t i = 0; i < (nums_idx + 1) / 2; i++) {
        int8_t peer = nums_idx - 1 - i;
        int8_t t;
        t = nums[i];
        nums[i] = nums[peer];
        nums[peer] = t;
    }

    int8_t int_cnt;
    if (numberflow->int_fix > 0) {
        int_cnt = numberflow->int_fix;
    }
    else {
        int_cnt = nums_idx - numberflow->dec_fix;
        if (int_cnt < 1) {
            int_cnt = 1;
        }
    }

    int8_t pad = int_cnt + numberflow->dec_fix - nums_idx;
    if (pad > 0) {
        for (int8_t i = nums_idx - 1; i >= 0; i--) {
            nums[i + pad] = nums[i];
        }
        for (int8_t i = 0; i < pad; i++) {
            nums[i] = 0;
        }
        nums_idx += pad;
    }

    /*Make sure we have at least 1 digit to process*/
    if (nums_idx == 0) nums_idx = 1;

    /*Trim decimal zeros*/
    int8_t dec_cnt = numberflow->dec_fix;
    if (numberflow->flags & LV_NUMBERFLOW_FLAG_TRIM_ZEROS) {
        for (int8_t i = nums_idx - 1; i >= nums_idx - numberflow->dec_fix; i--) {
            if (nums[i] == 0) {
                dec_cnt--;
            }
            else {
                break;
            }
        }
    }

    lv_numberflow_update_with_anim(obj, nums, int_cnt, dec_cnt, trend, anim);
    numberflow->value = value;
}

void lv_numberflow_set_mode(lv_obj_t * obj, lv_numberflow_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->mode = mode;
}

void lv_numberflow_set_dir(lv_obj_t * obj, lv_numberflow_dir_t dir)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->dir = dir;
}

void lv_numberflow_set_format(lv_obj_t * obj, uint8_t int_fix, uint8_t dec_fix)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (dec_fix > LV_NUMBERFLOW_MAX_DIGITS) {
        dec_fix = LV_NUMBERFLOW_MAX_DIGITS;
    }
    if (int_fix + dec_fix > LV_NUMBERFLOW_MAX_DIGITS) {
        int_fix = LV_NUMBERFLOW_MAX_DIGITS - dec_fix; 
    }

    numberflow->int_fix = int_fix;
    numberflow->dec_fix = dec_fix;
    lv_numberflow_set_value(obj, numberflow->value, LV_ANIM_ON);
}

void lv_numberflow_set_separators(lv_obj_t * obj, char ksep, char dsep)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (dsep == '\0') dsep = '.';

    numberflow->ksep = ksep;
    numberflow->dsep = dsep;

    lv_numberflow_set_value(obj, numberflow->value, LV_ANIM_ON);
}

void lv_numberflow_set_modulus(lv_obj_t * obj, int8_t pos, uint8_t modulus)
{
    if (pos <= -LV_NUMBERFLOW_MAX_DIGITS || pos >= LV_NUMBERFLOW_MAX_DIGITS) return;
    if (modulus > 10) return;
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->modulus[pos + LV_NUMBERFLOW_MAX_DIGITS] = modulus;
    int8_t idx = expo_pos_to_digit_index(numberflow, pos);
    if (idx >= 0 && idx < numberflow->digit_count) {
        numberflow->digits[idx]->modulus = modulus;
    }
}

void lv_numberflow_add_flag(lv_obj_t * obj, lv_numberflow_flag_t flags)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->flags |= flags;
    lv_numberflow_set_value(obj, numberflow->value, LV_ANIM_ON);
}

void lv_numberflow_clear_flag(lv_obj_t * obj, lv_numberflow_flag_t flags)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->flags &= ~flags;
    lv_numberflow_set_value(obj, numberflow->value, LV_ANIM_ON);
}

void lv_numberflow_set_anim_path_flow(lv_obj_t * obj, lv_anim_path_cb_t path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (path == NULL) {
        numberflow->anim_path_flow = lv_numberflow_ease_curve;
    }
    else {
        numberflow->anim_path_flow = path;
    }
}

void lv_numberflow_set_anim_path_size(lv_obj_t * obj, lv_anim_path_cb_t path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (path == NULL) {
        numberflow->anim_path_size = lv_anim_path_ease_out;
    }
    else {
        numberflow->anim_path_size = path;
    }
}

void lv_numberflow_set_blur_data(lv_obj_t * obj, const lv_numberflow_blur_data_t * blur_data)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (numberflow->blur_data == blur_data) return;

    numberflow->blur_data = blur_data;

    recalculate_style(numberflow);
}

/*=====================
 * Getter functions
 *====================*/

int32_t lv_numberflow_get_value(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    return numberflow->value;
}

lv_anim_path_cb_t lv_numberflow_get_anim_path_flow(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    return numberflow->anim_path_flow;
}

lv_anim_path_cb_t lv_numberflow_get_anim_path_size(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    return numberflow->anim_path_size;
}

const lv_numberflow_blur_data_t * lv_numberflow_get_blur_data(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    return numberflow->blur_data;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_numberflow_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;
    numberflow->mode = LV_NUMBERFLOW_MODE_NORMAL;
    numberflow->dir = LV_NUMBERFLOW_DIR_DEFAULT;
    numberflow->anim_path_flow = lv_numberflow_ease_curve;
    numberflow->anim_path_size = lv_anim_path_ease_out;
    numberflow->int_fix = 0;
    numberflow->dec_fix = 0;
    numberflow->ksep = '\0';
    numberflow->dsep = '.';
    numberflow->flags = LV_NUMBERFLOW_FLAG_FADE_VERTICAL;
    for (int8_t i = 0; i < sizeof(numberflow->modulus) / sizeof(numberflow->modulus[0]); i++) {
        numberflow->modulus[i] = 10;
    }
    numberflow->blur_data = NULL;

    numberflow->line_space = lv_obj_get_style_text_line_space(obj, 0);
    numberflow->letter_space = lv_obj_get_style_text_letter_space(obj, 0);
    numberflow->height = lv_font_get_line_height(lv_obj_get_style_text_font(obj, LV_PART_MAIN));
    numberflow->number_height = numberflow->height + numberflow->line_space;
    lv_memset_00(numberflow->x_adv, sizeof(numberflow->x_adv));

    numberflow->value = 0;

    numberflow->digit_count = 0;
    numberflow->int_cnt = 0;
    numberflow->dec_cnt = 0;
    lv_memset_00(numberflow->digits, sizeof(numberflow->digits));
    numberflow->ones_place_idx = -1;

    numberflow->width.begin = 0;
    numberflow->width.curr = 0;
    numberflow->width.end = 0;

    numberflow->x_ofs.begin = 0;
    numberflow->x_ofs.curr = 0;
    numberflow->x_ofs.end = 0;

    numberflow->anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_numberflow_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    for (int32_t i = 0; i < numberflow->digit_count; i++)
    {
        lv_anim_del(numberflow->digits[i], NULL);
    }
    lv_anim_del(numberflow, NULL);
    for (int8_t i = 0; i < LV_NUMBERFLOW_MAX_DIGITS; i++) {
        if (numberflow->digits[i] != NULL) {
            lv_mem_free(numberflow->digits[i]);
            numberflow->digits[i] = NULL;
        }
    }
    numberflow->digit_count = 0;
}

/*Simple integer linear interpolator for animation curve*/
static int32_t lv_numberflow_ease_curve(const lv_anim_t * anim)
{
    /*Calculate the current step*/
    uint32_t t = lv_map(anim->act_time, 0, anim->time, 0, LV_NUMBERFLOW_ANIM_RESOLUTION);

    uint32_t pos = (uint32_t)t * (LV_NUMBERFLOW_ANIM_LUT_SIZE - 1);

    uint32_t index = pos / LV_NUMBERFLOW_ANIM_RESOLUTION;
    uint32_t frac  = pos % LV_NUMBERFLOW_ANIM_RESOLUTION;

    uint32_t a = number_flow_ease_lut[index];
    uint32_t b = number_flow_ease_lut[index + 1];

    int32_t step = a + (((b - a) * frac) >> LV_NUMBERFLOW_ANIM_RES_SHIFT);

    int32_t new_value;
    new_value = step * (anim->end_value - anim->start_value);
    new_value = new_value >> LV_NUMBERFLOW_ANIM_RES_SHIFT;
    new_value += anim->start_value;

    return new_value;
}

static inline const lv_nf_glyph_blur_dsc_t *get_blur(lv_numberflow_t *numberflow, int8_t num, uint8_t blur_level)
{
    uint8_t blur_level_max = numberflow->blur_data->blur_level;
    return &numberflow->blur_data->blurs[num * blur_level_max + blur_level];
}

static inline bool digit_has_decimal_sep(lv_numberflow_t *numberflow, int8_t idx)
{
    return idx == numberflow->ones_place_idx + 1;
}

static inline bool digit_has_thousand_sep(lv_numberflow_t *numberflow, int8_t idx)
{
    int8_t integer_pos = numberflow->ones_place_idx - idx;
    if (integer_pos <= 0) return false;
    return integer_pos % 3 == 0 && numberflow->ksep != '\0';
}

static inline int8_t expo_pos_to_digit_index(lv_numberflow_t *numberflow, int8_t pos)
{
    int8_t idx = numberflow->ones_place_idx - pos;
    return idx;
}

static inline uint8_t digit_index_to_mod_index(lv_numberflow_t *numberflow, int8_t idx)
{
    int8_t pos = numberflow->ones_place_idx - idx;
    return pos + LV_NUMBERFLOW_MAX_DIGITS;
}

static void draw_number(lv_numberflow_t *numberflow, lv_draw_ctx_t *draw_ctx,
                        _lv_nf_number_draw_dsc_t *num_draw_dsc)
{
    lv_obj_t *obj = (lv_obj_t *)numberflow;
    uint16_t width = get_glyph_width(numberflow, num_draw_dsc->num + '0');
    lv_color_t color = lv_obj_get_style_text_color(obj, LV_PART_MAIN);

    if (num_draw_dsc->opa == LV_OPA_TRANSP) return;

    if (numberflow->blur_data != NULL) {
        lv_draw_img_dsc_t img_draw_dsc;
        lv_draw_img_dsc_init(&img_draw_dsc);
        img_draw_dsc.opa = num_draw_dsc->opa;
        img_draw_dsc.recolor = color;
        img_draw_dsc.recolor_opa = LV_OPA_COVER;

        lv_area_t coords;
        lv_obj_get_content_coords(obj, &coords);

        const lv_nf_glyph_blur_dsc_t *blur = get_blur(numberflow, num_draw_dsc->num, num_draw_dsc->blur_level);

        coords.x1 = coords.x1 + num_draw_dsc->x + blur->ofs_x + (num_draw_dsc->curr_width - width + 1) / 2;
        coords.y1 = coords.y1 + num_draw_dsc->y + blur->ofs_y;
        coords.x2 = coords.x1 + blur->box_w - 1;
        coords.y2 = coords.y1 + blur->box_h - 1;

        lv_draw_img_decoded(draw_ctx, &img_draw_dsc, &coords, numberflow->blur_data->glyph_blob + blur->start, LV_IMG_CF_ALPHA_8BIT);
    }
    else {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = color;
        label_dsc.font = numberflow->font;
        label_dsc.opa = num_draw_dsc->opa;

        lv_point_t pos;
        lv_area_t coords;
        lv_obj_get_content_coords(obj, &coords);

        pos.x = coords.x1 + num_draw_dsc->x;
        pos.y = coords.y1 + num_draw_dsc->y;

        /*Compensate for the horizontal position of non-monospace fonts to make them
        * display centered*/
        pos.x += (num_draw_dsc->curr_width - width + 1) / 2;

        lv_draw_letter(draw_ctx, &label_dsc, &pos, num_draw_dsc->num + '0');
    }
}

static void draw_glyph(lv_numberflow_t *numberflow, lv_draw_ctx_t *draw_ctx,
                        lv_point_t *pos_rel, lv_opa_t opa, const char glyph)
{
    lv_obj_t *obj = (lv_obj_t *)numberflow;
    lv_color_t color = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = color;
    label_dsc.font = numberflow->font;
    label_dsc.opa = opa;

    lv_point_t pos;
    lv_area_t coords;
    lv_obj_get_content_coords(obj, &coords);

    pos.x = coords.x1 + pos_rel->x;
    pos.y = coords.y1 + pos_rel->y;

    lv_draw_letter(draw_ctx, &label_dsc, &pos, glyph);
}

static void draw_numberflow(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;
    _lv_nf_digit_t * digit;

    lv_draw_ctx_t * ctx = lv_event_get_draw_ctx(e);

    for (int32_t i = 0; i < numberflow->digit_count; i++)
    {
        digit = numberflow->digits[i];
        if (digit_has_decimal_sep(numberflow, i)) {
            /*Decimal separator's animation is bound to the first decimal digit*/
            lv_point_t pos_rel;
            pos_rel.x = digit->draw[0].x - numberflow->letter_space - get_glyph_width(numberflow, numberflow->dsep);
            pos_rel.y = 0;
            draw_glyph(numberflow, ctx, &pos_rel, digit->opa.curr, numberflow->dsep);
        }
        draw_number(numberflow, ctx, &digit->draw[0]);
        draw_number(numberflow, ctx, &digit->draw[1]);
        if (digit_has_thousand_sep(numberflow, i)) {
            /*Thousand separator's animation is bound to the integer digit before it*/
            lv_point_t pos_rel;
            pos_rel.x = digit->draw[0].x + digit->draw[0].curr_width + numberflow->letter_space;
            pos_rel.y = 0;
            draw_glyph(numberflow, ctx, &pos_rel, digit->opa.curr, numberflow->ksep);
        }
    }
}

static uint16_t layout_digit(_lv_nf_digit_t *digit, lv_coord_t x)
{
    lv_numberflow_t *numberflow = (lv_numberflow_t *)digit->anim.numberflow;
    lv_obj_t *obj = (lv_obj_t *)numberflow;

    if (digit->anim.updated == false) {
        /*Digit animation didn't update. Only update x and return old width*/
        digit->draw[0].x = x;
        digit->draw[1].x = x;
        return digit->draw[0].curr_width;
    }
    digit->anim.updated = false;

    int32_t value = digit->anim.anim_state;

    if (value == LV_NUMBERFLOW_ANIM_STATE_INV) {
        value = LV_NUMBERFLOW_ANIM_STATE_END;
    }

    lv_coord_t height = numberflow->number_height;
    lv_coord_t total_height = height * digit->modulus;
    lv_coord_t center = ANIM_LERP(value, digit->flow);

    /*Calculate current number*/
    int center_mod = center % total_height;
    if (center_mod < 0) center_mod += total_height;

    int lower_num, upper_num;
    int lower_off, upper_off;
    int lower_opa, upper_opa;

    if (numberflow->mode == LV_NUMBERFLOW_MODE_NORMAL) {
        upper_num = center_mod / height;
        upper_off = -(center_mod % height);
        lower_num = (upper_num + 1) % digit->modulus;
        lower_off = upper_off + height;

        if (numberflow->flags & LV_NUMBERFLOW_FLAG_FADE_VERTICAL) {
            upper_opa = MAP(upper_off, 0, -height, LV_OPA_COVER, LV_OPA_TRANSP);
            lower_opa = LV_OPA_COVER - upper_opa;
        }
        else {
            lower_opa = LV_OPA_COVER;
            upper_opa = LV_OPA_COVER;
        }
    }
    else {
        lower_num = center_mod / height;
        lower_off = center_mod % height;
        upper_num = (lower_num + 1) % digit->modulus;
        upper_off = lower_off - height;

        if (numberflow->flags & LV_NUMBERFLOW_FLAG_FADE_VERTICAL) {
            lower_opa = MAP(lower_off, 0, height, LV_OPA_COVER, LV_OPA_TRANSP);
            upper_opa = LV_OPA_COVER - lower_opa;
        }
        else {
            lower_opa = LV_OPA_COVER;
            upper_opa = LV_OPA_COVER;
        }
    }

    int8_t blur_pos = 0;
    if (numberflow->blur_data != NULL) {
        /*Calculate blur based on flowing position difference*/
        if (value != LV_NUMBERFLOW_ANIM_STATE_END) {
            blur_pos = center - digit->flow.curr;
            blur_pos = LV_ABS(blur_pos) / 2;
            if (blur_pos >= 1) {
                blur_pos -= 1;
            }
            if (blur_pos >= numberflow->blur_data->blur_level) {
                blur_pos = numberflow->blur_data->blur_level - 1;
            }
        }
        else {
            /*There shouldn't be any blur at the last frame*/
            blur_pos = 0;
        }
    }

    int16_t digit_opa = ANIM_LERP(value, digit->opa);
    if (digit_opa > LV_OPA_COVER) {
        digit_opa = LV_OPA_COVER;
    }
    digit->opa.curr = (lv_opa_t)digit_opa;
    digit->width.curr = ANIM_LERP(digit->anim.anim_state, digit->width);
    digit->last_flow = digit->flow.curr;
    digit->flow.curr = center;

    digit->draw[0].num = upper_num;
    digit->draw[0].x = x;
    digit->draw[0].y = upper_off;
    digit->draw[0].curr_width = digit->width.curr;
    digit->draw[0].opa = (upper_opa * digit_opa) / LV_OPA_COVER;
    digit->draw[0].blur_level = blur_pos;

    digit->draw[1].num = lower_num;
    digit->draw[1].x = x;
    digit->draw[1].y = lower_off;
    digit->draw[1].curr_width = digit->width.curr;
    digit->draw[1].opa = (lower_opa * digit_opa) / LV_OPA_COVER;
    digit->draw[1].blur_level = blur_pos;

    return digit->width.curr;
}

static void layout_numberflow(lv_numberflow_t *numberflow)
{
    lv_obj_t * obj = (lv_obj_t *)numberflow;

    int32_t x_ofs = ANIM_LERP(numberflow->anim_state, numberflow->x_ofs);
    numberflow->x_ofs.curr = x_ofs;

    /*We need to count invisible digits's width*/
    for (int32_t i = 0; i < numberflow->digit_count; i++)
    {
        if (digit_has_decimal_sep(numberflow, i)) {
            /*Decimal separator's animation is bound to the first decimal digit*/
            x_ofs += get_glyph_width(numberflow, numberflow->dsep);
            x_ofs += numberflow->letter_space;
        }
        x_ofs += layout_digit(numberflow->digits[i], x_ofs);
        x_ofs += numberflow->letter_space;
        if (digit_has_thousand_sep(numberflow, i)) {
            /*Thousand separator's animation is bound to the integer digit before it*/
            x_ofs += get_glyph_width(numberflow, numberflow->ksep);
            x_ofs += numberflow->letter_space;
        }
    }
}

static void recalculate_style(lv_numberflow_t *numberflow)
{
    lv_obj_t *obj = (lv_obj_t *)numberflow;

    /*Update line height and space*/
    const lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    lv_coord_t height;
    lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
    lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);

    if (numberflow->blur_data != NULL) {
        height = LV_MAX(numberflow->blur_data->line_height, lv_font_get_line_height(font));
    }
    else {
        height = lv_font_get_line_height(font);
    }

    if (font != numberflow->font || height != numberflow->height
        || line_space != numberflow->line_space || letter_space != numberflow->letter_space) {
        /*Reset animation due to line_height change*/
        reset_animations(numberflow);

        numberflow->font = font;
        numberflow->height = height;
        numberflow->line_space = line_space;
        numberflow->number_height = height + numberflow->line_space;
        numberflow->letter_space = letter_space;
        lv_memset_00(numberflow->x_adv, sizeof(numberflow->x_adv));

        /*Reset animation again with new line_height*/
        reset_animations(numberflow);
    }
    lv_obj_invalidate(obj);
    lv_obj_refresh_self_size(obj);
}

static void lv_numberflow_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_coord_t * s = lv_event_get_param(e);

        /*Need to redraw the whole up/down fading area*/
        *s = LV_MAX(*s, numberflow->number_height);
        /*Also check for x_offset to display fading digits on the left/right*/
        if(numberflow->x_ofs.curr < 0)
            *s = LV_MAX(*s, -numberflow->x_ofs.curr);
        else
            *s = LV_MAX(*s, numberflow->x_ofs.curr + numberflow->width.curr);
    }
    else if(code == LV_EVENT_GET_SELF_SIZE) {
        lv_point_t * p = lv_event_get_param(e);
        p->x = ANIM_LERP(numberflow->anim_state, numberflow->width);
        numberflow->width.curr = p->x;
        p->y = numberflow->height;
    }
    else if (code == LV_EVENT_STYLE_CHANGED) {
        recalculate_style(numberflow);
    }
    else if(code == LV_EVENT_DRAW_MAIN) {
        layout_numberflow(numberflow);
        draw_numberflow(e);
    }
}

static void anim_digit(void * var, int32_t value)
{
    _lv_nf_digit_t * digit = var;

    digit->anim.anim_state = value;
    digit->anim.updated = true;

    lv_obj_invalidate(digit->anim.numberflow);
}

static void anim_digit_ready(lv_anim_t * a)
{
    _lv_nf_digit_t * digit = a->var;

    digit->anim.anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;
    digit->anim.updated = true;

    lv_obj_invalidate(digit->anim.numberflow);
}

static void anim_digit_start(lv_anim_t * a)
{
    _lv_nf_digit_t * digit = a->var;
    _lv_nf_slide_start_dsc_t* slide_start_dsc = &digit->slide_start_dsc;
    lv_numberflow_t *numberflow = (lv_numberflow_t *)digit->anim.numberflow;

    lv_coord_t line_height = numberflow->number_height;
    lv_coord_t total_height = line_height * digit->modulus;

    /*Remove redundant flow coordinates as we are using accumulate mode*/
    lv_coord_t end_px = digit->flow.end;
    lv_coord_t start_px = digit->flow.curr;
    lv_coord_t last_frame_px = digit->last_flow;
    lv_coord_t start_px_overflow = (start_px / total_height) * total_height;
    start_px -= start_px_overflow;
    end_px -= start_px_overflow;
    last_frame_px -= start_px_overflow;

    /*Update flow position*/
    digit->flow.begin = start_px;
    digit->flow.end = end_px + slide_start_dsc->ofs_y;
    /*Reset 'current' flow position to last frame's or the speed will be zero at next frame*/
    digit->flow.curr = last_frame_px;
    digit->last_flow = last_frame_px;
    digit->last_num = slide_start_dsc->target_num % digit->modulus;
    if (digit->last_num < 0) digit->last_num += digit->modulus;

    /*Clamp abs(end-begin) inside 2 full cycle to avoid flowing too fast*/
    lv_coord_t remaining = digit->flow.end - digit->flow.begin;
    lv_coord_t remaining_overflow = 0;
    if (remaining > total_height)
        remaining_overflow = (int)((remaining - total_height) / total_height) * total_height;
    else if (remaining < -total_height)
        remaining_overflow = (int)((remaining + total_height) / total_height) * total_height;

    digit->flow.end -= remaining_overflow;

    digit->anim.anim_state = LV_NUMBERFLOW_ANIM_STATE_START;
    digit->anim.updated = true;

    /*Update digit width*/
    int16_t width = get_glyph_width(numberflow, digit->last_num + '0');
    ANIM_UPDATE(digit->width, width);

    /*Update digit opacity*/
    ANIM_UPDATE(digit->opa, slide_start_dsc->target_opa);

    lv_obj_invalidate(digit->anim.numberflow);
}

static void anim_size(void * var, int32_t value)
{
    lv_obj_t * obj = (lv_obj_t *)var;
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;
    numberflow->anim_state = value;

    lv_obj_invalidate(obj);
    lv_obj_refresh_self_size(obj);
}

static void anim_size_ready(lv_anim_t * a)
{
    lv_obj_t * obj = (lv_obj_t *)a->var;
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;

    lv_obj_invalidate(obj);
    lv_obj_refresh_self_size(obj);
}

static uint16_t get_glyph_width(lv_numberflow_t *numberflow, char glyph)
{
    if (glyph == '\0') return 0;
    if (glyph >= '0' && glyph <= '9') {
        /*Use cache to speed up layout as Arabic numerals don't require kerning,
        * especially for mono fonts, which is recommended to use in this widget.
        * Use '0' as letter_next because next letter is a number*/
        if (numberflow->blur_data != NULL) {
            return numberflow->blur_data->x_adv[glyph - '0'];
        }
        else {
            if (numberflow->x_adv[glyph - '0'] != 0) return numberflow->x_adv[glyph - '0'];
            uint16_t width = lv_font_get_glyph_width(numberflow->font, glyph, '0');
            numberflow->x_adv[glyph - '0'] = width;
            return width;
        }
    }
    else {
        /*Fall back to lv_font's get width.*/
        uint16_t width = lv_font_get_glyph_width(numberflow->font, glyph, '0');
        return width;
    }
}

static void reset_animations(lv_numberflow_t *numberflow)
{
    if (numberflow->digit_count != 0) {
        lv_numberflow_set_value((lv_obj_t *)numberflow, numberflow->value, LV_ANIM_OFF);
    }
}

static void digit_init(lv_numberflow_t *numberflow, int8_t idx)
{
    _lv_nf_digit_t *digit = numberflow->digits[idx];
    lv_memset_00(digit, sizeof(_lv_nf_digit_t));
    digit->modulus = numberflow->modulus[digit_index_to_mod_index(numberflow, idx)];
    digit->anim.updated = true;
    digit->anim.numberflow = (lv_obj_t *)numberflow;
    uint16_t width0 = get_glyph_width(numberflow, '0');
    ANIM_UPDATE_STOP(digit->width, width0);
}

static bool digit_realloc(lv_numberflow_t *numberflow, int8_t front, int8_t back)
{
    int8_t new_count = numberflow->digit_count + front + back;
    if (new_count > LV_NUMBERFLOW_MAX_DIGITS) {
        LV_LOG_ERROR("digits overflow, please set LV_NUMBERFLOW_MAX_DIGITS larger");
        return false;
    }
    if (front == 0 && back == 0) return true;

    /*Move old digits to the back*/
    for (int8_t i = numberflow->digit_count - 1; i >= 0; i--) {
        numberflow->digits[i + front] = numberflow->digits[i];
        numberflow->digits[i] = NULL;
    }
    numberflow->ones_place_idx += front;

    /*Clear allocated digits*/
    for (int8_t i = 0; i < front; i++) {
        numberflow->digits[i] = lv_mem_alloc(sizeof(_lv_nf_digit_t));
        if (numberflow->digits[i] == NULL) goto free_all;
        digit_init(numberflow, i);
    }
    for (int8_t i = 0; i < back; i++) {
        int8_t digit_idx = i + front + numberflow->digit_count;
        numberflow->digits[digit_idx] = lv_mem_alloc(sizeof(_lv_nf_digit_t));
        if (numberflow->digits[digit_idx] == NULL) goto free_all;
        digit_init(numberflow, digit_idx);
    }

    numberflow->digit_count = new_count;
    return true;

free_all:
    for (int8_t i = 0; i < LV_NUMBERFLOW_MAX_DIGITS; i++) {
        if (numberflow->digits[i] != NULL) {
            lv_mem_free(numberflow->digits[i]);
            numberflow->digits[i] = NULL;
        }
    }
    numberflow->digit_count = 0;
    return false;
}

static int8_t digit_roll(_lv_nf_digit_t* digit, int8_t target_num, int8_t direction,
                         lv_opa_t target_opa, lv_anim_path_cb_t path, uint32_t time)
{
    lv_numberflow_t *numberflow = (lv_numberflow_t *)digit->anim.numberflow;

    lv_coord_t height = numberflow->number_height;
    int32_t round_height = height * digit->modulus;
    int8_t last_num = digit->last_num;

    /*Calculate remaining flow distance*/
    lv_coord_t ofs_y = (target_num - last_num) * height;
    if (direction == 1) {
        /*assuming target > last, otherwise rolling around*/
        if (ofs_y < 0)
            ofs_y += round_height;
    }
    else if (direction == -1) {
        /*assuming target < last, otherwise rolling around*/
        if (ofs_y > 0)
            ofs_y -= round_height;
    }
    else {
        /*nearest*/
        ofs_y = ofs_y % round_height;
        if (ofs_y > round_height / 2) ofs_y -= round_height;
        if (ofs_y < -round_height / 2) ofs_y += round_height;
    }

    if (time == 0) {
        lv_anim_del(digit, anim_digit);
        digit->anim.anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;
        digit->anim.updated = true;
        lv_obj_invalidate((lv_obj_t *)numberflow);
        lv_obj_refresh_self_size((lv_obj_t *)numberflow);

        /*Remove redundant loop coordinates generated by the accumulate method
        * otherwise lv_coord_t will overflow*/
        lv_coord_t end_px = digit->flow.end + ofs_y;
        lv_coord_t start_px = digit->flow.curr;
        lv_coord_t start_px_overflow = (start_px / round_height) * round_height;
        end_px -= start_px_overflow;

        ANIM_UPDATE_STOP(digit->flow, end_px);
        digit->last_flow = end_px;

        digit->last_num = (target_num + digit->modulus) % digit->modulus;
        int16_t width = get_glyph_width(numberflow, digit->last_num + '0');

        ANIM_UPDATE_STOP(digit->opa, target_opa);
        ANIM_UPDATE_STOP(digit->width, width);
    }
    else {
        if (ofs_y == 0 && target_opa == digit->opa.end) {
            /*target didn't changed so we don't need to animate*/
            return 0;
        }
        digit->slide_start_dsc.ofs_y = ofs_y;
        digit->slide_start_dsc.target_num = target_num;
        digit->slide_start_dsc.target_opa = target_opa;

        lv_anim_t a;

        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, anim_digit);
        lv_anim_set_time(&a, time);
        lv_anim_set_path_cb(&a, path);

        lv_anim_set_values(&a, LV_NUMBERFLOW_ANIM_STATE_START, LV_NUMBERFLOW_ANIM_STATE_END);
        lv_anim_set_var(&a, digit);

        lv_anim_set_ready_cb(&a, anim_digit_ready);

        /*We need to update digit position just before current animation start
        * to achieve smooth transition*/
        lv_anim_set_start_cb(&a, anim_digit_start);
        lv_anim_set_early_apply(&a, false);

        lv_anim_start(&a);
    }
    return target_num - last_num;
}

static void lv_numberflow_update_with_anim(lv_obj_t * obj, int8_t *nums, int8_t int_cnt, int8_t dec_cnt, int8_t trend, lv_anim_enable_t en)
{
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    uint32_t anim_time;
    if(en == LV_ANIM_OFF) {
        anim_time = 0;
    }
    else {
        anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    }

    /*Calculate new digits to be allocated before and after current digits*/
    int8_t int_space = numberflow->ones_place_idx + 1;
    int8_t dec_space = numberflow->digit_count - int_space;
    int8_t front, back;
    if (numberflow->digit_count == 0) {
        front = int_cnt;
        back = dec_cnt;
    }
    else {
        front = int_cnt - int_space;
        back = dec_cnt - dec_space;
    }
    if (front < 0) front = 0;
    if (back < 0) back = 0;

    if (!digit_realloc(numberflow, front, back)) return;
    int_space = numberflow->ones_place_idx + 1;
    dec_space = numberflow->digit_count - int_space;

    int8_t visible_digit_count = int_cnt + dec_cnt;
    int8_t first_visible_idx = int_space - int_cnt;
    int8_t last_visible_idx = first_visible_idx + visible_digit_count - 1;

    uint16_t width0 = get_glyph_width(numberflow, '0');
    uint16_t width_ksep = get_glyph_width(numberflow, numberflow->ksep) + numberflow->letter_space;
    uint16_t width_dsep = get_glyph_width(numberflow, numberflow->dsep) + numberflow->letter_space;
    int8_t ksep_cnt = (int_cnt - 1) / 3;
    int8_t dsep_cnt = dec_cnt > 0 ? 1 : 0;
    lv_coord_t width = numberflow->letter_space * (visible_digit_count + dsep_cnt - 1);
    for (int8_t i = first_visible_idx; i <= last_visible_idx; i++) {
        width += get_glyph_width(numberflow, nums[i - first_visible_idx] + '0');
    }
    width += width_dsep * dsep_cnt;
    if (numberflow->ksep != '\0') {
        width += ksep_cnt * width_ksep;
    }

    /*End x_offset should be the width of invisible zeros on the left plus their thousand separator's width*/
    int8_t inv_zeros = first_visible_idx;
    int8_t inv_kseps = (int_space - 1) / 3 - ksep_cnt;
    lv_coord_t x_offset_end = -inv_zeros * (width0 + numberflow->letter_space);
    if (numberflow->ksep != '\0') {
        x_offset_end -= inv_kseps * width_ksep;
    }
    /*Start x_offset should minus newly added zeros' width plus new thousand separators' width.
    * Note that x_offset should always be negative as it's used to move invisible digits
    * on the left out of widget, but keep them in place to make animation continuous*/
    lv_coord_t x_offset_start_delta = -front * (width0 + numberflow->letter_space);
    int8_t new_kseps = inv_kseps - ((int_space - 1 - front) / 3 - ksep_cnt);
    if (numberflow->ksep != '\0') {
        x_offset_start_delta -= new_kseps * width_ksep;
    }

    /*Calculate number offsets, accumulating results*/
    int8_t rolling_offset = 0;

    int8_t rolling_dir;
    if (numberflow->dir == LV_NUMBERFLOW_DIR_DEFAULT || numberflow->dir == LV_NUMBERFLOW_DIR_CONTINUOUS) {
        rolling_dir = trend;
    }
    else if (numberflow->dir == LV_NUMBERFLOW_DIR_INCREASE) {
        rolling_dir = 1;
    }
    else if (numberflow->dir == LV_NUMBERFLOW_DIR_DECREASE) {
        rolling_dir = -1;
    }
    else {
        rolling_dir = 0;
    }

    for (int8_t i = 0; i < numberflow->digit_count; i++) {
        lv_opa_t opacity;
        int8_t num;
        // TODO: implement digit alignment
        if (i < first_visible_idx || i > last_visible_idx) {
            opacity = LV_OPA_TRANSP;
            num = 0;
        }
        else {
            opacity = LV_OPA_COVER;
            num = nums[i - first_visible_idx];
        }

        if (numberflow->dir == LV_NUMBERFLOW_DIR_CONTINUOUS) {
            if (rolling_dir == 0) {
                if (num > numberflow->digits[i]->last_num) {
                    rolling_dir = 1;
                }
                else if (num < numberflow->digits[i]->last_num) {
                    rolling_dir = -1;
                }
            }
        }

        int8_t delta_num = digit_roll(
            numberflow->digits[i],
            num + rolling_offset * numberflow->digits[i]->modulus,
            rolling_dir,
            opacity,
            numberflow->anim_path_flow,
            anim_time
        );

        if (numberflow->dir == LV_NUMBERFLOW_DIR_CONTINUOUS) {
            if (delta_num != 0) {
                if (rolling_offset == 0) {
                    /*Only update rolling offset when all higher-position did't changed*/
                    if (delta_num > 0) {
                        rolling_dir = 1;
                    }
                    else if (delta_num < 0) {
                        rolling_dir = -1;
                    }
                    rolling_offset = rolling_dir;
                }
            }
        }
    }

    /*Size Animation*/
    if(anim_time == 0) {
        numberflow->int_cnt = int_cnt;
        numberflow->dec_cnt = dec_cnt;
        ANIM_UPDATE_STOP(numberflow->width, width);
        ANIM_UPDATE_STOP(numberflow->x_ofs, x_offset_end);
        lv_anim_del(obj, anim_size);
        numberflow->anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;
        lv_obj_invalidate(obj);
        lv_obj_refresh_self_size(obj);
    }
    else {
        /*For non-monospace fonts, only update animation when width increase or digit
        * count change, or x_offset change, to prevent animation stuck at one place when fast updating.
        * For monospace fonts, width will be the same unless digit count change*/
        if (numberflow->flags & LV_NUMBERFLOW_FLAG_TIGHT
             || width > numberflow->width.end
             || int_cnt != numberflow->int_cnt
             || dec_cnt != numberflow->dec_cnt) {
            numberflow->int_cnt = int_cnt;
            numberflow->dec_cnt = dec_cnt;
            ANIM_UPDATE(numberflow->width, width);
            /*When a new digit is inserted on the left side, update the starting
            * value of the X offset so that the position of the original digit
            * remains unchanged*/
            numberflow->x_ofs.begin = numberflow->x_ofs.curr + x_offset_start_delta;
            numberflow->x_ofs.end = x_offset_end;
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, obj);
            lv_anim_set_exec_cb(&a, anim_size);
            lv_anim_set_values(&a, LV_NUMBERFLOW_ANIM_STATE_START, LV_NUMBERFLOW_ANIM_STATE_END);
            lv_anim_set_ready_cb(&a, anim_size_ready);
            lv_anim_set_time(&a, anim_time);
            lv_anim_set_path_cb(&a, numberflow->anim_path_flow);
            lv_anim_start(&a);
        }
    }
}
