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
#define LV_NUMBERFLOW_ANIM_STATE_END   256

/** Mark no animation is in progress*/
#define LV_NUMBERFLOW_ANIM_STATE_INV   -1

/** log2(LV_NUMBERFLOW_ANIM_STATE_END) used to normalize data*/
#define LV_NUMBERFLOW_ANIM_STATE_NORM  8

/** maximum value in number_flow_ease_lut*/
#define LV_NUMBERFLOW_ANIM_RESOLUTION 1024

/** log2(LV_NUMBERFLOW_ANIM_RESOLUTION) used to normalize data*/
#define LV_NUMBERFLOW_ANIM_RES_SHIFT 10

/** Element count of number_flow_ease_lut*/
#define LV_NUMBERFLOW_ANIM_LUT_SIZE 90

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_numberflow_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_numberflow_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static int32_t lv_numberflow_ease_curve(const lv_anim_t * anim);
static void lv_numberflow_event(const lv_obj_class_t * class_p, lv_event_t * e);
static uint16_t lv_nf_get_number_width(lv_numberflow_t *numberflow, uint8_t number);
static void lv_numberflow_set_value_with_anim(lv_obj_t * obj, int32_t new_value, lv_anim_enable_t en);

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

static const uint16_t number_flow_ease_lut[90] = {
    0,     5,    19,    40,    68,    98,   132,   169,   207,   246,   285,   324,   362,   399,   436,   472,
  506,   539,   570,   600,   629,   655,   681,   706,   728,   749,   769,   787,   805,   821,   837,   851,
  864,   877,   888,   898,   908,   918,   926,   934,   941,   947,   953,   959,   965,   970,   974,   978,
  982,   985,   988,   991,   994,   996,   999,  1001,  1004,  1005,  1007,  1008,  1010,  1011,  1012,  1013,
 1014,  1015,  1016,  1016,  1017,  1018,  1018,  1019,  1019,  1020,  1020,  1020,  1021,  1021,  1021,  1021,
 1022,  1022,  1022,  1022,  1022,  1022,  1023,  1023,  1023,  1024
};

/**********************
 *      MACROS
 **********************/

#define ANIMATION_LERP(state, minval, maxval) \
    ( LV_NUMBERFLOW_ANIM_STATE_INV != (state) ? \
    ((minval) + ((((maxval) - (minval)) * (state)) >> LV_NUMBERFLOW_ANIM_STATE_NORM)) \
    : (maxval))

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

    if(numberflow->value == value) return;
    numberflow->value = value;
    lv_numberflow_set_value_with_anim(obj, value, anim);
}

void lv_numberflow_set_anim_path(lv_obj_t * obj, lv_anim_path_cb_t path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (path == NULL) {
        numberflow->anim_path = lv_numberflow_ease_curve;
    }
    else {
        numberflow->anim_path = path;
    }
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

lv_anim_path_cb_t lv_numberflow_get_anim_path(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    return numberflow->anim_path;
}
/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_numberflow_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;
    numberflow->anim_path = lv_numberflow_ease_curve;
    numberflow->glyph_dsc = NULL;
    numberflow->glyph_blob = NULL;
    numberflow->max_blur_level = 0;
    numberflow->y_offset = 0;
    numberflow->line_space = lv_obj_get_style_text_line_space(obj, 0);
    numberflow->letter_space = lv_obj_get_style_text_letter_space(obj, 0);

    numberflow->value = -1;
    numberflow->digit_count = 0;
    numberflow->digits = NULL;
    lv_memset_00(numberflow->digits_ptr_static, sizeof(numberflow->digits_ptr_static));
    numberflow->height = lv_font_get_line_height(lv_obj_get_style_text_font(obj, LV_PART_MAIN));
    numberflow->number_height = numberflow->height + numberflow->line_space;

    numberflow->visible_digit_cnt_prev = 0;

    numberflow->start_content_width = 0;
    numberflow->target_content_width = 0;
    numberflow->current_content_width = 0;

    numberflow->start_x_offset = 0;
    numberflow->target_x_offset = 0;
    numberflow->current_x_offset = 0;

    numberflow->anim_state = 0;

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
    
    if(numberflow->digits != NULL) {
        for (int32_t i = 0; i < numberflow->digit_count; i++)
        {
            lv_anim_del(&numberflow->digits[i], NULL);
        }
        lv_anim_del(numberflow, NULL);
        lv_mem_free(numberflow->digits);
    }
}

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

