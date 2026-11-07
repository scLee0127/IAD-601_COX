#include "main.h"
#include "global.h"
#include "mygpio.h"
#include "mybutton.h"

/* External-pull-up, active-low buttons. */
#define MYBTN_PROC_REFRESH_TIME_MS       10U
#define MYBTN_DEBOUNCE_TIME_MS           20U
#define MYBTN_LONGKEY_TIME_MS          2200U
#define MYBTN_PUSH_STATE                  0U
#define MYBTN_PULL_STATE                  1U

typedef struct
{
    uint8_t raw_state;
    uint8_t stable_state;
    uint8_t longkey_sent;
    uint32_t raw_changed_tick;
    uint32_t pressed_tick;
} MYBTN_STATE_T;

static MYBTN_STATE_T gstBtnUp = { MYBTN_PULL_STATE, MYBTN_PULL_STATE, 0U, 0U, 0U };
static MYBTN_STATE_T gstBtnDown = { MYBTN_PULL_STATE, MYBTN_PULL_STATE, 0U, 0U, 0U };
static MYBTN_STATE_T gstBtnSel = { MYBTN_PULL_STATE, MYBTN_PULL_STATE, 0U, 0U, 0U };

static uint8_t _mybtn_update(const uint8_t sample,
                             MYBTN_STATE_T *state,
                             const uint32_t now,
                             const uint8_t click_event,
                             const uint8_t long_event)
{
    uint8_t event = MYBTN_STATE_IDLE;

    if (sample != state->raw_state) {
        state->raw_state = sample;
        state->raw_changed_tick = now;
    }

    if ((state->raw_state != state->stable_state) &&
        ((now - state->raw_changed_tick) >= MYBTN_DEBOUNCE_TIME_MS)) {
        state->stable_state = state->raw_state;

        if (MYBTN_PUSH_STATE == state->stable_state) {
            state->pressed_tick = now;
            state->longkey_sent = 0U;
        }
        else {
            /* A long-press release must not become a second short click. */
            if ((0U == state->longkey_sent) && (MYBTN_STATE_IDLE != click_event)) {
                event = click_event;
            }
            state->longkey_sent = 0U;
        }
    }

    if ((MYBTN_PUSH_STATE == state->stable_state) &&
        (0U == state->longkey_sent) &&
        ((now - state->pressed_tick) >= MYBTN_LONGKEY_TIME_MS)) {
        state->longkey_sent = 1U;
        event = long_event;
    }

    return event;
}

uint8_t MYBTN_Proc(void)
{
    uint8_t event = MYBTN_STATE_IDLE;
    uint8_t button_event;
    const uint32_t now = HAL_GetTick();

    if (0 != gnBtnTimer) {
        return MYBTN_STATE_IDLE;
    }
    gnBtnTimer = MYBTN_PROC_REFRESH_TIME_MS;

    button_event = _mybtn_update(
        (uint8_t)HAL_GPIO_ReadPin(MYGPIO_BTN_GPIO_PORT, MYGPIO_BTN_UP_GPIO_NUM),
        &gstBtnUp, now, MYBTN_STATE_CLICK_UP, MYBTN_STATE_LONGKEY_UP);
    if (MYBTN_STATE_IDLE != button_event) {
        event = button_event;
    }

    button_event = _mybtn_update(
        (uint8_t)HAL_GPIO_ReadPin(MYGPIO_BTN_GPIO_PORT, MYGPIO_BTN_DOWN_GPIO_NUM),
        &gstBtnDown, now, MYBTN_STATE_CLICK_DOWN, MYBTN_STATE_LONGKEY_DOWN);
    if (MYBTN_STATE_IDLE != button_event) {
        event = button_event;
    }

    button_event = _mybtn_update(
        (uint8_t)HAL_GPIO_ReadPin(MYGPIO_BTN_GPIO_PORT, MYGPIO_BTN_SEL_GPIO_NUM),
        &gstBtnSel, now, MYBTN_STATE_CLICK_SEL, MYBTN_STATE_LONGKEY_SEL);
    if (MYBTN_STATE_IDLE != button_event) {
        event = button_event;
        if (MYBTN_STATE_CLICK_SEL == event) {
            /* Preserve the existing manual over-current clear behavior. */
            gcnMotorOverCurrent_f = 0U;
        }
    }

    if (MYBTN_STATE_IDLE != event) {
        MYUTIL_Printf("Button event %u\r\n", event);
        MYGPIO_Run_Buzzer(BUZZER_PLAY_TYPE_BEEP);
    }

    return event;
}
