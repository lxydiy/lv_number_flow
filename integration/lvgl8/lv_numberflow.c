/**
 * @file lv_numberflow.c
 *
 */
#include "lv_numberflow.h"

#define MY_CLASS &lv_numberflow_class

static void lv_numberflow_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_numberflow_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_numberflow_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void recalculate_style(lv_numberflow_t *widget);
static uint16_t get_glyph_width(numberflow_t *numberflow, char glyph);
static void animation_tick(lv_timer_t *timer);
static void refresh(lv_numberflow_t *widget);

const lv_obj_class_t lv_numberflow_class = {
    .constructor_cb = lv_numberflow_constructor,
    .destructor_cb = lv_numberflow_destructor,
    .event_cb = lv_numberflow_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_numberflow_t),
    .base_class = &lv_obj_class
};

lv_obj_t * lv_numberflow_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_numberflow_set_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    widget->core.now_ms = lv_tick_get();
    widget->core.anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    numberflow_set_value(&widget->core, value, anim == LV_ANIM_ON);
    refresh(widget);
}

void lv_numberflow_set_mode(lv_obj_t * obj, lv_numberflow_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_set_mode(&widget->core, mode);
}

void lv_numberflow_set_dir(lv_obj_t * obj, lv_numberflow_dir_t dir)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_set_dir(&widget->core, dir);
}

void lv_numberflow_set_format(lv_obj_t * obj, uint8_t int_fix, uint8_t dec_fix)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    widget->core.now_ms = lv_tick_get();
    widget->core.anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    numberflow_set_format(&widget->core, int_fix, dec_fix);
    refresh(widget);
}

void lv_numberflow_set_separators(lv_obj_t * obj, char ksep, char dsep)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    widget->core.now_ms = lv_tick_get();
    widget->core.anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    numberflow_set_separators(&widget->core, ksep, dsep);
    refresh(widget);
}

void lv_numberflow_set_modulus(lv_obj_t * obj, int8_t pos, uint8_t modulus)
{
    if (pos <= -LV_NUMBERFLOW_MAX_DIGITS || pos >= LV_NUMBERFLOW_MAX_DIGITS) return;
    if (modulus > 10) return;
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_set_modulus(&widget->core, pos, modulus);
}

void lv_numberflow_add_flag(lv_obj_t * obj, lv_numberflow_flag_t flags)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    widget->core.now_ms = lv_tick_get();
    widget->core.anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    numberflow_add_flag(&widget->core, flags);
    refresh(widget);
}

void lv_numberflow_clear_flag(lv_obj_t * obj, lv_numberflow_flag_t flags)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    widget->core.now_ms = lv_tick_get();
    widget->core.anim_time = lv_obj_get_style_anim_time(obj, LV_PART_MAIN);
    numberflow_clear_flag(&widget->core, flags);
    refresh(widget);
}

void lv_numberflow_set_anim_path_flow(lv_obj_t * obj, numberflow_anim_path_t path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_set_anim_path_flow(&widget->core, path);
}

void lv_numberflow_set_anim_path_size(lv_obj_t * obj, numberflow_anim_path_t path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_set_anim_path_size(&widget->core, path);
}

int32_t lv_numberflow_get_value(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    return numberflow_get_value(&widget->core);
}

numberflow_anim_path_t lv_numberflow_get_anim_path_flow(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    return numberflow_get_anim_path_flow(&widget->core);
}

numberflow_anim_path_t lv_numberflow_get_anim_path_size(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    return numberflow_get_anim_path_size(&widget->core);
}

void lv_numberflow_set_blur_data(lv_obj_t * obj, const lv_numberflow_blur_data_t * blur_data)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    if (numberflow->blur_data == blur_data) return;

    numberflow->blur_data = blur_data;

    recalculate_style(numberflow);
}

const lv_numberflow_blur_data_t * lv_numberflow_get_blur_data(const lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_numberflow_t * numberflow = (lv_numberflow_t *)obj;

    return numberflow->blur_data;
}

static uint16_t measure_glyph(void *userdata, char glyph)
{
    lv_numberflow_t *widget = userdata;
    return get_glyph_width(&widget->core, glyph);
}

static void lv_numberflow_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_init(&widget->core, widget, measure_glyph);
    widget->blur_data = NULL;
    lv_memset_00(widget->x_adv, sizeof(widget->x_adv));
    widget->core.height = lv_font_get_line_height(lv_obj_get_style_text_font(obj, LV_PART_MAIN));
    widget->core.line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
    widget->core.letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);
    widget->core.number_height = widget->core.height + widget->core.line_space;
    widget->core.now_ms = lv_tick_get();
    widget->timer = lv_timer_create(animation_tick, LV_DISP_DEF_REFR_PERIOD, widget);
    lv_timer_pause(widget->timer);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_numberflow_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    lv_timer_del(widget->timer);
}

static void refresh(lv_numberflow_t *widget)
{
    lv_obj_invalidate((lv_obj_t *)widget);
    lv_obj_refresh_self_size((lv_obj_t *)widget);
    if (numberflow_is_animating(&widget->core)) {
        lv_timer_resume(widget->timer);
    }
    else {
        lv_timer_pause(widget->timer);
    }
}

static void animation_tick(lv_timer_t *timer)
{
    lv_numberflow_t *widget = timer->user_data;
    bool changed = numberflow_update(&widget->core, lv_tick_get());
    if (changed)
        refresh(widget);
}

static inline const lv_nf_glyph_blur_dsc_t *get_blur(numberflow_t *numberflow, int8_t num, uint8_t blur_level)
{
    lv_numberflow_t *widget = numberflow->userdata;
    uint8_t blur_level_max = widget->blur_data->blur_level;
    return &widget->blur_data->blurs[num * blur_level_max + blur_level];
}

