#include "lcd.h"
#include "LPC17xx.h"
#include "systick.h"
#include "uart.h"
#include <stdint.h>
#include <string.h>

uint16_t fix_color(uint16_t color) {
    uint16_t fixed = 0;
    for (uint32_t i = 0; i < 8; i++) {
        uint16_t lsb = color & (1 << i);
        uint16_t msb = color & (1 << (15 - i));
        fixed |= (lsb << (15 - 2 * i)) | (msb >> (15 - 2 * i));
    }
    return fixed;
}

uint16_t read_pixel_color(uint16_t x, uint16_t y) {
    // setting coordinates
    lcdWriteReg(ADRX_RAM, x);
    lcdWriteReg(ADRY_RAM, y);

    // selecting data register
    lcdWriteIndex(DATA_RAM);

    // lcdReadData(); // dummy read 9. GRAM Address Map & Read/Write on page 79
    // of ILI9325 Version: 0.43

    return fix_color(lcdReadData());
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    lcdWriteReg(ADRX_RAM, x);
    lcdWriteReg(ADRY_RAM, y);
    lcdWriteReg(DATA_RAM, color);
}

void reset_window() {
    lcdWriteReg(HADRPOS_RAM_START, 0);
    lcdWriteReg(HADRPOS_RAM_END, LCD_MAX_X);
    lcdWriteReg(VADRPOS_RAM_START, 0);
    lcdWriteReg(VADRPOS_RAM_END, LCD_MAX_Y);
}

void fill_screen_brute(uint16_t color) {
    for (uint16_t x = 0; x < LCD_MAX_X; x++) {
        for (uint16_t y = 0; y < LCD_MAX_Y; y++) {
            draw_pixel(x, y, color);
        }
    }
}

void fill_screen_fast(uint16_t color) {
    lcdWriteReg(ADRX_RAM, 0);
    lcdWriteReg(ADRY_RAM, 0);
    lcdWriteIndex(DATA_RAM);
    for (uint32_t i = 0; i < LCD_MAX_X * LCD_MAX_Y; i++) {
        lcdWriteData(color);
    }
}

//
void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
               uint16_t color) {
    // zmienne pomocnicze
    int d, dx, dy, ai, bi, xi, yi;
    int x = x1, y = y1;
    // ustalenie kierunku rysowania
    if (x1 < x2) {
        xi = 1;
        dx = x2 - x1;
    } else {
        xi = -1;
        dx = x1 - x2;
    }
    // ustalenie kierunku rysowania
    if (y1 < y2) {
        yi = 1;
        dy = y2 - y1;
    } else {
        yi = -1;
        dy = y1 - y2;
    }
    // pierwszy piksel
    draw_pixel(x, y, color);
    // os wiodaca OX
    if (dx > dy) {
        ai = (dy - dx) * 2;
        bi = dy * 2;
        d = bi - dx;
        // petla po kolejnych x
        while (x != x2) {
            // test wsp�lczynnika
            if (d >= 0) {
                x += xi;
                y += yi;
                d += ai;
            } else {
                d += bi;
                x += xi;
            }
            draw_pixel(x, y, color);
        }
    }
    // os wiodaca OY
    else {
        ai = (dx - dy) * 2;
        bi = dx * 2;
        d = bi - dy;
        // petla po kolejnych y
        while (y != y2) {
            // test wsp�lczynnika
            if (d >= 0) {
                x += xi;
                y += yi;
                d += ai;
            } else {
                d += bi;
                y += yi;
            }
            draw_pixel(x, y, color);
        }
    }
}

void draw_X_sign(uint16_t x, uint16_t y, uint16_t color) {
    draw_line(x - 10, y - 10, x + 10, y + 10, color);
    draw_line(x - 10, y + 10, x + 10, y - 10, color);
}

// all values are inclusive
void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    draw_line(x, y, x + w + 1, y, color);                 // horizontal left
    draw_line(x + w + 1, y, x + w + 1, y + h + 1, color); // vertical top
    draw_line(x + w + 1, y + h + 1, x, y + h + 1, color); // horizontal right
    draw_line(x, y + h + 1, x, y, color);                 // vertical bottom
}

// all values are inclusive
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    lcdWriteReg(HADRPOS_RAM_START, x);
    lcdWriteReg(HADRPOS_RAM_END, x + w);
    lcdWriteReg(VADRPOS_RAM_START, y);
    lcdWriteReg(VADRPOS_RAM_END, y + h);

    lcdWriteReg(ADRX_RAM, x);
    lcdWriteReg(ADRY_RAM, y);
    lcdWriteIndex(DATA_RAM);
    for (uint32_t i = 0; i < (w + 1) * (h + 1); i++) {
        lcdWriteData(color);
    }
    reset_window();
}

void draw_poly(Point *points, uint32_t n, uint16_t color) {
    for (uint32_t i = 0; i < n - 1; i++) {
        draw_line(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y,
                  color);
    }
}

