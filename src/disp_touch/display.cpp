#include "disp_touch.hpp"

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 280;

LGFX gfx;
// 缓冲区
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[2][screenWidth * screenHeight / 10];

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    if (gfx.getStartCount() == 0)
    { // Processing if not yet started
        gfx.startWrite();
    }
    gfx.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);
    lv_disp_flush_ready(disp);
}

void init_display_touch()
{
    gfx.begin();
    gfx.setRotation(3);
    touch.begin();
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf[0], buf[1], screenWidth * screenHeight / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    // disp_drv.hor_res = screenWidth;
    // disp_drv.ver_res = screenHeight;

    disp_drv.hor_res = screenHeight;
    disp_drv.ver_res = screenWidth;

    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // 修改屏幕亮度
    gfx.setBrightness(100);
    // lv_demo_benchmark();
    // lv_demo_music();
    lv_demo_widgets();
    // lv_demo_stress();
}