#ifndef THINGER_MBED_OPTA_OTA_H
#define THINGER_MBED_OPTA_OTA_H

#ifndef THINGER_OTA_COMPRESSION
#define THINGER_OTA_COMPRESSION 1
#endif

#ifndef THINGER_OTA_CHUNK_SIZE
#define THINGER_OTA_CHUNK_SIZE 8192
#endif

#include "ThingerOTA.h"
#include <mbed.h>
#include <FATFileSystem.h>
#include <QSPIFBlockDevice.h>
#include <MBRBlockDevice.h>
#include <stm32h7xx_hal_rtc_ex.h>

#if THINGER_OTA_COMPRESSION
#include "util/lzss.h"
#endif

#define SD_MOUNT_PATH                       "fs"
#define FULL_UPDATE_FILE_PATH               "/" SD_MOUNT_PATH "/" "UPDATE.BIN"

#define APOTA_QSPI_FLASH_FLAG         (1 << 2)
#define APOTA_SDCARD_FLAG             (1 << 3)
#define APOTA_RAW_FLAG                (1 << 4)
#define APOTA_FATFS_FLAG              (1 << 5)
#define APOTA_LITTLEFS_FLAG           (1 << 6)
#define APOTA_MBR_FLAG                (1 << 7)

extern RTC_HandleTypeDef RTCHandle;

class ThingerPortentaOTA : public ThingerOTA{

public:
    ThingerPortentaOTA(ThingerClient& client) :
        ThingerOTA(client)
    {
        // initialize a default block size
        set_block_size(THINGER_OTA_CHUNK_SIZE);
    }

    virtual ~ThingerPortentaOTA(){

    }

protected:

    void fill_options(pson& options) override{
        // set atmelsam platform
        options["platform"] = "atmelsam";
    }

#if THINGER_OTA_COMPRESSION
    const char* supported_compression() override{
        return "lzss";
    }
#endif

    bool is_ota_supported(){
        #define BOOTLOADER_ADDR (0x8000000)
        uint32_t bootloader_data_offset = 0x1F000;
        uint8_t* bootloader_data = (uint8_t*)(BOOTLOADER_ADDR + bootloader_data_offset);
        uint8_t currentBootloaderVersion = bootloader_data[1];
        return currentBootloaderVersion >= 22;
    }

    bool begin_ota(const char* firmware, const char* version, size_t size, pson& options, pson& state) override
    {
        if(!is_ota_supported()){
            state["error"] = "OTA is not supported by the current bootloader";
            return false;
        }

        _bd_raw_qspi = mbed::BlockDevice::get_default_instance();

        if (_bd_raw_qspi !=nullptr && _bd_raw_qspi->init() != QSPIF_BD_ERROR_OK) {
            state["error"] = "QSPI init failure";
            return false;
        }

        bool mounted = false;

        if(storage_type_ == QSPI_FLASH_FATFS){
            _fs_qspi = new mbed::FATFileSystem(SD_MOUNT_PATH);
            mounted = _fs_qspi->mount(_bd_raw_qspi) == 0;
        }else if(storage_type_ == QSPI_FLASH_FATFS_MBR){
            _bd_qspi = new mbed::MBRBlockDevice(_bd_raw_qspi, 2);
            _fs_qspi = new mbed::FATFileSystem(SD_MOUNT_PATH);
            mounted = _fs_qspi->mount(_bd_qspi) == 0;
        }else{
            state["error"] = "Unknown target destination";
            return false;
        }

        if(!mounted){
            state["error"] = "Error while mounting the filesystem";
            return false;
        }

        // open file for writing
        f = fopen(FULL_UPDATE_FILE_PATH, "wb");

        // check if the file cannot be created
        if(f==nullptr){
            state["error"] = "Cannot create file: " FULL_UPDATE_FILE_PATH ;
        }

#if THINGER_OTA_COMPRESSION
        // if the file is compressed, initialize the decoder
        if(is_compressed()){
            decoder_ = new lzss_decoder([&](uint8_t c) {
                fputc(c, f);
#if THINGER_OTA_MD5_VERIFICATION
                update_firmware_checksum(&c, 1);
#endif
            });
        }
#endif

        if(f==nullptr){
            return false;
        }

        return f!=nullptr;
    }

    bool reset_ota() override{
        // close current file (if any)
        if(f!=nullptr){
            fclose(f);
            f = nullptr;
        }

        // ensure no pending OTA files
        if(_fs_qspi && _fs_qspi->remove(FULL_UPDATE_FILE_PATH)==0){
            THINGER_DEBUG_VALUE("OTA", "Removed: ", FULL_UPDATE_FILE_PATH);
        }

        return true;
    }

    bool write_ota(uint8_t* buffer, size_t bytes, pson& state) override{
#if THINGER_OTA_COMPRESSION
        if(is_compressed()){
            decoder_->decode(buffer, bytes);
            return true;
        }else{
            return fwrite(buffer, 1, bytes, f) == bytes;
        }
#else
        return fwrite(buffer, 1, bytes, f) == bytes;
#endif
    }

    bool end_ota(pson& state) override{

        // close the file
        int err = fclose(f);
        if(err<0){
            state["error"] = "error while closing file";
            return false;
        }

#if THINGER_OTA_COMPRESSION
        // if the file is compressed, delete the decoder
        if(is_compressed()){
            delete decoder_;
            decoder_ = nullptr;
#if THINGER_OTA_MD5_VERIFICATION
            if(!verify_checksum(md5_ctx_, md5_checksum_)){
                state["error"] = "MD5 checksum verification failed!";
                return false;
            }
#endif
        }
#endif

        // write the firmware size to the RTC backup registers
        HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR0, 0x07AA);
        HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR1, storage_type_);
        HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR2, 2);
        HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR3, firmware_size_);

        return true;
    }

protected:

    enum StorageTypePortenta {
        QSPI_FLASH_FATFS        = APOTA_QSPI_FLASH_FLAG | APOTA_FATFS_FLAG,
        QSPI_FLASH_FATFS_MBR    = APOTA_QSPI_FLASH_FLAG | APOTA_FATFS_FLAG | APOTA_MBR_FLAG,
        SD_FATFS                = APOTA_SDCARD_FLAG | APOTA_FATFS_FLAG,
        SD_FATFS_MBR            = APOTA_SDCARD_FLAG | APOTA_FATFS_FLAG | APOTA_MBR_FLAG,
    };

private:
    uint32_t _data_offset = 2;
    mbed::BlockDevice   * _bd_raw_qspi = nullptr;
    mbed::BlockDevice   * _bd_qspi     = nullptr;
    mbed::FATFileSystem * _fs_qspi     = nullptr;

    StorageTypePortenta storage_type_ = QSPI_FLASH_FATFS_MBR;
    FILE *f = nullptr;

#if THINGER_OTA_COMPRESSION
    lzss_decoder* decoder_ = nullptr;
#endif

};

#endif