static uint16_t draw_number(lv_numberflow_t *numberflow, lv_draw_ctx_t *draw_ctx, int32_t number,
                        lv_coord_t x, lv_coord_t y, uint16_t curr_width, lv_opa_t opa)
{
    lv_obj_t *obj = (lv_obj_t *)numberflow;

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
    label_dsc.font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    label_dsc.opa = opa;

    lv_point_t pos;
    lv_area_t coords;
    lv_obj_get_content_coords(obj, &coords);

    pos.x = coords.x1 + x;
    pos.y = coords.y1 + y;

    uint16_t width = lv_nf_get_number_width(numberflow, number);

    /*Compensate for the horizontal position of non-monospace fonts to make them
    * display centered*/
    pos.x += (curr_width - width + 1) / 2;

    if (opa > LV_OPA_TRANSP)
        lv_draw_letter(draw_ctx, &label_dsc, &pos, number + '0');

    return width;
}

static uint16_t draw_digit(lv_draw_ctx_t * ctx, _lv_nf_digit_t *digit, lv_coord_t x)
{
    _lv_nf_slide_dsc_t* slide_dsc = &digit->slide_dsc;
    lv_numberflow_t *numberflow = (lv_numberflow_t *)digit->anim.numberflow;
    lv_obj_t *obj = (lv_obj_t *)numberflow;

    int32_t value = digit->anim.anim_state;

    if (value == LV_NUMBERFLOW_ANIM_STATE_INV) {
        value = LV_NUMBERFLOW_ANIM_STATE_END;
    }

    lv_coord_t height = numberflow->number_height;
    lv_coord_t total_height = height * digit->modulus;
    lv_coord_t center = ANIMATION_LERP(value, slide_dsc->start_px, slide_dsc->end_px);

    // 计算当前屏幕中心的数字
    int center_mod = center % total_height;
    if (center_mod < 0) center_mod += total_height;

    int upper_num = center_mod / height;
    int upper_off = -(center_mod % height);
    int lower_num = (upper_num + 1) % digit->modulus;
    int lower_off = upper_off + height;

    int upper_opa = LV_OPA_COVER + upper_off * LV_OPA_COVER / height;
    int lower_opa = LV_OPA_COVER - upper_opa;

    // 动态计算模糊度
    int blur_pos;
    if (value != LV_NUMBERFLOW_ANIM_STATE_END) {
        blur_pos = LV_ABS(center - slide_dsc->last_px[0]);
        blur_pos = blur_pos * 2 / 3;
        if (blur_pos >= 1) {
            blur_pos -= 1;
        }
        if (blur_pos > numberflow->max_blur_level) {
            blur_pos = numberflow->max_blur_level;
        }
    }
    else {
        // 最后一帧一定是不模糊的
        blur_pos = 0;
    }

    slide_dsc->last_px[1] = slide_dsc->last_px[0];
    slide_dsc->last_px[0] = center;

    // 动态计算全局不透明度
    int digit_opa = ANIMATION_LERP(value, slide_dsc->start_opacity, slide_dsc->end_opacity);
    if (digit_opa > LV_OPA_COVER) {
        digit_opa = LV_OPA_COVER;
    }
    slide_dsc->last_opacity = digit_opa;

    slide_dsc->curr_width = ANIMATION_LERP(digit->anim.anim_state,
                                           slide_dsc->start_width,
                                           slide_dsc->end_width);

    draw_number(numberflow, ctx, upper_num, x, upper_off, slide_dsc->curr_width, (upper_opa * digit_opa) / LV_OPA_COVER);
    draw_number(numberflow, ctx, lower_num, x, lower_off, slide_dsc->curr_width, (lower_opa * digit_opa) / LV_OPA_COVER);

    return slide_dsc->curr_width;
}