Point TP_get_mean_XY() {
    uint32_t samples = 50;
    uint32_t x = 0;
    uint32_t y = 0;
    for (uint32_t i = 0; i < samples; i++) {
        int32_t tx = 0, ty = 0;
        touchpanelGetXY(&tx, &ty);
        x += tx;
        y += ty;
    }
    Point tp = {x / samples, y / samples};
    return tp;
}

Calibration_Matrix cal_matrix;
#define CALIBRATION_PRECISION 1000

void TP_config_restore(void) {
    memcpy(&cal_matrix, (void*)&LPC_RTC->GPREG0, sizeof(cal_matrix));
}

void TP_config_store(void) {
    memcpy((void*)&LPC_RTC->GPREG0, &cal_matrix, sizeof(cal_matrix));
}

void _calculate_calibration_matrix(Point lcd[], Point tp[]) {
    for (uint32_t i = 0; i < 3; i++) {
        lcd[i].x /= 10;
        lcd[i].y /= 10;
        tp[i].x /= 10;
        tp[i].y /= 10;
    }

    int32_t det = tp[0].x * (tp[1].y - tp[2].y) +
                  tp[1].x * (tp[2].y - tp[0].y) + tp[2].x * (tp[0].y - tp[1].y);

    int32_t detA = lcd[0].x * (tp[1].y - tp[2].y) +
                   lcd[1].x * (tp[2].y - tp[0].y) +
                   lcd[2].x * (tp[0].y - tp[1].y);

    int32_t detB = lcd[0].x * (tp[2].x - tp[1].x) +
                   lcd[1].x * (tp[0].x - tp[2].x) +
                   lcd[2].x * (tp[1].x - tp[0].x);

    int32_t detC = lcd[0].x * (tp[1].x * tp[2].y - tp[2].x * tp[1].y) +
                   lcd[1].x * (tp[2].x * tp[0].y - tp[0].x * tp[2].y) +
                   lcd[2].x * (tp[0].x * tp[1].y - tp[1].x * tp[0].y);

    int32_t detD = lcd[0].y * (tp[1].y - tp[2].y) +
                   lcd[1].y * (tp[2].y - tp[0].y) +
                   lcd[2].y * (tp[0].y - tp[1].y);

    int32_t detE = lcd[0].y * (tp[2].x - tp[1].x) +
                   lcd[1].y * (tp[0].x - tp[2].x) +
                   lcd[2].y * (tp[1].x - tp[0].x);

    int32_t detF = lcd[0].y * (tp[1].x * tp[2].y - tp[2].x * tp[1].y) +
                   lcd[1].y * (tp[2].x * tp[0].y - tp[0].x * tp[2].y) +
                   lcd[2].y * (tp[0].x * tp[1].y - tp[1].x * tp[0].y);

    cal_matrix.A = detA * CALIBRATION_PRECISION / det;
    cal_matrix.B = detB * CALIBRATION_PRECISION / det;
    cal_matrix.C = detC * CALIBRATION_PRECISION * 10 / det;
    cal_matrix.D = detD * CALIBRATION_PRECISION / det;
    cal_matrix.E = detE * CALIBRATION_PRECISION / det;
    cal_matrix.F = detF * CALIBRATION_PRECISION * 10 / det;
}

void TP_config(void) {
    Point cal_tp_points[3];
    Point cal_lcd_points[] = {
        {40, 40}, {LCD_MAX_X - 40, 40}, {LCD_MAX_X - 40, LCD_MAX_Y - 40}};

    for (uint32_t i = 0; i < 3; i++) {
        fill_screen_fast(LCDBlack);
        draw_X_sign(cal_lcd_points[i].x, cal_lcd_points[i].y, LCDMagenta);
        // wait for tp irq
        while (LPC_GPIO0->FIOPIN & (1 << 19))
            ;
        cal_tp_points[i] = TP_get_mean_XY();

        fill_screen_fast(LCDBlueSea);
        SYSTICK_wait(1500);

        char buf[64];
        sprintf(buf, "tx: %d\tty: %d ", cal_tp_points[i].x, cal_tp_points[i].y);
        UART_write_string(buf);
        sprintf(buf, "lx: %d\tly: %d\r\n", cal_lcd_points[i].x,
                cal_lcd_points[i].y);
        UART_write_string(buf);
    }

    _calculate_calibration_matrix(cal_lcd_points, cal_tp_points);
}

Point TP_to_LCD(const Point tp) {
    Point lcd = {tp.x * cal_matrix.A / CALIBRATION_PRECISION +
                     tp.y * cal_matrix.B / CALIBRATION_PRECISION +
                     cal_matrix.C / CALIBRATION_PRECISION,
                 tp.x * cal_matrix.D / CALIBRATION_PRECISION +
                     tp.y * cal_matrix.E / CALIBRATION_PRECISION +
                     cal_matrix.F / CALIBRATION_PRECISION};

    return lcd;
}
