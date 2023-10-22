#include <esp_system.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "esp_camera.h"
#include "camera_pins.h"
#include <string.h>

#include <quirc.h>

#define CONFIG_XCLK_FREQ 20000000

#define print false

static esp_err_t init_camera(void)
{
    camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_GRAYSCALE,
        // .frame_size = FRAMESIZE_VGA,
        .frame_size = FRAMESIZE_96X96,

        .fb_count = 1,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY}; // CAMERA_GRAB_LATEST. Sets when buffers should be filled
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}

void app_main()
{
    esp_err_t err;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    err = init_camera();
    if (err != ESP_OK)
    {
        printf("err: %s\n", esp_err_to_name(err));
        return;
    }

    struct quirc *qr;

    qr = quirc_new();
    if (!qr)
    {
        perror("Failed to allocate memory");
        abort();
    }

    if (quirc_resize(qr, 96, 96) < 0)
    {
        perror("Failed to allocate video memory");
        abort();
    }

    while (1)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        // fb->width, fb->height, fb->format, fb->buf, fb->len

        uint8_t *image;

        int w, h;

        image = quirc_begin(qr, &w, &h);

        // printf("qr buffer is %dx%d\n", w, h);
        // printf("image is %dx%d (%d pixels)\n", fb->width, fb->height, fb->len);
        // printf("\n");

        /* Fill out the image buffer here.
         * image is a pointer to a w*h bytes.
         * One byte per pixel, w pixels per line, h lines in the buffer.
         */

        memcpy(image, fb->buf, fb->len);

        quirc_end(qr);

        /* We've previously fed an image to the decoder via
         * quirc_begin/quirc_end.
         */

        if (true)
        {
            int num_codes = quirc_count(qr);
            printf("found %d qrs\n", num_codes);

            for (int i = 0; i < num_codes; i++)
            {
                printf("procesing qr n:%d", i);

                struct quirc_code code;
                struct quirc_data data;
                quirc_decode_error_t err;

                quirc_extract(qr, i, &code);

            
                /* Decoding stage */
                // err = quirc_decode(&code, &data);
                // if (err)
                //     printf("DECODE FAILED: %s\n", quirc_strerror(err));
                // else
                //     printf("Data: %s\n", data.payload);
            }
        }

        if (false)
        {

            for (int y = 0; y < fb->height; y++)
            {
                for (int x = 0; x < fb->width; x++)
                {
                    int pixel = fb->buf[y * fb->height + x];
                    char *colors[] = {"  ",
                                      "· ",
                                      "··",
                                      "·:",
                                      "::",
                                      ":o",
                                      "oo",
                                      "Oo",
                                      "OO",
                                      "O#",
                                      "##",
                                      "@#",
                                      "@@"};
                    int color_count = sizeof(colors) / sizeof(*colors);
                    for (int i = 0; i < color_count; i++)
                    {
                        if (pixel < 250 / color_count * i || i == color_count - 1)
                        {
                            printf("%s", colors[i]);
                            i = color_count + 1;
                        }
                    }
                }
                printf("\n");
            }
            printf("\n\n\n\n\n\n");
        }
        esp_camera_fb_return(fb);
    }

    quirc_destroy(qr);
}