static void draw_numberflow(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

    int32_t x_offset = ANIMATION_LERP(numberflow->anim_state, numberflow->start_x_offset, numberflow->target_x_offset);
    numberflow->current_x_offset = x_offset;

    /*We need to count invisible digits's width*/
    for (int32_t i = 0; i < numberflow->digit_count; i++)
    {
        x_offset += draw_digit(draw_ctx, &numberflow->digits[i], x_offset);
        x_offset += numberflow->letter_space;
    }
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
        if(numberflow->current_x_offset < 0)
            *s = LV_MAX(*s, -numberflow->current_x_offset);
        else
            *s = LV_MAX(*s, numberflow->current_x_offset + numberflow->current_content_width);
    }
    else if(code == LV_EVENT_GET_SELF_SIZE) {
        lv_point_t * p = lv_event_get_param(e);
        p->x = ANIMATION_LERP(numberflow->anim_state, numberflow->start_content_width, numberflow->target_content_width);
        p->y = numberflow->height;
        numberflow->current_content_width = p->x;
    }
    else if (code == LV_EVENT_STYLE_CHANGED)
    {
        /*Update line height and space*/
        lv_coord_t height = lv_font_get_line_height(lv_obj_get_style_text_font(obj, LV_PART_MAIN));
        lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
        lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);

        if (height != numberflow->height || line_space != numberflow->line_space || letter_space != numberflow->letter_space) {
            /*Reset animation due to line_height change*/
            if (numberflow->digit_count != 0) {
                lv_numberflow_set_value_with_anim(obj, numberflow->value, LV_ANIM_OFF);
            }

            numberflow->height = height;
            numberflow->line_space = line_space;
            numberflow->number_height = height + numberflow->line_space;
            numberflow->letter_space = letter_space;

            /*Reset animation again with new line_height*/
            if (numberflow->digit_count != 0) {
                lv_numberflow_set_value_with_anim(obj, numberflow->value, LV_ANIM_OFF);
            }
        }
    }
    
    else if(code == LV_EVENT_DRAW_MAIN) {
        draw_numberflow(e);
    }
}

static uint16_t lv_nf_get_number_width(lv_numberflow_t *numberflow, uint8_t number)
{
    // TODO: implement number width buffer.
    const lv_font_t *font = lv_obj_get_style_text_font((lv_obj_t *)numberflow, LV_PART_MAIN);
    return lv_font_get_glyph_width(font, number + '0', '\0');
}

static void lv_nf_digit_anim(void * var, int32_t value)
{
    _lv_nf_digit_t * digit = var;

    digit->anim.anim_state = value;

    lv_obj_invalidate(digit->anim.numberflow);
}

static void lv_nf_digit_anim_ready(lv_anim_t * a)
{
    _lv_nf_digit_t * digit = a->var;

    digit->anim.anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;

    lv_obj_invalidate(digit->anim.numberflow);
}

static void lv_nf_digit_anim_start(lv_anim_t * a)
{
    _lv_nf_digit_t * digit = a->var;
    _lv_nf_slide_dsc_t* slide_dsc = &digit->slide_dsc;
    _lv_nf_slide_start_dsc_t* slide_start_dsc = &digit->slide_start_dsc;
    lv_numberflow_t *numberflow = (lv_numberflow_t *)digit->anim.numberflow;

    lv_coord_t line_height = numberflow->number_height;
    lv_coord_t total_height = line_height * digit->modulus;

    // 移除accumulate方式产生的多余的循环坐标
    lv_coord_t end_px = slide_dsc->end_px;
    lv_coord_t start_px = slide_dsc->last_px[0];
    lv_coord_t last_frame_px = slide_dsc->last_px[1];
    lv_coord_t start_px_overflow = (start_px / total_height) * total_height;
    start_px -= start_px_overflow;
    end_px -= start_px_overflow;
    last_frame_px -= start_px_overflow;

    // 更新不透明度
    slide_dsc->start_opacity = slide_dsc->last_opacity;
    slide_dsc->end_opacity = slide_start_dsc->target_opa;

    // 更新位移信息
    slide_dsc->start_px = start_px;
    slide_dsc->end_px = end_px + slide_start_dsc->delta_px;
    slide_dsc->last_px[0] = last_frame_px;  // 回溯速度，避免切换时模糊计算错误
    slide_dsc->last_px[1] = last_frame_px;
    slide_dsc->last_num = slide_start_dsc->target_num % digit->modulus;
    if (slide_dsc->last_num < 0) slide_dsc->last_num += digit->modulus;

    // 避免abs(end-start)超过2整圈以影响观感
    lv_coord_t remaining = slide_dsc->end_px - slide_dsc->start_px;
    lv_coord_t remaining_overflow = 0;
    if (remaining > total_height) 
        remaining_overflow = (int)((remaining - total_height) / total_height) * total_height;
    else if (remaining < -total_height) 
        remaining_overflow = (int)((remaining + total_height) / total_height) * total_height;

    slide_dsc->end_px -= remaining_overflow;

    digit->anim.anim_state = LV_NUMBERFLOW_ANIM_STATE_START;

    // 更新目标宽度
    digit->slide_dsc.start_width = digit->slide_dsc.curr_width;
    digit->slide_dsc.end_width = lv_nf_get_number_width(
                                        (lv_numberflow_t *)digit->anim.numberflow,
                                        digit->slide_dsc.last_num);

    lv_obj_invalidate(digit->anim.numberflow);
}

