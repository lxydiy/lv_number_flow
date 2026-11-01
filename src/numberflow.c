#include "numberflow.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define NF_ABS(x) ((x) > 0 ? (x) : -(x))
#define NF_MAX(a, b) ((a) > (b) ? (a) : (b))

/** Mark no animation is in progress*/
#define NUMBERFLOW_ANIM_STATE_INV   -1

/** Calculate current animation value*/
#define ANIM_LERP(state, dsc) \
    ( NUMBERFLOW_ANIM_STATE_INV != (state) ? \
    ((dsc.begin) + ((((dsc.end) - (dsc.begin)) * (state)) >> NUMBERFLOW_ANIM_STATE_NORM)) \
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
 *  STATIC PROTOTYPES
 **********************/
static int8_t expo_pos_to_digit_index(numberflow_t *numberflow, int8_t pos);
static uint8_t digit_index_to_mod_index(numberflow_t *numberflow, int8_t idx);
static uint16_t get_glyph_width(numberflow_t *numberflow, char glyph);
static void numberflow_update_with_anim(numberflow_t *numberflow, int8_t *nums,
                                        int8_t int_cnt, int8_t dec_cnt,
                                        int8_t trend, int8_t symbol, bool en);
static void anim_schedule(_nf_anim_state_t *anim, numberflow_t *numberflow,
                           numberflow_anim_path_t path, uint32_t duration);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void numberflow_set_value(numberflow_t *numberflow, int32_t value, bool anim)
{
    assert(numberflow != NULL);

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
    numberflow->value = value;

    int8_t symbol;
    if (value > 0) {
        symbol = 1;
    }
    else if (value < 0) {
        value = -value;
        symbol = -1;
    }
    else {
        symbol = 0;
    }

    char nums[NUMBERFLOW_MAX_DIGITS];
    memset(nums, 0, sizeof(nums));

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

    int8_t int_cnt = nums_idx - numberflow->dec_fix;
    if (int_cnt < 1) {
        int_cnt = 1;
    }
    if (int_cnt < numberflow->int_fix) {
        int_cnt = numberflow->int_fix;
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
    if (numberflow->flags & NUMBERFLOW_FLAG_TRIM_ZEROS) {
        for (int8_t i = nums_idx - 1; i >= nums_idx - numberflow->dec_fix; i--) {
            if (nums[i] == 0) {
                dec_cnt--;
            }
            else {
                break;
            }
        }
    }

    numberflow_update_with_anim(numberflow, nums, int_cnt, dec_cnt, trend, symbol, anim);
}

void numberflow_set_mode(numberflow_t *numberflow, numberflow_mode_t mode)
{
    assert(numberflow != NULL);

    numberflow->mode = mode;
}

void numberflow_set_dir(numberflow_t *numberflow, numberflow_dir_t dir)
{
    assert(numberflow != NULL);

    numberflow->dir = dir;
}

void numberflow_set_format(numberflow_t *numberflow, uint8_t int_fix, uint8_t dec_fix)
{
    assert(numberflow != NULL);

    if (dec_fix > NUMBERFLOW_MAX_DIGITS) {
        dec_fix = NUMBERFLOW_MAX_DIGITS;
    }
    if (int_fix + dec_fix > NUMBERFLOW_MAX_DIGITS) {
        int_fix = NUMBERFLOW_MAX_DIGITS - dec_fix;
    }

    numberflow->int_fix = int_fix;
    numberflow->dec_fix = dec_fix;
    numberflow_set_value(numberflow, numberflow->value, true);
}

void numberflow_set_separators(numberflow_t *numberflow, char ksep, char dsep)
{
    assert(numberflow != NULL);

    if (dsep == '\0') dsep = '.';

    numberflow->ksep = ksep;
    numberflow->dsep = dsep;

    numberflow_set_value(numberflow, numberflow->value, true);
}

void numberflow_set_modulus(numberflow_t *numberflow, int8_t pos, uint8_t modulus)
{
    if (pos <= -NUMBERFLOW_MAX_DIGITS || pos >= NUMBERFLOW_MAX_DIGITS) return;
    if (modulus > 10) return;
    assert(numberflow != NULL);

    numberflow->modulus[pos + NUMBERFLOW_MAX_DIGITS] = modulus;
    int8_t idx = expo_pos_to_digit_index(numberflow, pos);
    if (idx >= 0 && idx < numberflow->digit_count) {
        numberflow->digits[idx].modulus = modulus;
    }
}

void numberflow_add_flag(numberflow_t *numberflow, numberflow_flag_t flags)
{
    assert(numberflow != NULL);

    numberflow->flags |= flags;
    numberflow_set_value(numberflow, numberflow->value, true);
}

void numberflow_clear_flag(numberflow_t *numberflow, numberflow_flag_t flags)
{
    assert(numberflow != NULL);

    numberflow->flags &= ~flags;
    numberflow_set_value(numberflow, numberflow->value, true);
}

void numberflow_set_anim_path_flow(numberflow_t *numberflow, numberflow_anim_path_t path)
{
    assert(numberflow != NULL);

    if (path == NULL) {
        numberflow->anim_path_flow = numberflow_anim_path_flow;
    }
    else {
        numberflow->anim_path_flow = path;
    }
}

void numberflow_set_anim_path_size(numberflow_t *numberflow, numberflow_anim_path_t path)
{
    assert(numberflow != NULL);

    if (path == NULL) {
        numberflow->anim_path_size = numberflow_anim_path_ease_out;
    }
    else {
        numberflow->anim_path_size = path;
    }
}

int32_t numberflow_get_value(const numberflow_t *numberflow)
{
    assert(numberflow != NULL);

    return numberflow->value;
}

numberflow_anim_path_t numberflow_get_anim_path_flow(const numberflow_t *numberflow)
{
    assert(numberflow != NULL);

    return numberflow->anim_path_flow;
}

numberflow_anim_path_t numberflow_get_anim_path_size(const numberflow_t *numberflow)
{
    assert(numberflow != NULL);

    return numberflow->anim_path_size;
}

void numberflow_init(numberflow_t *numberflow, void *userdata,
                     uint16_t (*get_width)(void *userdata, char glyph))
{
    assert(numberflow != NULL);
    assert(get_width != NULL);
    memset(numberflow, 0, sizeof(*numberflow));
    numberflow->userdata = userdata;
    numberflow->get_glyph_width = get_width;
    numberflow->mode = NUMBERFLOW_MODE_NORMAL;
    numberflow->dir = NUMBERFLOW_DIR_DEFAULT;
    numberflow->anim_path_flow = numberflow_anim_path_flow;
    numberflow->anim_path_size = numberflow_anim_path_ease_out;
    numberflow->dsep = '.';
    numberflow->flags = NUMBERFLOW_FLAG_FADE_VERTICAL;
    for (int8_t i = 0; i < sizeof(numberflow->modulus) / sizeof(numberflow->modulus[0]); i++) {
        numberflow->modulus[i] = 10;
    }
    numberflow->ones_place_idx = -1;
    numberflow->anim.anim_state = NUMBERFLOW_ANIM_STATE_INV;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
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

static inline int8_t expo_pos_to_digit_index(numberflow_t *numberflow, int8_t pos)
{
    int8_t idx = numberflow->ones_place_idx - pos;
    return idx;
}

static inline uint8_t digit_index_to_mod_index(numberflow_t *numberflow, int8_t idx)
{
    int8_t pos = numberflow->ones_place_idx - idx;
    return pos + NUMBERFLOW_MAX_DIGITS;
}

static uint16_t layout_digit(numberflow_t *numberflow, _nf_digit_t *digit, int32_t x)
{
    if (digit->anim.updated == false) {
        /*Digit animation didn't update. Only update x and return old width*/
        digit->draw[0].x = x;
        digit->draw[1].x = x;
        return digit->draw[0].curr_width;
    }
    digit->anim.updated = false;

    int32_t value = digit->anim.anim_state;

    if (value == NUMBERFLOW_ANIM_STATE_INV) {
        value = NUMBERFLOW_ANIM_STATE_END;
    }

    int32_t height = numberflow->number_height;
    int32_t total_height = height * digit->modulus;
    int32_t center = ANIM_LERP(value, digit->flow);

    /*Calculate current number*/
    int center_mod = center % total_height;
    if (center_mod < 0) center_mod += total_height;

    int lower_num, upper_num;
    int lower_off, upper_off;
    int lower_opa, upper_opa;

    if (numberflow->mode == NUMBERFLOW_MODE_NORMAL) {
        upper_num = center_mod / height;
        upper_off = -(center_mod % height);
        lower_num = (upper_num + 1) % digit->modulus;
        lower_off = upper_off + height;

        if (numberflow->flags & NUMBERFLOW_FLAG_FADE_VERTICAL) {
            upper_opa = MAP(upper_off, 0, -height, 255, 0);
            lower_opa = 255 - upper_opa;
        }
        else {
            lower_opa = 255;
            upper_opa = 255;
        }
    }
    else {
        lower_num = center_mod / height;
        lower_off = center_mod % height;
        upper_num = (lower_num + 1) % digit->modulus;
        upper_off = lower_off - height;

        if (numberflow->flags & NUMBERFLOW_FLAG_FADE_VERTICAL) {
            lower_opa = MAP(lower_off, 0, height, 255, 0);
            upper_opa = 255 - lower_opa;
        }
        else {
            lower_opa = 255;
            upper_opa = 255;
        }
    }

    int8_t blur_pos = 0;
    if (numberflow->blur_levels != 0) {
        /*Calculate blur based on flowing position difference*/
        if (value != NUMBERFLOW_ANIM_STATE_END) {
            blur_pos = center - digit->flow.curr;
            blur_pos = NF_ABS(blur_pos) / 2;
            if (blur_pos >= 1) {
                blur_pos -= 1;
            }
            if (blur_pos >= numberflow->blur_levels) {
                blur_pos = numberflow->blur_levels - 1;
            }
        }
        else {
            /*There shouldn't be any blur at the last frame*/
            blur_pos = 0;
        }
    }

    int16_t digit_opa = ANIM_LERP(value, digit->opa);
    if (digit_opa > 255) {
        digit_opa = 255;
    }
    digit->opa.curr = (uint8_t)digit_opa;
    digit->width.curr = ANIM_LERP(digit->anim.anim_state, digit->width);
    digit->last_flow = digit->flow.curr;
    digit->flow.curr = center;

    digit->draw[0].num = upper_num;
    digit->draw[0].x = x;
    digit->draw[0].y = upper_off;
    digit->draw[0].curr_width = digit->width.curr;
    digit->draw[0].opa = (upper_opa * digit_opa) / 255;
    digit->draw[0].blur_level = blur_pos;

    digit->draw[1].num = lower_num;
    digit->draw[1].x = x;
    digit->draw[1].y = lower_off;
    digit->draw[1].curr_width = digit->width.curr;
    digit->draw[1].opa = (lower_opa * digit_opa) / 255;
    digit->draw[1].blur_level = blur_pos;

    return digit->width.curr;
}

void numberflow_layout(numberflow_t *numberflow)
{
    assert(numberflow != NULL);
    numberflow_layout_size(numberflow);

    int32_t x_ofs = ANIM_LERP(numberflow->anim.anim_state, numberflow->x_ofs);
    numberflow->x_ofs.curr = x_ofs;

    x_ofs += numberflow->sym_width.curr;
    /*We need to count invisible digits's width*/
    for (int32_t i = 0; i < numberflow->digit_count; i++)
    {
        if (digit_has_decimal_sep(numberflow, i)) {
            /*Decimal separator's animation is bound to the first decimal digit*/
            x_ofs += get_glyph_width(numberflow, numberflow->dsep);
            x_ofs += numberflow->letter_space;
        }
        x_ofs += layout_digit(numberflow, &numberflow->digits[i], x_ofs);
        x_ofs += numberflow->letter_space;
        if (digit_has_thousand_sep(numberflow, i)) {
            /*Thousand separator's animation is bound to the integer digit before it*/
            x_ofs += get_glyph_width(numberflow, numberflow->ksep);
            x_ofs += numberflow->letter_space;
        }
    }
}

void numberflow_layout_size(numberflow_t *numberflow)
{
    assert(numberflow != NULL);
    numberflow->width.curr = ANIM_LERP(numberflow->anim.anim_state, numberflow->width);
    numberflow->sym_width.curr = ANIM_LERP(numberflow->anim.anim_state, numberflow->sym_width);
    numberflow->sym_opa_old.curr = ANIM_LERP(numberflow->anim.anim_state, numberflow->sym_opa_old);
    numberflow->sym_opa_curr.curr = ANIM_LERP(numberflow->anim.anim_state, numberflow->sym_opa_curr);
}

static void digit_start(numberflow_t *numberflow, _nf_digit_t *digit)
{
    digit->anim.pending = false;
    _nf_slide_start_dsc_t* slide_start_dsc = &digit->slide_start_dsc;

    if (slide_start_dsc->available == false) {
        /*A subsequent update to the current digit restores it to the original
        * animation end state. However, once the animation start callback is invoked,
        * it means the original animation has already been overridden, so the digit
        * animation variables must be recalculated to values corresponding to the
        * new anim_state*/
        digit->flow.begin = digit->flow.curr;
        digit->opa.begin = digit->opa.curr;
        digit->width.begin = digit->width.curr;
        digit->anim.anim_state = NUMBERFLOW_ANIM_STATE_START;
        digit->anim.updated = true;

        return;
    }
    slide_start_dsc->available = false;

    int32_t line_height = numberflow->number_height;
    int32_t total_height = line_height * digit->modulus;

    /*Remove redundant flow coordinates as we are using accumulate mode*/
    int32_t end_px = digit->flow.end;
    int32_t start_px = digit->flow.curr;
    int32_t last_frame_px = digit->last_flow;
    int32_t start_px_overflow = (start_px / total_height) * total_height;
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
    int32_t remaining = digit->flow.end - digit->flow.begin;
    int32_t remaining_overflow = 0;
    if (remaining > total_height)
        remaining_overflow = (int)((remaining - total_height) / total_height) * total_height;
    else if (remaining < -total_height)
        remaining_overflow = (int)((remaining + total_height) / total_height) * total_height;

    digit->flow.end -= remaining_overflow;

    digit->anim.anim_state = NUMBERFLOW_ANIM_STATE_START;
    digit->anim.updated = true;

    /*Update digit width*/
    int16_t width = get_glyph_width(numberflow, digit->last_num + '0');
    ANIM_UPDATE(digit->width, width);

    /*Update digit opacity*/
    ANIM_UPDATE(digit->opa, slide_start_dsc->target_opa);

}

static uint16_t get_glyph_width(numberflow_t *numberflow, char glyph)
{
    return numberflow->get_glyph_width(numberflow->userdata, glyph);
}

static void digit_init(numberflow_t *numberflow, int8_t idx)
{
    _nf_digit_t *digit = &numberflow->digits[idx];
    memset(digit, 0, sizeof(_nf_digit_t));
    digit->modulus = numberflow->modulus[digit_index_to_mod_index(numberflow, idx)];
    digit->anim.updated = true;
    uint16_t width0 = get_glyph_width(numberflow, '0');
    ANIM_UPDATE_STOP(digit->width, width0);
}

static bool digit_retired(const _nf_digit_t *digit)
{
    return digit->opa.end == 0 && digit->anim.duration == 0;
}

static bool digit_resize(numberflow_t *numberflow, int8_t int_cnt, int8_t dec_cnt,
                          int8_t *front)
{
    int8_t old_high = numberflow->ones_place_idx;
    int8_t old_low = old_high - numberflow->digit_count + 1;
    int8_t high = NF_MAX(old_high, int_cnt - 1);
    int8_t low = old_low < -dec_cnt ? old_low : -dec_cnt;
    if (numberflow->digit_count == 0) {
        high = int_cnt - 1;
        low = -dec_cnt;
    }

    /* Reuse completed hidden places before extending the exponent range. */
    while (high - low + 1 > NUMBERFLOW_MAX_DIGITS && high >= int_cnt) {
        int8_t idx = old_high - high;
        if (digit_retired(&numberflow->digits[idx]) == false) break;
        --high;
    }
    while (high - low + 1 > NUMBERFLOW_MAX_DIGITS && low < -dec_cnt) {
        int8_t idx = old_high - low;
        if (digit_retired(&numberflow->digits[idx]) == false) break;
        ++low;
    }

    bool animate = high - low + 1 <= NUMBERFLOW_MAX_DIGITS;
    if (animate == false) {
        /* Dropping a place also drops its embedded pending/running animation. */
        numberflow->digit_count = 0;
        high = int_cnt - 1;
        low = -dec_cnt;
    }
    int8_t count = high - low + 1;
    int8_t shift = high - old_high;
    if (shift > 0) {
        /*Move old digits to the back*/
        for (int8_t i = numberflow->digit_count - 1; i >= 0; --i) {
            if (i + shift < count)
                numberflow->digits[i + shift] = numberflow->digits[i];
        }
    }
    else if (shift < 0) {
        for (int8_t i = -shift; i < numberflow->digit_count; ++i) {
            if (i + shift < count)
                numberflow->digits[i + shift] = numberflow->digits[i];
        }
    }
    numberflow->ones_place_idx = high;
    /*Clear allocated digits*/
    for (int8_t i = 0; i < count; ++i) {
        int8_t pos = high - i;
        if (numberflow->digit_count == 0 || pos > old_high || pos < old_low)
            digit_init(numberflow, i);
    }
    numberflow->digit_count = count;
    *front = shift;
    return animate;
}

static int8_t digit_roll(numberflow_t *numberflow, _nf_digit_t* digit, int8_t target_num, int8_t direction,
                         uint8_t target_opa, numberflow_anim_path_t path, uint32_t time)
{
    int32_t height = numberflow->number_height;
    int32_t round_height = height * digit->modulus;
    int8_t last_num = digit->last_num;

    /*Calculate remaining flow distance*/
    int32_t ofs_y = (target_num - last_num) * height;
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
        digit->anim.duration = 0;
        digit->anim.pending = false;
        digit->slide_start_dsc.available = false;
        digit->anim.anim_state = NUMBERFLOW_ANIM_STATE_INV;
        digit->anim.updated = true;

        /*Remove redundant loop coordinates generated by the accumulate method
        * otherwise lv_coord_t will overflow*/
        int32_t end_px = digit->flow.end + ofs_y;
        int32_t start_px = digit->flow.curr;
        int32_t start_px_overflow = (start_px / round_height) * round_height;
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
            digit->slide_start_dsc.available = false;
            return 0;
        }
        digit->slide_start_dsc.ofs_y = ofs_y;
        digit->slide_start_dsc.target_num = target_num;
        digit->slide_start_dsc.target_opa = target_opa;
        digit->slide_start_dsc.available = true;

        /*We need to update digit position just before current animation start
        * to achieve smooth transition*/
        anim_schedule(&digit->anim, numberflow, path, time);
    }
    return target_num - last_num;
}

static void numberflow_update_with_anim(numberflow_t *numberflow, int8_t *nums, int8_t int_cnt, int8_t dec_cnt, int8_t trend, int8_t symbol, bool en)
{

    uint32_t anim_time;
    if(en == false) {
        anim_time = 0;
    }
    else {
        anim_time = numberflow->anim_time;
    }

    /*Calculate new digits to be allocated before and after current digits*/
    int8_t front;
    bool ret = digit_resize(numberflow, int_cnt, dec_cnt, &front);
    if (ret == false)
        anim_time = 0;
    int8_t int_space = numberflow->ones_place_idx + 1;

    int8_t visible_digit_count = int_cnt + dec_cnt;
    int8_t first_visible_idx = int_space - int_cnt;
    int8_t last_visible_idx = first_visible_idx + visible_digit_count - 1;

    uint16_t width0 = get_glyph_width(numberflow, '0');
    uint16_t width_ksep = get_glyph_width(numberflow, numberflow->ksep) + numberflow->letter_space;
    uint16_t width_dsep = get_glyph_width(numberflow, numberflow->dsep) + numberflow->letter_space;
    int8_t ksep_cnt = (int_cnt - 1) / 3;
    int8_t dsep_cnt = dec_cnt > 0 ? 1 : 0;
    int32_t width = numberflow->letter_space * (visible_digit_count + dsep_cnt - 1);
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
    int32_t x_offset_end = -inv_zeros * (width0 + numberflow->letter_space);
    if (numberflow->ksep != '\0') {
        x_offset_end -= inv_kseps * width_ksep;
    }
    /*Start x_offset should minus newly added zeros' width plus new thousand separators' width.
    * Note that x_offset should always be negative as it's used to move invisible digits
    * on the left out of widget, but keep them in place to make animation continuous*/
    int32_t x_offset_start_delta = -front * (width0 + numberflow->letter_space);
    int8_t new_kseps = inv_kseps - ((int_space - 1 - front) / 3 - ksep_cnt);
    if (numberflow->ksep != '\0') {
        x_offset_start_delta -= new_kseps * width_ksep;
    }

    /*Calculate number offsets, accumulating results*/
    int8_t rolling_offset = 0;

    int8_t rolling_dir;
    if (numberflow->dir == NUMBERFLOW_DIR_DEFAULT || numberflow->dir == NUMBERFLOW_DIR_CONTINUOUS) {
        rolling_dir = trend;
    }
    else if (numberflow->dir == NUMBERFLOW_DIR_INCREASE) {
        rolling_dir = 1;
    }
    else if (numberflow->dir == NUMBERFLOW_DIR_DECREASE) {
        rolling_dir = -1;
    }
    else {
        rolling_dir = 0;
    }

    for (int8_t i = 0; i < numberflow->digit_count; i++) {
        uint8_t opacity;
        int8_t num;
        // TODO: implement digit alignment
        if (i < first_visible_idx || i > last_visible_idx) {
            opacity = 0;
            num = 0;
        }
        else {
            opacity = 255;
            num = nums[i - first_visible_idx];
        }

        if (numberflow->dir == NUMBERFLOW_DIR_CONTINUOUS) {
            if (rolling_dir == 0) {
                if (num > numberflow->digits[i].last_num) {
                    rolling_dir = 1;
                }
                else if (num < numberflow->digits[i].last_num) {
                    rolling_dir = -1;
                }
            }
        }

        int8_t delta_num = digit_roll(
            numberflow,
            &numberflow->digits[i],
            num + rolling_offset * numberflow->digits[i].modulus,
            rolling_dir,
            opacity,
            numberflow->anim_path_flow,
            anim_time
        );

        if (numberflow->dir == NUMBERFLOW_DIR_CONTINUOUS) {
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

    /*Symbol handling*/
    uint16_t symbol_width = 0;
    numberflow->sym_old = numberflow->sym_curr;
    numberflow->sym_opa_old = numberflow->sym_opa_curr;
    if (symbol == -1) {
        numberflow->sym_curr = '-';
    }
    else if (symbol == 1 && numberflow->flags & NUMBERFLOW_FLAG_SHOW_POSITIVE) {
        numberflow->sym_curr = '+';
    }
    else {
        numberflow->sym_curr = '\0';
    }
    symbol_width = get_glyph_width(numberflow, numberflow->sym_curr);
    if (symbol_width != 0) symbol_width += numberflow->letter_space;

    /*Animations*/
    if(anim_time == 0) {
        numberflow->int_cnt = int_cnt;
        numberflow->dec_cnt = dec_cnt;
        ANIM_UPDATE_STOP(numberflow->width, width);
        ANIM_UPDATE_STOP(numberflow->x_ofs, x_offset_end);
        ANIM_UPDATE_STOP(numberflow->sym_opa_old, 0);
        ANIM_UPDATE_STOP(numberflow->sym_opa_curr, 255);
        ANIM_UPDATE_STOP(numberflow->sym_width, symbol_width);
        numberflow->anim.duration = 0;
        numberflow->anim.pending = false;
        numberflow->anim.anim_state = NUMBERFLOW_ANIM_STATE_INV;

    }
    else {
        /*For non-monospace fonts, only update animation when width increase or digit
        * count change, or x_offset change, to prevent animation stuck at one place when fast updating.
        * For monospace fonts, width will be the same unless digit count change*/
        if (numberflow->flags & NUMBERFLOW_FLAG_TIGHT
             || width > numberflow->width.end
             || int_cnt != numberflow->int_cnt
             || dec_cnt != numberflow->dec_cnt
             || numberflow->sym_old != numberflow->sym_curr) {
            numberflow->int_cnt = int_cnt;
            numberflow->dec_cnt = dec_cnt;
            ANIM_UPDATE(numberflow->width, width);
            /*When a new digit is inserted on the left side, update the starting
            * value of the X offset so that the position of the original digit
            * remains unchanged*/
            numberflow->x_ofs.begin = numberflow->x_ofs.curr + x_offset_start_delta;
            numberflow->x_ofs.end = x_offset_end;
            ANIM_UPDATE(numberflow->sym_opa_old, 0);
            if (numberflow->sym_old != numberflow->sym_curr) {
                numberflow->sym_opa_curr.begin = 0;
                numberflow->sym_opa_curr.end = 255;
            }
            else {
                ANIM_UPDATE(numberflow->sym_opa_curr, 255);
            }
            ANIM_UPDATE(numberflow->sym_width, symbol_width);
            anim_schedule(&numberflow->anim, numberflow,
                          numberflow->anim_path_flow, anim_time);
            numberflow->anim.anim_state = NUMBERFLOW_ANIM_STATE_START;
        }
    }
}

static void anim_schedule(_nf_anim_state_t *anim, numberflow_t *numberflow,
                           numberflow_anim_path_t path, uint32_t duration)
{
    anim->started_at = numberflow->now_ms;
    anim->duration = duration;
    anim->path = path;
    anim->pending = true;
}

static bool anim_update(_nf_anim_state_t *anim, uint32_t now_ms)
{
    if (anim->duration == 0) return false;
    uint32_t elapsed = now_ms - anim->started_at;
    int32_t value;
    if (elapsed >= anim->duration) {
        value = NUMBERFLOW_ANIM_STATE_INV;
        anim->duration = 0;
    }
    else {
        value = anim->path(elapsed, anim->duration);
    }
    bool changed = anim->pending || value != anim->anim_state;
    anim->pending = false;
    if (changed) {
        anim->anim_state = value;
        anim->updated = true;
    }
    return changed;
}

bool numberflow_update(numberflow_t *numberflow, uint32_t now_ms)
{
    assert(numberflow != NULL);
    numberflow->now_ms = now_ms;
    bool changed = anim_update(&numberflow->anim, now_ms);
    for (int8_t i = 0; i < numberflow->digit_count; ++i) {
        _nf_digit_t *digit = &numberflow->digits[i];
        bool started = digit->anim.pending;
        if (started)
            digit_start(numberflow, digit);
        bool ret = anim_update(&digit->anim, now_ms);
        changed = changed || ret || started;
    }
    return changed;
}

bool numberflow_is_animating(const numberflow_t *numberflow)
{
    assert(numberflow != NULL);

    if (numberflow->anim.duration != 0) return true;
    for (int8_t i = 0; i < numberflow->digit_count; ++i) {
        if (numberflow->digits[i].anim.duration != 0) return true;
    }
    return false;
}
