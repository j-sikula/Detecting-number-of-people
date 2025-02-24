#include "sd_card.h"

static const char *TAG = "SD_CARD";


#define PIN_NUM_LCD_CS    23
#define PIN_NUM_TOUCH_CS  4



typedef struct {
    /**
     * If FAT partition can not be mounted, and this parameter is true,
     * create partition table and format the filesystem.
     */
    bool format_if_mount_failed;
    int max_files;                  ///< Max number of open files
    /**
     * If format_if_mount_failed is set, and mount fails, format the card
     * with given allocation unit size. Must be a power of 2, between sector
     * size and 128 * sector size.
     * For SD cards, sector size is always 512 bytes. For wear_levelling,
     * sector size is determined by CONFIG_WL_SECTOR_SIZE option.
     *
     * Using larger allocation unit size will result in higher read/write
     * performance and higher overhead when storing small files.
     *
     * Setting this field to 0 will result in allocation unit set to the
     * sector size.
     */
    size_t allocation_unit_size;
    /**
     * Enables real ff_disk_status function implementation for SD cards
     * (ff_sdmmc_status). Possibly slows down IO performance.
     *
     * Try to enable if you need to handle situations when SD cards
     * are not unmounted properly before physical removal
     * or you are experiencing issues with SD cards.
     *
     * Doesn't do anything for other memory storage media.
     */
    bool disk_status_check_enable;
} mount_config_t;

typedef struct vfs_fat_sd_ctx_t {
    BYTE pdrv;                                  //Drive number that is mounted
    mount_config_t mount_config;    //Mount configuration
    FATFS *fs;                                  //FAT structure pointer that is registered
    sdmmc_card_t *card;                         //Card info
    char *base_path;                            //Path where partition is registered
} vfs_fat_sd_ctx_t;

static inline size_t s_sdcard_vfs_fat_get_allocation_unit_size(
        size_t sector_size, size_t requested_size)
{
    size_t alloc_unit_size = requested_size;
    const size_t max_sectors_per_cylinder = 128;
    const size_t max_size = sector_size * max_sectors_per_cylinder;
    alloc_unit_size = MAX(alloc_unit_size, sector_size);
    alloc_unit_size = MIN(alloc_unit_size, max_size);
    return alloc_unit_size;
}

static vfs_fat_sd_ctx_t *s_ctx[FF_VOLUMES] = {};

static esp_err_t s_sdcard_mount_prepare_mem(const char *base_path,
        BYTE *out_pdrv,
        char **out_dup_path,
        sdmmc_card_t** out_card)
{
    esp_err_t ret = ESP_OK;
    char* dup_path = NULL;
    sdmmc_card_t* card = NULL;

    // connect SDMMC driver to FATFS
    BYTE pdrv = FF_DRV_NOT_USED;
    if (ff_diskio_get_drive(&pdrv) != ESP_OK || pdrv == FF_DRV_NOT_USED) {
        ESP_LOGD(TAG, "the maximum count of volumes is already mounted");
        return ESP_ERR_NO_MEM;
    }

    // not using ff_memalloc here, as allocation in internal RAM is preferred
    card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
    ESP_GOTO_ON_FALSE(card, ESP_ERR_NO_MEM, cleanup, TAG, "could not locate new sdmmc_card_t");

    dup_path = strdup(base_path);
    if(!dup_path){
        ESP_LOGD(TAG, "could not copy base_path");
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    *out_card = card;
    *out_pdrv = pdrv;
    *out_dup_path = dup_path;
    return ESP_OK;
cleanup:
    free(card);
    free(dup_path);
    return ret;
}

static void s_sdcard_call_host_deinit(const sdmmc_host_t *host_config)
{
    if (host_config->flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
        host_config->deinit_p(host_config->slot);
    } else {
        host_config->deinit();
    }
}

static uint32_t s_sdcard_get_unused_context_id(void)
{
    for (uint32_t i = 0; i < FF_VOLUMES; i++) {
        if (!s_ctx[i]) {
            return i;
        }
    }
    return FF_VOLUMES;
}

static esp_err_t s_sdcard_partition_card(const mount_config_t *mount_config,
                                const char *drv, sdmmc_card_t *card, BYTE pdrv)
{
    FRESULT res = FR_OK;
    esp_err_t err;
    const size_t workbuf_size = 4096;
    void* workbuf = NULL;
    ESP_LOGW(TAG, "partitioning card");

    workbuf = ff_memalloc(workbuf_size);
    if (workbuf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    LBA_t plist[] = {100, 0, 0, 0};
    res = f_fdisk(pdrv, plist, workbuf);
    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGD(TAG, "f_fdisk failed (%d)", res);
        goto fail;
    }
    size_t alloc_unit_size = s_sdcard_vfs_fat_get_allocation_unit_size(
                card->csd.sector_size,
                mount_config->allocation_unit_size);
    ESP_LOGW(TAG, "formatting card, allocation unit size=%d", alloc_unit_size);
    const MKFS_PARM opt = {(BYTE)FM_ANY, 0, 0, 0, alloc_unit_size};
    res = f_mkfs(drv, &opt, workbuf, workbuf_size);
    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGD(TAG, "f_mkfs failed (%d)", res);
        goto fail;
    }

    free(workbuf);
    return ESP_OK;
fail:
    free(workbuf);
    return err;
}

