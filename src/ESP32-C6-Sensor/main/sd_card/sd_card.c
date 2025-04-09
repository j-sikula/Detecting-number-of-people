#include "sd_card.h"

static const char *TAG = "SD_CARD";

/**
 * @brief Initialize the SD card.
 * source: https://github.com/espressif/esp-idf/blob/master/examples/storage/sd_card/sdmmc/main/sd_card_example_main.c
 * @author Espressif under Apache license
 */
void init_sd_card()
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = 0,
        .max_files = SDCARD_MAX_OPEN_FILES,
        .allocation_unit_size = SDCARD_ALLOCATION_UNIT_SIZE,
        .disk_status_check_enable = SDCARD_CHECK_DISK_STATUS,
    };
    sdmmc_card_t *card;
    const char mount_point[] = SDCARD_MOUNT_POINT;

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card));

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

}

void write_to_sd_card(const char *filename, const char *data)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    char *extension = strchr(filename, '.');
    if (extension == NULL) // extension is not present
    {
        strcat(filepath, ".csv");
    }

    FILE *f = fopen(filepath, "a");
    if (f == NULL)
    {
        ESP_LOGE("SD Card", "Failed to open file for writing");
        return;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI("SD Card", "File written: %s", filepath);
}

void save_raw_data(const char *filename, measurement_t *measurement)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    char *extension = strchr(filename, '.');
    if (extension == NULL) // extension is not present
    {
        strcat(filepath, ".csv");
    }
    ESP_LOGD("SD Card", "Preparing saving raw data to %s", filepath);
    FILE *f = fopen(filepath, "a");
    if (f == NULL)
    {
        ESP_LOGE("SD Card", "Failed to open file for writing");
        for (int i = 0; i < MEASUREMENT_LOOP_COUNT; i++)
        {
            free(measurement[i].timestamp);
        }
        free(measurement);
        return;
    }
    for (int i = 0; i < MEASUREMENT_LOOP_COUNT; i++)
    {
        char *data = measurement_array_to_string(&measurement[i]);
        fprintf(f, "%s", data);
        free(data);
    }
    fclose(f);
    ESP_LOGI("SD Card", "File written: %s", filepath);
    free(measurement);
}

char *read_data(const char *filename, int size)
{
    if (size <= 0)
    {
        ESP_LOGE("SD Card", "Invalid size");
        return NULL;
    }
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    FILE *f = fopen(filepath, "r");
    if (f == NULL)
    {
        ESP_LOGE("SD Card", "Failed to open file for writing");
        return NULL;
    }

    char *buffer = (char *)malloc(size * sizeof(char));
    if (buffer == NULL)
    {
        ESP_LOGE("SD Card", "Failed to allocate memory");
        return NULL;
    }

    int ch;
    for (int i = 0; i < size; i++)
    {
        ch = fgetc(f);
        buffer[i] = ch;
        if (ch == EOF)
        {
            break;
        }
    }

    fclose(f);
    return buffer;
}

/**
 * @brief Get the size of a file.
 * source: GPT-4o request "how to get size of file?"
 */

long get_file_size(const char *filename)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", SDCARD_MOUNT_POINT, filename);

    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filepath);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return size;
}

char *read_file(const char *filename)
{
    return read_data(filename, get_file_size(filename));
}