static void lv_nf_size_anim(void * var, int32_t value)
{
    lv_obj_t * obj = (lv_obj_t *)var;
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;
    numberflow->anim_state = value;

    lv_obj_invalidate(obj);
    lv_obj_refresh_self_size(obj);
}

static void lv_nf_size_anim_ready(lv_anim_t * a)
{
    lv_obj_t * obj = (lv_obj_t *)a->var;
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    numberflow->anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;

    lv_obj_invalidate(obj);
    lv_obj_refresh_self_size(obj);
}

int32_t lv_nf_roll_digit(_lv_nf_digit_t* digit, int32_t target_num, int32_t direction,
                         int32_t target_opa, lv_anim_path_cb_t path, uint32_t time)
{
    _lv_nf_slide_dsc_t* slide_dsc = &digit->slide_dsc;
    lv_numberflow_t *numberflow = (lv_numberflow_t *)digit->anim.numberflow;

    lv_coord_t height = numberflow->number_height;
    int32_t round_height = height * digit->modulus;
    int last_num = slide_dsc->last_num;
    // 计算本次滚动距离
    int delta_px = (target_num - last_num) * height;
    if (direction == 1) {
        /*target > last, otherwise rolling around*/
        if (delta_px < 0)
            delta_px += round_height;
    }
    else if (direction == -1) {
        /*target < last, otherwise rolling around*/
        if (delta_px > 0)
            delta_px -= round_height;
    }
    else {
        /*nearest*/
        delta_px = delta_px % round_height;
        if (delta_px > round_height / 2) delta_px -= round_height;
        if (delta_px < -round_height / 2) delta_px += round_height;
    }

    if (time == 0) {
        lv_anim_del(digit, lv_nf_digit_anim);
        digit->anim.anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;
        lv_obj_invalidate((lv_obj_t *)numberflow);
        lv_obj_refresh_self_size((lv_obj_t *)numberflow);

        /*Remove redundant loop coordinates generated by the accumulate method
        * or lv_coord_t will overflow*/
        lv_coord_t end_px = digit->slide_dsc.end_px + delta_px;
        lv_coord_t start_px = slide_dsc->last_px[0];
        lv_coord_t start_px_overflow = (start_px / round_height) * round_height;
        end_px -= start_px_overflow;

        digit->slide_dsc.end_px = end_px;
        digit->slide_dsc.start_px = end_px;
        digit->slide_dsc.last_px[0] = end_px;
        digit->slide_dsc.last_px[1] = end_px;

        digit->slide_dsc.end_opacity = target_opa;
        digit->slide_dsc.start_opacity = target_opa;
        digit->slide_dsc.last_opacity = target_opa;
        digit->slide_dsc.last_num = (target_num + digit->modulus) % digit->modulus;
        digit->slide_dsc.end_width = lv_nf_get_number_width(numberflow, digit->slide_dsc.last_num);
        digit->slide_dsc.start_width = digit->slide_dsc.end_width;
        digit->slide_dsc.curr_width = digit->slide_dsc.end_width;
    }
    else {
        if (delta_px == 0 && target_opa == slide_dsc->end_opacity) {
            /*target didn't changed so we don't need to animate*/
            return 0;
        }
        digit->slide_start_dsc.delta_px = delta_px;
        digit->slide_start_dsc.target_num = target_num;
        digit->slide_start_dsc.target_opa = target_opa;

        lv_anim_t a;

        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, lv_nf_digit_anim);
        lv_anim_set_time(&a, time);
        lv_anim_set_path_cb(&a, path);

        lv_anim_set_values(&a, LV_NUMBERFLOW_ANIM_STATE_START, LV_NUMBERFLOW_ANIM_STATE_END);
        lv_anim_set_var(&a, digit);

        lv_anim_set_ready_cb(&a, lv_nf_digit_anim_ready);

        /*Using 'accumulate' mode, we need to update slide_dsc just before the
        * animation start for smooth transition*/
        lv_anim_set_start_cb(&a, lv_nf_digit_anim_start);
        lv_anim_set_early_apply(&a, false);

        lv_anim_start(&a);
    }
    return target_num - last_num;
}