static esp_err_t s_sdcard_f_mount(sdmmc_card_t *card, FATFS *fs, const char *drv, uint8_t pdrv, const mount_config_t *mount_config)
{

    esp_err_t err = ESP_OK;
    FRESULT res = f_mount(fs, drv, 1);
    if (res != FR_OK) {
        err = ESP_FAIL;
        ESP_LOGW(TAG, "failed to mount card (%d)", res);

        bool need_mount_again = (res == FR_NO_FILESYSTEM || res == FR_INT_ERR) && mount_config->format_if_mount_failed;
        if (!need_mount_again) {
            return ESP_FAIL;
        }

        err = s_sdcard_partition_card(mount_config, drv, card, pdrv);
        if (err != ESP_OK) {
            return err;
        }

        ESP_LOGW(TAG, "mounting again");
        res = f_mount(fs, drv, 0);
        if (res != FR_OK) {
            err = ESP_FAIL;
            ESP_LOGD(TAG, "f_mount failed after formatting (%d)", res);
            return err;
        }
    }

    return ESP_OK;
}

static esp_err_t s_sdcard_mount_to_vfs_fat(const mount_config_t *mount_config, sdmmc_card_t *card, uint8_t pdrv,
                                  const char *base_path, FATFS **out_fs)
{
    FATFS *fs = NULL;
    esp_err_t err;
    ff_diskio_register_sdmmc(pdrv, card);
    ff_sdmmc_set_disk_status_check(pdrv, mount_config->disk_status_check_enable);
    ESP_LOGD(TAG, "using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    // connect FATFS to VFS
    err = esp_vfs_fat_register(base_path, drv, mount_config->max_files, &fs);
    *out_fs = fs;
    if (err == ESP_ERR_INVALID_STATE) {
        // it's okay, already registered with VFS
    } else if (err != ESP_OK) {
        ESP_LOGD(TAG, "esp_vfs_fat_register failed 0x(%x)", err);
        goto fail;
    }

    // Try to mount partition
    err = s_sdcard_f_mount(card, fs, drv, pdrv, mount_config);
    if (err != ESP_OK) {
        goto fail;
    }
    return ESP_OK;

fail:
    if (fs) {
        f_mount(NULL, drv, 0);
    }
    esp_vfs_fat_unregister_path(base_path);
    ff_diskio_unregister(pdrv);
    return err;
}

static esp_err_t s_sdcard_esp_vfs_fat_sdspi_mount(const char* base_path,
                                  const sdmmc_host_t* host_config_input,
                                  const sdspi_device_config_t* slot_config,
                                  const mount_config_t* mount_config,
                                  sdmmc_card_t** out_card)
{
    esp_err_t ret;
    const sdmmc_host_t* host_config = host_config_input;
    vfs_fat_sd_ctx_t *ctx = NULL;
    uint32_t ctx_id = FF_VOLUMES;
    FATFS *fs = NULL;
    int card_handle = -1;   //uninitialized
    sdmmc_card_t* card = NULL;
    BYTE pdrv = FF_DRV_NOT_USED;
    char* dup_path = NULL;
    bool host_initted = false;

    ESP_RETURN_ON_ERROR(s_sdcard_mount_prepare_mem(base_path, &pdrv, &dup_path, &card), TAG, "failed to prepare memory");

    ESP_RETURN_ON_ERROR(sdspi_host_init(), TAG, "failed to initialise sdspi host");
    ESP_RETURN_ON_ERROR(sdspi_host_init_device(slot_config, &card_handle), TAG, "failed to initialise SD card device");
    host_initted = true;


    /*
     * The `slot` argument inside host_config should be replaced by the SD SPI handled returned
     * above. But the input pointer is const, so create a new variable.
     */
    sdmmc_host_t new_config;
    if (card_handle != host_config->slot) {
        new_config = *host_config_input;
        host_config = &new_config;
        new_config.slot = card_handle;
    }

    // This seems to be required. I really don't know why adding this works but it seems to make
    // all the difference as to if the cards initialise correctly or fail. With this delay my dirt cheap card works
    // but without this it fails out. I have only tested this with one 4G card as I connected the pins wrongly on my
    // other 4G card and it got really hot and fried itself and I don't have any other cheap cards of other sizes to test.
    // I guess I should test lowering the delay to the fail point to find out just how much delay is required.
    // I think I found this solution in response to a "my card is not working" style post from someone when I did a google
    // search because my card kept failing to init, but not always, and this pointed to a change or line in Arduino code.
    ESP_LOGI(TAG, "Waiting brief time for card to stabilize after reset");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // probe and initialize card
    ESP_GOTO_ON_ERROR(sdmmc_card_init(host_config, card), cleanup, TAG, "sdmmc_card_init failed");

    ESP_GOTO_ON_ERROR(s_sdcard_mount_to_vfs_fat(mount_config, card, pdrv, dup_path, &fs), cleanup, TAG, "mount_to_vfs failed");

    if (out_card != NULL) {
        *out_card = card;
    }

    ctx = calloc(sizeof(vfs_fat_sd_ctx_t), 1);
    ESP_GOTO_ON_FALSE(ctx, ESP_ERR_NO_MEM, cleanup, TAG, "failed to allocate ctx memory");

    ctx->pdrv = pdrv;
    memcpy(&ctx->mount_config, mount_config, sizeof(mount_config_t));
    ctx->card = card;
    ctx->base_path = dup_path;
    ctx->fs = fs;
    ctx_id = s_sdcard_get_unused_context_id();
    assert(ctx_id != FF_VOLUMES);
    s_ctx[ctx_id] = ctx;

    return ESP_OK;

cleanup:
    if (host_initted) {
        s_sdcard_call_host_deinit(host_config);
    }
    free(card);
    free(dup_path);
    return ret;
};

static esp_err_t s_sdcard_unmount_core(const char *base_path, sdmmc_card_t *card)
{
    BYTE pdrv = ff_diskio_get_pdrv_card(card);
    if (pdrv == 0xff) {
        return ESP_ERR_INVALID_ARG;
    }

    // unmount
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    f_mount(0, drv, 0);
    // release SD driver
    ff_diskio_unregister(pdrv);

    s_sdcard_call_host_deinit(&card->host);
    free(card);

    esp_err_t err = esp_vfs_fat_unregister_path(base_path);
    return err;
}

static bool s_sdcard_get_context_id_by_card(const sdmmc_card_t *card, uint32_t *out_id)
{
    vfs_fat_sd_ctx_t *p_ctx = NULL;
    for (int i = 0; i < FF_VOLUMES; i++) {
        p_ctx = s_ctx[i];
        if (p_ctx) {
            if (p_ctx->card == card) {
                *out_id = i;
                return true;
            }
        }
    }
    return false;
}

esp_err_t s_sdcard_example_esp_vfs_fat_sdspi_unmount(const char *base_path, sdmmc_card_t *card)
{
    uint32_t id = FF_VOLUMES;
    bool found = s_sdcard_get_context_id_by_card(card, &id);
    if (!found) {
        return ESP_ERR_INVALID_ARG;
    }
    free(s_ctx[id]->base_path);
    s_ctx[id]->base_path = NULL;
    free(s_ctx[id]);
    s_ctx[id] = NULL;

    esp_err_t err = s_sdcard_unmount_core(base_path, card);

    return err;
}


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
    mount_config_t mount_config = {
        .format_if_mount_failed = SDCARD_FORMAT_ON_MOUNT_FAIL,
        .max_files =              SDCARD_MAX_OPEN_FILES,
        .allocation_unit_size =   SDCARD_ALLOCATION_UNIT_SIZE,
        .disk_status_check_enable = SDCARD_CHECK_DISK_STATUS,
    };
    sdmmc_card_t *card;
    const char mount_point[] = SDCARD_MOUNT_POINT;

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAG, "Initialising SPI peripheral");

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
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    #define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
    spi_device_interface_config_t dev_cfg = {
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_TXBIT_LSBFIRST | SPI_DEVICE_3WIRE,
        .clock_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .mode = 0,
        .queue_size = 10,
    };


    /*
     * I think the following text is no longer completely true. I think its a
     * combination of these adds and the delay in s_sdcard_esp_vfs_fat_sdspi_mount
     * that fixes the actual problem. These "dummy adds" are actually because I
     * needed to test adding the touch screen for conflicts for when I merge this
     * code with my display code at some random point in the future but I have
     * these devices connected so the CS's must be correctly initialised.
     *
     * Removing both these adds and the delay causes the failed to init to be triggered.
     * Removing only the delay causes the init fail to happen very rarely.
     * Removing only the adds causes the failed to init to be triggered.
     *
     * When I made the below comments I didn't quite have the limited understanding
     * that I do now. I've gone from totally clueless to moderately clueless.
     *
     * Adding "fake" devices here to the SPI bus means that there no longer needs
     * to be a delay before adding the SD card to the SPI bus to allow the devices
     * to reach a consistent "we're not listening" state and/or incorrect levels
     * on the pins. Even when no other devices were attached it was sometimes touch
     * and go whether the SD card would work or fail after a reboot/cold start.
     *
     * It should be noted that the physical SPI devices are attached and the cs pin
     * numbers used in the dummy device adds are real connections. It is just that
     * the devices are not being handled in this testing/experimental code.
     *
     * It should also be noted that until the SD card is registered and mounted that
     * no transactions should happen on any other SPI device because the SD card
     * may decide not to go into SPI mode and then may randomly respond to any
     * transactions because it will be in SD mode and so will ignore CS polling.
     *
     */
    ESP_LOGI(TAG, "Adding fake LCD SPI device");
    dev_cfg.spics_io_num = PIN_NUM_LCD_CS;
    spi_device_handle_t lcd_handle;
    lcd_handle = (spi_device_handle_t)malloc(sizeof(lcd_handle));
    ESP_ERROR_CHECK(spi_bus_add_device(host.slot, &dev_cfg, &lcd_handle));

    ESP_LOGI(TAG, "Adding fake LCD TOUCH device");
    dev_cfg.spics_io_num = PIN_NUM_TOUCH_CS;
    spi_device_handle_t touch_handle;
    touch_handle = (spi_device_handle_t)malloc(sizeof(touch_handle));
    ESP_ERROR_CHECK(spi_bus_add_device(host.slot, &dev_cfg, &touch_handle));

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ESP_ERROR_CHECK(s_sdcard_esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card));
    //ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card));

    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

 // Write data to the SD card
 const char *data = "Hello, SD card!";
 write_to_sd_card("hello.txt", data);

}


void write_to_sd_card(const char *filename, const char *data)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE("SD Card", "Failed to open file for writing");
        return;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI("SD Card", "File written: %s", filepath);
}