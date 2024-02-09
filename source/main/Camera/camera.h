#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_camera.h"
#include "../common.h"

struct CameraConf
{
    QueueHandle_t to_qr_queue;
    // QueueHandle_t to_cam_queue;
    QueueHandle_t to_screen_queue;
    camera_config_t *camera_config;
};

void camera_start(struct CameraConf *conf);

enum meta_frame_state
{
    empty,
    written,
    readded
};

struct meta_frame
{
    uint8_t buf[IMG_WIDTH * IMG_HEIGHT * 2];
    enum meta_frame_state state;
};

void meta_frame_write(struct meta_frame *frame, uint8_t *buf);
void meta_frame_free(struct meta_frame *frame);