static void lv_nf_init_digit(lv_numberflow_t *numberflow, _lv_nf_digit_t *digit)
{
    lv_memset_00(digit, sizeof(_lv_nf_digit_t));
    digit->modulus = 10;
    digit->anim.numberflow = (lv_obj_t *)numberflow;
    digit->slide_dsc.curr_width = lv_nf_get_number_width(numberflow, 0);
    digit->slide_dsc.start_width = digit->slide_dsc.curr_width;
}

static bool lv_nf_realloc_digit(lv_numberflow_t *numberflow, int32_t count, bool push_back)
{
    if (numberflow->digit_count < count) {
        lv_anim_t *a[LV_NUMBERFLOW_MAX_DIGITS];
        lv_memset_00(a, sizeof(a));

        /*Store old animation pointer to change their var later*/
        for (uint8_t i = 0; i < numberflow->digit_count; i++)
        {
            a[i] = lv_anim_get(&numberflow->digits[i], lv_nf_digit_anim);
        }

        numberflow->digits = lv_mem_realloc(numberflow->digits, count * sizeof(_lv_nf_digit_t));
        if (numberflow->digits == NULL) {
            numberflow->digit_count = 0;
            numberflow->visible_digit_cnt_prev = 0;
            return false;
        }

        int32_t delta = count - numberflow->digit_count;
        if (push_back == false) {
            /*Move old digits to the back*/
            for (int32_t i = numberflow->digit_count - 1; i >= 0; i--) {
                numberflow->digits[i + delta] = numberflow->digits[i];
            }

            for (int32_t i = 0; i < delta; i++) {
                lv_nf_init_digit(numberflow, &numberflow->digits[i]);
            }
            for (uint8_t i = 0; i < numberflow->digit_count; i++) {
                if (a[i] != NULL) {
                    lv_anim_set_var(a[i], &numberflow->digits[i + delta]);
                }
            }
        }
        else {
            for (int32_t i = numberflow->digit_count; i < count; i++) {
                lv_nf_init_digit(numberflow, &numberflow->digits[i]);
            }
            for (uint8_t i = 0; i < numberflow->digit_count; i++) {
                if (a[i] != NULL) {
                    lv_anim_set_var(a[i], &numberflow->digits[i]);
                }
            }
        }

        numberflow->digit_count = count;
    }
    return true;
}

