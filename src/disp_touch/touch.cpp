#include "disp_touch.hpp"

CST816T touch(TP_SDA, TP_SCL, TP_RST, TP_INT);
/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    bool touched;
    uint8_t gesture;
    uint16_t touchX, touchY;

    touched = touch.getTouch(&touchX, &touchY, &gesture);

    // y:1-271
    // x:0-230
    // 打印触摸坐标
    Serial.println("touchX: " + String(touchX) + ", touchY: " + String(touchY) + ", gesture: " + String(gesture) + ", touched: " + String(touched));
    touchY = 28 * touchY / 27 + 1;
    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates*/
        int rotation = gfx.getRotation() % 4;
        int width, height;
        if (rotation != 0)
        {
            width = gfx.width();
            height = gfx.height();
        }
        // 根据屏幕方向，调整触摸坐标
        switch (rotation)
        {
        case 0:
            data->point.x = touchX;
            data->point.y = touchY;
            break;
        case 1:
            data->point.x = touchY;
            data->point.y = width - touchX;
            break;
        case 2:
            data->point.x = height - touchX;
            data->point.y = width - touchY;
            break;
        case 3:
            data->point.x = height - touchY;
            data->point.y = touchX;
            break;
        }
    }
}