static inline bool digit_has_decimal_sep(numberflow_t *numberflow, int8_t idx)
{
    return idx == numberflow->ones_place_idx + 1;
}

static inline bool digit_has_thousand_sep(numberflow_t *numberflow, int8_t idx)
{
    int8_t integer_pos = numberflow->ones_place_idx - idx;
    if (integer_pos <= 0) return false;
    return integer_pos % 3 == 0 && numberflow->ksep != '\0';
}

static void draw_number(numberflow_t *numberflow, lv_draw_ctx_t *draw_ctx,
                        _nf_number_draw_dsc_t *num_draw_dsc)
{
    lv_numberflow_t *widget = numberflow->userdata;
    lv_obj_t *obj = (lv_obj_t *)widget;
    uint16_t width = get_glyph_width(numberflow, num_draw_dsc->num + '0');
    lv_color_t color = lv_obj_get_style_text_color(obj, LV_PART_MAIN);

    if (num_draw_dsc->opa == LV_OPA_TRANSP) return;

    if (widget->blur_data != NULL) {
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

        lv_draw_img_decoded(draw_ctx, &img_draw_dsc, &coords, widget->blur_data->glyph_blob + blur->start, LV_IMG_CF_ALPHA_8BIT);
    }
    else {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = color;
        label_dsc.font = widget->font;
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

static void draw_glyph(numberflow_t *numberflow, lv_draw_ctx_t *draw_ctx,
                        lv_point_t *pos_rel, lv_opa_t opa, const char glyph)
{
    lv_numberflow_t *widget = numberflow->userdata;
    if (glyph == '\0') return;
    lv_obj_t *obj = (lv_obj_t *)widget;
    lv_color_t color = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = color;
    label_dsc.font = widget->font;
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
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_t *numberflow = &widget->core;
    _nf_digit_t * digit;

    lv_draw_ctx_t * ctx = lv_event_get_draw_ctx(e);

    if (numberflow->sym_width.curr != 0) {
        /*Symbol's position is always top-left.*/
        lv_point_t pos_rel;
        pos_rel.x = 0;
        pos_rel.y = 0;
        draw_glyph(numberflow, ctx, &pos_rel, numberflow->sym_opa_old.curr, numberflow->sym_old);
        draw_glyph(numberflow, ctx, &pos_rel, numberflow->sym_opa_curr.curr, numberflow->sym_curr);
    }

    for (int32_t i = 0; i < numberflow->digit_count; i++)
    {
        digit = &numberflow->digits[i];
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

static uint16_t get_glyph_width(numberflow_t *numberflow, char glyph)
{
    lv_numberflow_t *widget = numberflow->userdata;
    if (glyph == '\0') return 0;
    if (glyph >= '0' && glyph <= '9') {
        /*Use cache to speed up layout as Arabic numerals don't require kerning,
        * especially for mono fonts, which is recommended to use in this widget.
        * Use '0' as letter_next because next letter is a number*/
        if (widget->blur_data != NULL) {
            return widget->blur_data->x_adv[glyph - '0'];
        }
        else {
            if (widget->x_adv[glyph - '0'] != 0) return widget->x_adv[glyph - '0'];
            uint16_t width = lv_font_get_glyph_width(widget->font, glyph, '0');
            widget->x_adv[glyph - '0'] = width;
            return width;
        }
    }
    else {
        /*Fall back to lv_font's get width.*/
        uint16_t width = lv_font_get_glyph_width(widget->font, glyph, '0');
        return width;
    }
}

static void reset_animations(lv_numberflow_t *numberflow)
{
    if (numberflow->core.digit_count != 0) {
        lv_numberflow_set_value((lv_obj_t *)numberflow, numberflow->core.value, LV_ANIM_OFF);
    }
}

static void recalculate_style(lv_numberflow_t *widget)
{
    lv_obj_t *obj = (lv_obj_t *)widget;
    numberflow_t *numberflow = &widget->core;

    /*Update line height and space*/
    const lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    lv_coord_t height;
    lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
    lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);

    if (widget->blur_data != NULL) {
        height = LV_MAX(widget->blur_data->line_height, lv_font_get_line_height(font));
    }
    else {
        height = lv_font_get_line_height(font);
    }

    if (font != widget->font || height != numberflow->height
        || line_space != numberflow->line_space || letter_space != numberflow->letter_space) {
        /*Reset animation due to line_height change*/
        reset_animations(widget);

        widget->font = font;
        numberflow->height = height;
        numberflow->line_space = line_space;
        numberflow->number_height = height + numberflow->line_space;
        numberflow->letter_space = letter_space;
        lv_memset_00(widget->x_adv, sizeof(widget->x_adv));

        /*Reset animation again with new line_height*/
        reset_animations(widget);
    }
    numberflow->blur_levels = widget->blur_data != NULL ? widget->blur_data->blur_level : 0;
    refresh(widget);
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
    lv_numberflow_t *widget = (lv_numberflow_t *)obj;
    numberflow_t *numberflow = &widget->core;

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
        numberflow_layout_size(numberflow);
        p->x = numberflow->width.curr + numberflow->sym_width.curr;
        p->y = numberflow->height;
    }
    else if (code == LV_EVENT_STYLE_CHANGED) {
        recalculate_style(widget);
    }
    else if(code == LV_EVENT_DRAW_MAIN) {
        numberflow_layout(numberflow);
        if (numberflow->digit_count > 0) {
            draw_numberflow(e);
        }
    }
}