static void lv_numberflow_set_value_with_anim(lv_obj_t * obj, int32_t new_value, lv_anim_enable_t en)
{
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    int32_t nums[LV_NUMBERFLOW_MAX_DIGITS] = {0};
    int32_t digit_count_delta;
    int32_t visible_digit_cnt = 0;
    uint32_t anim_time;

    if(en == LV_ANIM_OFF) {
        anim_time = 0;
    }
    else {
        anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    }

    /*Only supports displaying positive numbers here*/
    if (new_value < 0) new_value = -new_value;
    if (new_value == 0)
        visible_digit_cnt = 1;
    while (new_value > 0) {
        nums[visible_digit_cnt] = new_value % 10;
        new_value /= 10;
        visible_digit_cnt++;
    }

    // TODO: implement digit alignment
    digit_count_delta = numberflow->digit_count;
    lv_nf_realloc_digit(numberflow, visible_digit_cnt, false);
    digit_count_delta = numberflow->digit_count - digit_count_delta;
    if (numberflow->digit_count == 0) return;

    /*align nums with digits*/
    for (int32_t i = 0; i < (numberflow->digit_count + 1) / 2; i++)
    {
        int32_t peer = numberflow->digit_count - 1 - i;
        int32_t t;
        t = nums[i];
        nums[i] = nums[peer];
        nums[peer] = t;
    }

    int32_t width_end = numberflow->letter_space * (numberflow->digit_count - 1);
    int32_t x_offset_start_delta = 0;
    int32_t x_offset_end = 0;
    uint16_t width0 = lv_nf_get_number_width(numberflow, 0);

    for (int32_t i = 0; i < digit_count_delta; i++)
    {
        numberflow->digits[i].slide_dsc.start_width = width0;
    }
    
    x_offset_start_delta = -digit_count_delta * width0;
    for (int32_t i = numberflow->digit_count - visible_digit_cnt; i < numberflow->digit_count; ++i) {
        width_end += lv_nf_get_number_width(numberflow, nums[i]);
    }
    x_offset_end = -((numberflow->digit_count - visible_digit_cnt) * width0);

    /*Calculate number offsets, accumulating results*/
    int32_t rolling_dir = 0;
    int32_t rolling_offset = 0;

    /*First, if visible digits count changed, we should force rolling direction. 
    * For example: 999 -> 1000 or 123 -> 12*/
    if (visible_digit_cnt > numberflow->visible_digit_cnt_prev) {
        rolling_dir = 1;
    }
    else if (visible_digit_cnt < numberflow->visible_digit_cnt_prev) {
        rolling_dir = -1;
    }

    for (int32_t i = 0; i < numberflow->digit_count; i++) {
        lv_opa_t opacity;
        int32_t num;
        // TODO: implement digit alignment
        if (i < numberflow->digit_count - visible_digit_cnt) {
            opacity = LV_OPA_TRANSP;
        }
        else {
            opacity = LV_OPA_COVER;
        }
        num = nums[i];

        if (rolling_dir == 0) {
            if (num > numberflow->digits[i].slide_dsc.last_num)
                rolling_dir = 1;
            else if (num < numberflow->digits[i].slide_dsc.last_num)
                rolling_dir = -1;
        }

        int32_t delta_num = lv_nf_roll_digit(
            &numberflow->digits[i],
            num + rolling_offset * numberflow->digits[i].modulus,
            rolling_dir,
            opacity,
            numberflow->anim_path,
            anim_time
        );

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

    numberflow->visible_digit_cnt_prev = visible_digit_cnt;

    /*Size Animation*/
    if(anim_time == 0) {
        numberflow->start_content_width = width_end;
        numberflow->target_content_width = width_end;
        numberflow->start_x_offset = x_offset_end;
        numberflow->target_x_offset = x_offset_end;
        lv_anim_del(obj, lv_nf_size_anim);
        numberflow->anim_state = LV_NUMBERFLOW_ANIM_STATE_INV;
        lv_obj_invalidate(obj);
        lv_obj_refresh_self_size(obj);
    }
    else {
        /*For non-monospace fonts, only update animation when width increase or digit
        * count change, to prevent animation stuck at one place when fast updating.
        * For monospace fonts, width will be the same unless digit count change.*/
        if (width_end > numberflow->target_content_width || x_offset_start_delta != 0 || numberflow->target_x_offset != x_offset_end) {
            numberflow->start_content_width = numberflow->current_content_width;
            numberflow->target_content_width = width_end;
            numberflow->start_x_offset = numberflow->current_x_offset + x_offset_start_delta;
            numberflow->target_x_offset = x_offset_end;
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, obj);
            lv_anim_set_exec_cb(&a, lv_nf_size_anim);
            lv_anim_set_values(&a, LV_NUMBERFLOW_ANIM_STATE_START, LV_NUMBERFLOW_ANIM_STATE_END);
            lv_anim_set_ready_cb(&a, lv_nf_size_anim_ready);
            lv_anim_set_time(&a, anim_time);
            lv_anim_set_path_cb(&a, numberflow->anim_path);
            lv_anim_start(&a);
        }
    }
    numberflow->visible_digit_cnt_prev = visible_digit_cnt;
}
