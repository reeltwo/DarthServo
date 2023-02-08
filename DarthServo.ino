#define USE_SMQDEBUG
/*
 * --------------------------------------------------------------------
 * Darth Servo (https://github.com/reeltwo/DarthServo)
 * --------------------------------------------------------------------
 * Written by Mimir Reynisson (skelmir)
 *
 * DarthServo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * AmidalaFirmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with AmidalaFirmware; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

///////////////////////////////////

#if __has_include("build_version.h")
#include "build_version.h"
#else
#define BUILD_VERSION "custom"
#endif

///////////////////////////////////
// CONFIGURABLE OPTIONS
///////////////////////////////////

#define USE_DEBUG
#define USE_WIFI
//#define USE_SERVO_DEBUG

//#define USE_DROID_REMOTE              // Define for droid remote support

#ifdef USE_WIFI
#define USE_MDNS
#define USE_OTA
#define USE_WIFI_WEB
#define USE_WIFI_MARCDUINO
#endif

////////////////

#define PREFERENCE_WIFI_ENABLED         "wifi"
#define PREFERENCE_WIFI_SSID            "ssid"
#define PREFERENCE_WIFI_PASS            "pass"
#define PREFERENCE_WIFI_AP              "ap"

#define PREFERENCE_REMOTE_ENABLED       "remote"
#define PREFERENCE_REMOTE_HOSTNAME      "rhost"
#define PREFERENCE_REMOTE_SECRET        "rsecret"
#define PREFERENCE_REMOTE_PAIRED        "rpaired"
#define PREFERENCE_REMOTE_LMK           "rlmk"

#define PREFERENCE_MARCSERIAL1          "mserial1"
#define PREFERENCE_MARCSERIAL2          "mserial2"
#define PREFERENCE_MARCSERIAL_PASS      "mserialpass"
#define PREFERENCE_MARCSERIAL_ENABLED   "mserial"

#define PREFERENCE_MARCWIFI_ENABLED     "mwifi"
#define PREFERENCE_MARCWIFI_SERIAL_PASS "mwifipass"

#define MAX_NAME_LENGTH 64

////////////////

#ifdef USE_DROID_REMOTE
#include "ReelTwoSMQ32.h"
#else
#include "ReelTwo.h"
#endif

////////////////

#include "dome/Logics.h"
#include "dome/LogicEngineController.h"
#include "dome/HoloLights.h"
#include "dome/NeoPSI.h"
#include "dome/FireStrip.h"
#include "dome/BadMotivator.h"
#include "dome/TeecesPSI.h"
#include "dome/TeecesLogics.h"
#include "body/DataPanel.h"
#include "body/ChargeBayIndicator.h"

////////////////

#define PIN_FRONT_LOGIC 15
#define PIN_REAR_LOGIC 33
#define PIN_FRONT_PSI 32
#define PIN_REAR_PSI 23
#define PIN_FRONT_HOLO 25
#define PIN_REAR_HOLO 26
#define PIN_TOP_HOLO 27
#define PIN_AUX1 2
#define PIN_AUX2 4
#define PIN_AUX3 5
#define PIN_AUX4 18
#define PIN_AUX5 19

#ifdef USE_RSERIES_RLD_CURVED
// Define RSeries RLD clock pin to be AUX5 (could just as well be AUX1, AUX2, AUX3, or AUX4)
#define PIN_REAR_LOGIC_CLOCK  PIN_AUX5
#endif

#define CBI_DATAIN_PIN      PIN_AUX3
#define CBI_CLOCK_PIN       PIN_AUX2
#define CBI_LOAD_PIN        PIN_AUX1

////////////////

#include "ServoDispatchPCA9685.h"
#include "ServoSequencer.h"
#include "ServoEasing.h"
#include "core/Animation.h"
#include "core/Marcduino.h"
#include <Preferences.h>
#ifdef USE_OTA
#include <ArduinoOTA.h>
#endif
#include "SPIFFS.h"
#include "LittleFS.h"
#include "FS.h"

#define SYSTEM_FS SPIFFS
#define STORAGE_FS LittleFS

#define SEQUENCE_MAGIC_SIGNATURE uint32_t(0x4D494E41) // ANIM
#define SCRIPT_MAGIC_SIGNATURE   uint32_t(0x54584554) // TEXT
#define SETTINGS_MAGIC_SIGNATURE uint32_t(0x4F565253) // SRVO
#define VIRTUOSO_MAGIC_SIGNATURE uint32_t(0x55545256) // VRTU
#include "inc/VirtuosoInterpreter.h"

////////////////

#define PCA9685_START_ADDRESS 0x40
#define PCA9685_END_ADDRESS   0x6F
#define PCA9685_ALL_CALL      0x70

///////////////////////////////////

#include "pin-map.h"

///////////////////////////////////

#include "miniz/miniz.h"
#include "inc/Base64.h"
#include "inc/JSONOrderedEncoder.h"
#include "inc/StreamEncoderDecoder.h"

StreamBase64Decoder streamBase64Decoder;

///////////////////////////////////

#if defined(USE_RSERIES_RLD_CURVED)
LogicEngineCurvedRLD<PIN_REAR_LOGIC, PIN_REAR_LOGIC_CLOCK> RLD(LogicEngineRLDDefault, 3);
#elif defined(USE_RSERIES_RLD)
LogicEngineDeathStarRLD<PIN_REAR_LOGIC> RLD(LogicEngineRLDDefault, 3);
#else
AstroPixelRLD<PIN_REAR_LOGIC> RLD(LogicEngineRLDDefault, 3);
#endif

#ifdef USE_RSERIES_FLD
LogicEngineDeathStarFLD<PIN_FRONT_LOGIC> FLD(LogicEngineFLDDefault, 1);
#else
AstroPixelFLD<PIN_FRONT_LOGIC> FLD(LogicEngineFLDDefault, 1);
#endif

AstroPixelFrontPSI<PIN_FRONT_PSI> frontPSI(LogicEngineFrontPSIDefault, 4);
AstroPixelRearPSI<PIN_REAR_PSI> rearPSI(LogicEngineRearPSIDefault, 5);

#if USE_HOLO_TEMPLATE
HoloLights<PIN_FRONT_HOLO, NEO_GRB> frontHolo(1);
HoloLights<PIN_REAR_HOLO, NEO_GRB> rearHolo(2);
HoloLights<PIN_TOP_HOLO, NEO_GRB> topHolo(3);
#else
HoloLights frontHolo(PIN_FRONT_HOLO, HoloLights::kRGB, 1);
HoloLights rearHolo(PIN_REAR_HOLO, HoloLights::kRGB, 2);
HoloLights topHolo(PIN_TOP_HOLO, HoloLights::kRGB, 3);
#endif

///////////////////////////////////

#ifdef USE_LVGL_DISPLAY
#include "core/PushButton.h"
#include "TFT_eSPI.h"
#include "lvgl.h"

#define SCREEN_WIDTH            320     // OLED display width, in pixels
#define SCREEN_HEIGHT           170     // OLED display height, in pixels
#define SCREEN_BUFFER_SIZE      (SCREEN_WIDTH * SCREEN_HEIGHT)
#define SCREEN_SLEEP_TIMER      30*1000 // Turn off display if idle for 30 seconds

#define LONGPRESS_DELAY         2000
#define SPLASH_SCREEN_DURATION  2000
#define STATUS_SCREEN_DURATION  1000

#define LV_DELAY(x) {                                               \
  uint32_t start = millis();                                        \
  do {                                                              \
    lv_timer_handler();                                             \
    delay(1);                                                       \
  } while (millis() < start + (x));                                 \
}

TFT_eSPI tft = TFT_eSPI();
static lv_disp_drv_t disp_drv;      // contains callback functions
static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
static lv_color_t *lv_disp_buf;
PushButton button1(PIN_BUTTON_1, true);
PushButton button2(PIN_BUTTON_2, true);

////////////////////////////////

LV_FONT_DECLARE(lv_font_Astromech);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_FONT_DECLARE(lv_font_montserrat_48);

////////////////////////////////

static const lv_font_t* font_large = &lv_font_Astromech;
static const lv_font_t* font_medium = &lv_font_montserrat_48;
// static const lv_font_t* font_normal = &lv_font_montserrat_32;

#endif

////////////////

#ifdef USE_DROID_REMOTE
#define REMOTE_ENABLED       true
#define SMQ_HOSTNAME         "DarthServo"
#define SMQ_SECRET           "Astromech"
#endif
#ifdef USE_WIFI
#define WIFI_ENABLED         true
// Set these to your desired credentials.
#define WIFI_AP_NAME         "DarthServo"
#define WIFI_AP_PASSPHRASE   "Astromech"
#define WIFI_ACCESS_POINT    true  /* true if access point: false if joining existing wifi */

#include "wifi/WifiAccess.h"
#endif

#define MARC_SERIAL1_BAUD_RATE          9600
#define MARC_SERIAL2_BAUD_RATE          9600
#define MARC_SERIAL_PASS                false
#define MARC_SERIAL_ENABLED             true
#define MARC_WIFI_ENABLED               true
#define MARC_WIFI_SERIAL_PASS           true

#ifdef USE_MDNS
#include <ESPmDNS.h>
#endif
#ifdef USE_WIFI_WEB
#include "wifi/WifiWebServer.h"
#endif
#ifdef USE_WIFI_MARCDUINO
#include "wifi/WifiMarcduinoReceiver.h"
#endif

#define GROUP1 0x0001

const ServoSettings servoSettings[] PROGMEM = {
#ifdef AMIDALA_ORDER
    { 8,  600, 2400, GROUP1 },
    { 7,  600, 2400, GROUP1 },
    { 6,  600, 2400, GROUP1 },
    { 5,  600, 2400, GROUP1 },

    { 4,  600, 2400, GROUP1 },
    { 3,  600, 2400, GROUP1 },
    { 2,  600, 2400, GROUP1 },
    { 1,  600, 2400, GROUP1 },

    { 9,  600, 2400, GROUP1 },
    { 10, 600, 2400, GROUP1 },
    { 11, 600, 2400, GROUP1 },
    { 12, 600, 2400, GROUP1 },

    { 13, 600, 2400, GROUP1 },
    { 14, 600, 2400, GROUP1 },
    { 15, 600, 2400, GROUP1 },
    { 16, 600, 2400, GROUP1 },

    { 24, 600, 2400, GROUP1 },
    { 23, 600, 2400, GROUP1 },
    { 22, 600, 2400, GROUP1 },
    { 21, 600, 2400, GROUP1 },

    { 20, 600, 2400, GROUP1 },
    { 19, 600, 2400, GROUP1 },
    { 18, 600, 2400, GROUP1 },
    { 17, 600, 2400, GROUP1 },

    { 25, 600, 2400, GROUP1 },
    { 26, 600, 2400, GROUP1 },
    { 27, 600, 2400, GROUP1 },
    { 28, 600, 2400, GROUP1 },

    { 29, 600, 2400, GROUP1 },
    { 30, 600, 2400, GROUP1 },
    { 31, 600, 2400, GROUP1 },
    { 32, 600, 2400, GROUP1 },
#else
    { 1,  600, 2400, GROUP1 },
    { 2,  600, 2400, GROUP1 },
    { 3,  600, 2400, GROUP1 },
    { 4,  600, 2400, GROUP1 },

    { 5,  600, 2400, GROUP1 },
    { 6,  600, 2400, GROUP1 },
    { 7,  600, 2400, GROUP1 },
    { 8,  600, 2400, GROUP1 },

    { 9,  600, 2400, GROUP1 },
    { 10, 600, 2400, GROUP1 },
    { 11, 600, 2400, GROUP1 },
    { 12, 600, 2400, GROUP1 },

    { 13, 600, 2400, GROUP1 },
    { 14, 600, 2400, GROUP1 },
    { 15, 600, 2400, GROUP1 },
    { 16, 600, 2400, GROUP1 },

    { 17, 600, 2400, GROUP1 },
    { 18, 600, 2400, GROUP1 },
    { 19, 600, 2400, GROUP1 },
    { 20, 600, 2400, GROUP1 },

    { 21, 600, 2400, GROUP1 },
    { 22, 600, 2400, GROUP1 },
    { 23, 600, 2400, GROUP1 },
    { 24, 600, 2400, GROUP1 },

    { 25, 600, 2400, GROUP1 },
    { 26, 600, 2400, GROUP1 },
    { 27, 600, 2400, GROUP1 },
    { 28, 600, 2400, GROUP1 },

    { 29, 600, 2400, GROUP1 },
    { 30, 600, 2400, GROUP1 },
    { 31, 600, 2400, GROUP1 },
    { 32, 600, 2400, GROUP1 },
#endif
};
ServoDispatchPCA9685<SizeOfArray(servoSettings)> servoDispatch(servoSettings);
ServoSequencer servoSequencer(servoDispatch);
AnimationPlayer player(servoSequencer);
MarcduinoSerial<> marcduinoSerial(player);
Virtuoso::Virtuosopreter interpreter(servoSequencer);
Preferences preferences;
uint8_t sPCA9685StartAddress;
uint8_t sPCA9685Count;
bool sPCA9685Nonsequential;
bool sPCA9685Connected[PCA9685_END_ADDRESS - PCA9685_START_ADDRESS];

#ifndef STORAGE_FS
const esp_partition_t* sSettingsPartition;
const esp_partition_t* sWorkingPartition;
const esp_partition_t* sStoragePartition;
const esp_partition_t* sVirtuosoScript;
#endif

const esp_partition_t* sVirtuosoBytes;

static const esp_partition_t* find_partition(esp_partition_type_t type, esp_partition_subtype_t subtype, const char* name)
{
    const esp_partition_t* part  = esp_partition_find_first(type, subtype, name);
    
    if (part != NULL) {
        printf("\tfound partition '%s' at offset 0x%x with size 0x%x\n", part->label, part->address, part->size);
    } else {
        printf("\tpartition not found!\n");
    }
    return part;
}

#ifdef USE_WIFI
WifiAccess wifiAccess;
bool wifiEnabled;
bool wifiActive;
#endif

#ifdef USE_DROID_REMOTE
bool remoteEnabled;
bool remoteActive;
#endif

#ifdef USE_WIFI_MARCDUINO
WifiMarcduinoReceiver wifiMarcduinoReceiver(wifiAccess);
#endif

#ifdef USE_WIFI
TaskHandle_t eventTask;
#endif

#ifdef USE_OTA
bool otaInProgress;
#endif

#ifdef ST7789V_DRIVER
#define TOUCH_MODULES_CST_MUTUAL
#define USE_TOUCH
#include "TouchLib.h"
TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS328_SLAVE_ADDRESS, PIN_TOUCH_RES);

static void lv_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touch.read())
    {
        TP_Point t = touch.getPoint(0);
        data->point.x = t.x;
        data->point.y = t.y;
        data->state = LV_INDEV_STATE_PR;
        printf("[%d,%d]\n", t.x, t.y);
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_WIFI
String getHostName()
{
    String mac = wifiAccess.getMacAddress();
    String hostName = mac.substring(mac.length()-5, mac.length());
    hostName.remove(2, 1);
    hostName = WIFI_AP_NAME+String("-")+hostName;
    return hostName;
}
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_SDCARD
static bool sSDCardMounted;
#endif

bool mountReadOnlyFileSystem()
{
#ifdef SYSTEM_FS
    return (SYSTEM_FS.begin(true));
#endif
    return false;
}

bool mountWritableFileSystem()
{
#ifdef STORAGE_FS
    if (!STORAGE_FS.begin(false, "/lfs2", 5, "storage"))
    {
        DEBUG_PRINTLN("Formatting storage filesystem");
        return STORAGE_FS.begin(true, "/lfs2", 5, "storage");
    }
    return true;
#endif
    return false;
}

bool getSDCardMounted()
{
#ifdef USE_SDCARD
    return sSDCardMounted;
#else
    return false;
#endif
}

bool mountSDFileSystem()
{
#ifdef USE_SDCARD
    if (!ensureVSPIStarted())
        return false;
    if (SD.begin(SD_CS_PIN))
    {
        DEBUG_PRINTLN("Card Mount Success");
        sSDCardMounted = true;
        return true;
    }
    DEBUG_PRINTLN("Card Mount Failed");
#endif
    return false;
}

void unmountSDFileSystem()
{
#ifdef USE_SDCARD
    if (sSDCardMounted)
    {
        sSDCardMounted = false;
        SD.end();
    }
    if (sVSPIStarted)
    {
        SPI.end();
    }
#endif
}

void unmountFileSystems()
{
    unmountSDFileSystem();
#ifdef STORAGE_FS
    STORAGE_FS.end();
#endif
#ifdef SYSTEM_FS
    SYSTEM_FS.end();
#endif
}

///////////////////////////////////////////////////////////////////////////////

void sendSerialCommand(const char* cmd)
{
    // TODO
    DEBUG_PRINT("SERIAL: "); DEBUG_PRINTLN(cmd);
}

///////////////////////////////////////////////////////////////////////////////

uint16_t sNumServoNames;
String* sServoNames;

String getServoName(uint16_t index)
{
    if (sServoNames != nullptr &&
        index < sNumServoNames &&
        sServoNames[index].length() != 0)
    {
        return sServoNames[index];
    }
    return "Servo"+String(index+1);
}

void setServoName(uint16_t index, String servoName)
{
    if (sServoNames != nullptr &&
        index < sNumServoNames)
    {
        if (servoName.length() > MAX_NAME_LENGTH)
            servoName = servoName.substring(0, MAX_NAME_LENGTH);
        sServoNames[index] = servoName;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Animations are stored compressed and binary encoded in the writable
// filesystem. The encoding is essentially sorted json order using binary
// tags. Tags can be found in JSONOrderedEncoder.h
//
class StoredAnimationPlayer
{
public:
    inline bool isPlaying() const
    {
        return fPlaying;
    }

#ifdef STORAGE_FS
    bool startPlaying(fs::FS &fs, const char* path, uint16_t startFrame = 0)
#else
    bool startPlaying(const esp_partition_t* partition, uint16_t startFrame = 0)
#endif
    {
        // Stop if already playing
        stop();
    #ifdef STORAGE_FS
        in.reset(fs, path);
        if (in.hasError()) {
            printf("Error1\n");
            return false;
        }
        fBufferSize = in.fileSize();
    #else
        in.reset(partition);
        uint32_t magicSignature = 0;
        if (in.read(magicSignature) != SEQUENCE_MAGIC_SIGNATURE)
        {
            printf("BAD SIG\n");
            return false;
        }
        fBufferSize = 0;
        in.read(fBufferSize);
    #endif
        uint8_t tag = 0;
        char buffer[64];
        if (in.read(tag) != JSONOrderedEncoder::kSEQUENCE_START)
        {
            printf("SEQSTART MISSING\n");
            return false;
        }
        if (in.read(tag) != JSONOrderedEncoder::kSEQID)
        {
            printf("SEQID MISSING\n");
            return false;
        }
        in.readString(buffer, sizeof(buffer));
        printf("SEQID: %s\n", buffer);
        if (in.read(tag) != JSONOrderedEncoder::kSEQNAM) {
            return false;
        }
        in.readString(buffer, sizeof(buffer));
        printf("SEQNAM: %s\n", buffer);
        if (in.read(tag) != JSONOrderedEncoder::kFRAMES_START)
        {
            printf("Missing start\n");
            return false;
        }
        fStartFrame = startFrame;
        fCurrentFrame = 0;
        fPlaying = true;
        fNextFrameTime = millis();
        *fSerialCmd = '\0';
        return true;
    }

    inline bool isPlaying()
    {
        return fPlaying;
    }

    void stop()
    {
        if (fPlaying)
        {
            fPlaying = false;
        #ifdef STORAGE_FS
            in.close();
        #endif
        }
    }

    bool play()
    {
        if (!fPlaying)
            return false;
        if (fNextFrameTime > millis())
            return true;
        uint16_t frameDur = 0;
        uint16_t servoNum = 0;
        uint16_t servoDur = 0;
        uint16_t servoPos = 0;
        uint16_t servoMaxDur = 0;
        uint8_t servoEas = 0;
        bool servoEna = false;
        char buffer[64];
        if (*fSerialCmd != '\0')
        {
            sendSerialCommand(fSerialCmd);
            *fSerialCmd = '\0';
        }
        // bool servosStarted = false;
        bool sequenceEnd = false;
        while (fPlaying && !in.hasError() && in.size() < fBufferSize)
        {
            uint8_t tag;
            in.read(tag);
            switch (tag)
            {
                case JSONOrderedEncoder::kFRAME_START:
                    printf("----------------- %u\n", fCurrentFrame);
                    break;
                case JSONOrderedEncoder::kFRAME_END:
                    if (frameDur == 0)
                        frameDur = servoMaxDur;
                    // printf("next frame: %d\n", frameDur * 10);
                    if (fCurrentFrame >= fStartFrame)
                        fNextFrameTime = millis() + frameDur * 10;
                    fCurrentFrame++;
                    // printf("END FRAME\n");
                    return true;
                case JSONOrderedEncoder::kFRBEG:
                    in.readString(buffer, sizeof(buffer));
                    if (fCurrentFrame >= fStartFrame)
                    {
                        // printf("FRBEG: %s\n", buffer);
                        sendSerialCommand(buffer);
                    }
                    break;
                case JSONOrderedEncoder::kFRDUR:
                    in.read(frameDur);
                    // printf("FRDUR: %d\n", frameDur);
                    break;
                case JSONOrderedEncoder::kFREND:
                    in.readString(fSerialCmd, sizeof(fSerialCmd));
                    if (fCurrentFrame >= fStartFrame)
                    {
                        // printf("FREND: %s\n", fSerialCmd);
                    }
                    else
                    {
                        /* skipping */
                        *fSerialCmd = '\0';
                    }
                    break;
                case JSONOrderedEncoder::kFRNAM:
                    in.readString(buffer, sizeof(buffer));
                    // printf("FRNAM: %s\n", buffer);
                    break;
                case JSONOrderedEncoder::kSERVOS_START:
                    // servosStarted = true;
                    break;
                case JSONOrderedEncoder::kSERVOS_END:
                    // servosStarted = false;
                    break;
                case JSONOrderedEncoder::kSRVNUM:
                    in.read(servoNum);
                    servoNum -= 1;
                    // printf("num: %d\n", servoNum);
                    break;
                case JSONOrderedEncoder::kSRVDUR:
                    in.read(servoDur);
                    servoMaxDur = max(servoDur, servoMaxDur);
                    // printf("dur: %d\n", servoDur);
                    break;
                case JSONOrderedEncoder::kSRVEAS:
                    in.read(servoEas);
                    // printf("eas: %d\n", servoEas);
                    break;
                case JSONOrderedEncoder::kSRVENA:
                    in.read(servoEna);
                    // printf("ena: %d\n", servoEna);
                    break;
                case JSONOrderedEncoder::kSRVPOS:
                    in.read(servoPos);
                    // printf("pos: %d\n", servoPos);
                    if (fCurrentFrame >= fStartFrame && servoEna)
                    {
                        servoDispatch.setServoEasingMethod(servoNum,
                            Easing::getEasingMethod(servoEas));
                        printf("[%u].moveTo(%f, %u) using %u\n",
                            servoNum,
                            servoPos / 1000.0,
                            uint32_t(servoDur) * 10,
                            servoEas);
                        servoDispatch.moveTo(servoNum, 0,
                            uint32_t(servoDur) * 10,
                            servoPos / 1000.0);
                    }
                    servoEna = false;
                    servoNum = -1;
                    servoDur = 0;
                    servoEas = 0;
                    servoPos = 0;
                    break;
                case JSONOrderedEncoder::kFRAMES_END:
                    // printf("FRAMES END\n");
                    break;
                case JSONOrderedEncoder::kSEQUENCE_END:
                    printf("DONE\n");
                    sequenceEnd = true;
                    break;
                default:
                    printf("BAD TAG: %d\n", tag);
                    fPlaying = false;
                    break;
            }
        }
        if (fPlaying &&
            (sequenceEnd || in.hasError() || in.size() >= fBufferSize))
        {
            stop();
        }
        // printf("GAME OVER\n");
        return fPlaying;
    }

protected:
#ifdef STORAGE_FS
    CompressedFileInputStream in;
#else
    PartitionStream in;
#endif
    uint32_t fBufferSize = 0;
    bool fPlaying = false;
    uint16_t fStartFrame = 0;
    uint16_t fCurrentFrame = 0;
    uint32_t fNextFrameTime = 0;
    char fSerialCmd[64];
};

StoredAnimationPlayer animationPlayer;

///////////////////////////////////////////////////////////////////////////////

class ServoAnimation
{
public:
#ifdef STORAGE_FS
    static bool loadSettings(fs::FS &fs, const char* path)
#else
    static bool loadSettings(const esp_partition_t* partition)
#endif
    {
    #ifdef STORAGE_FS
        FileStream in(fs, path);
        uint32_t bufferSize = in.fileSize();
    #else
        PartitionStream in(partition);
        uint32_t magicSignature = 0;
        if (in.read(magicSignature) != SETTINGS_MAGIC_SIGNATURE)
        {
            printf("BAD SIG\n");
            return false;
        }
        uint32_t bufferSize = 0;
        in.read(bufferSize);
    #endif
        uint8_t tag = 0;
        char buffer[64];
        if (in.read(tag) != JSONOrderedEncoder::kSETTINGS_START)
        {
            printf("SETTINGS_START MISSING\n");
            return false;
        }
        unsigned servoIndex = 0;
        while (in.size() < bufferSize)
        {
            in.read(tag);
            if (tag == JSONOrderedEncoder::kSETTINGS_END)
                break;
            if (tag != JSONOrderedEncoder::kSETTING_START)
            {
                printf("SETTING_START MISSING\n");
                return false;
            }
            while (tag != JSONOrderedEncoder::kSETTING_END && in.size() < bufferSize)
            {
                bool servoEna = false;
                bool servoUseErr = false;
                uint16_t servoErr = 0;
                uint16_t servoMax = 0;
                uint16_t servoMin = 0;
                in.read(tag);
                switch (tag)
                {
                    case JSONOrderedEncoder::kSRVENA:
                        in.read(servoEna);
                        servoDispatch.setPin(servoIndex, servoEna ?
                                servoSettings[servoIndex].pinNum : 0);
                        break;
                    case JSONOrderedEncoder::kSRVERR:
                        in.read(servoErr);
                        servoDispatch.setNeutral(servoIndex, servoErr);
                        break;
                    case JSONOrderedEncoder::kSRVMAX:
                        in.read(servoMax);
                        servoDispatch.setEnd(servoIndex, servoMax);
                        break;
                    case JSONOrderedEncoder::kSRVMIN:
                        in.read(servoMin);
                        servoDispatch.setStart(servoIndex, servoMin);
                        break;
                    case JSONOrderedEncoder::kSRVNAM:
                        in.readString(buffer, sizeof(buffer));
                        setServoName(servoIndex, buffer);
                        break;
                    case JSONOrderedEncoder::kSRVUSEERR:
                        // Ignore for now
                        in.read(servoUseErr);
                        (void) servoUseErr;
                        break;
                }
            }
            servoIndex++;
        }
        return true;
    }

    static void printSettingsJSON(Print& out)
    {
        out.print("[");
        int servoCount = min(int(servoDispatch.getNumServos()), sPCA9685Count*16);
        for (unsigned i = 0; i < servoCount; i++)
        {
            if (i > 0)
                out.print(",");
            out.print("{");
            out.print("\"srverr\":");
            out.print(servoDispatch.getNeutral(i));
            out.print(",\"srvena\":");
            out.print(servoDispatch.getPin(i) != 0 ? "true" : "false");
            out.print(",\"srvmax\":");
            out.print(servoDispatch.getEnd(i));
            out.print(",\"srvmin\":");
            out.print(servoDispatch.getStart(i));
            out.print(",\"srvnam\":\"");
            out.print(getServoName(i));
            out.print("\",\"srvuseerr\":");
            // TODO: Not supported yet
            out.print("false");
            out.print("}");
        }
        out.print("]");
    }

    static void printSequencesJSON(Print& out)
    {
    #ifdef STORAGE_FS
        bool first = true;
        File dir = STORAGE_FS.open("/seq");
        out.print("[");
        if (dir && dir.isDirectory())
        {
            File file = dir.openNextFile();
            while(file)
            {
                String fileName = file.name();
                if (!file.isDirectory() && fileName.endsWith(".z"))
                {
                    fileName = fileName.substring(0, fileName.length() - 2);
                    if (!first)
                        out.print(",");
                    out.print("{\"id\":\"");
                    out.print(fileName);
                    out.print("\",\"data\":\"");

                    for (size_t i = 0; i < file.size(); i++)
                    {
                        uint8_t b;
                        file.read(&b, sizeof(b));
                    }
                    file.seek(0, SeekSet);

                    StreamBase64Encoder encoder(file);
                    encoder.write(out);
                    out.print("\"}");
                    first = false;
                }
                file = dir.openNextFile();
            }
        }
        dir.close();
        out.print("]");
    #else
        out.print("[]");
    #endif
    }
};

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_WIFI_WEB
static bool sUpdateSettings;
#include "WebPages.h"
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_DROID_REMOTE
static bool sRemoteActive;
static SMQAddress sRemoteAddress;
#endif

///////////////////////////////////////////////////////////////////////////////

size_t countPCA9685Devices()
{
    unsigned nDevices = 0;
    byte previousAddress = 0;
    for (byte address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        if (error == 0)
        {
            // There is a conflict in the Adafruit address range. All call is 0x70
            // but the address that can be specified go from 0x40 - 0x7F. We require
            // sequential addresses so we limit the number of devices to 47 instead of
            // 63.
            if (address == PCA9685_ALL_CALL)
            {
                // All call address for PCA9685
            }
            // Support PCA9685 boards are in the range 0x40 - 0x6F
            else if (address >= PCA9685_START_ADDRESS && address <= PCA9685_END_ADDRESS)
            {
                // Require sequential addresses
                if (!previousAddress || previousAddress+1 == address)
                {
                    if (!previousAddress)
                        sPCA9685StartAddress = address;
                    nDevices++;            
                    previousAddress = address;
                    sPCA9685Connected[address - PCA9685_START_ADDRESS] = true;
                }
                else if (previousAddress != 0)
                {
                    sPCA9685Nonsequential = true;
                    sPCA9685Connected[address - PCA9685_START_ADDRESS] = true;
                }
            }
            else
            {
                // Ignore everything else.
            }
        }
    }
    return nDevices;
}

////////////////

#ifdef USE_LVGL_DISPLAY

static void astro_lvgl_flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

////////////////////////////////

bool lv_display_gif_until_complete(fs::File fd)
{
    if (!fd || fd.isDirectory())
    {
        DEBUG_PRINTLN("File not found");
        return false;
    }
    size_t imageSize = fd.available();
    char* imageData = (char*)ps_malloc(imageSize);
    if (imageData != nullptr)
    {
        size_t readBytes = fd.readBytes(imageData, imageSize);
        if (readBytes == imageSize)
        {
            static bool sSplashFinished;
            lv_img_dsc_t splash_gif;
            memset(&splash_gif, '\0', sizeof(splash_gif));
            splash_gif.header.cf = LV_IMG_CF_RAW_CHROMA_KEYED,
            splash_gif.data_size = imageSize;
            splash_gif.data = (const uint8_t*)imageData;
            lv_obj_t* splash_img = lv_gif_create(lv_scr_act());
            lv_obj_center(splash_img);
            lv_gif_set_src(splash_img, &splash_gif);
            ((lv_gif_t*)splash_img)->gif->loop_count = 1;
            lv_obj_add_event_cb(splash_img, [](lv_event_t* evt){
                sSplashFinished = true;
            }, LV_EVENT_READY, NULL);
            // Spin until gif is finished
            while (!sSplashFinished)
            {
                lv_timer_handler();
                delay(1);
            }
            lv_obj_del(splash_img);
            return true;
        }
        free(imageData);
    }
    else
    {
        DEBUG_PRINTLN("Failed to allocate space for image");
    }
    return false;
}

////////////////////////////////

#include "inc/StatusDisplayLVGL.h"
StatusDisplayLVGL sStatusDisplay;

void setupLVGLDisplay()
{
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    lv_init();
    lv_disp_buf = (lv_color_t *)heap_caps_malloc(SCREEN_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    tft.begin();          /* TFT init */
    tft.setRotation( 3 ); /* Landscape orientation, flipped */

    lv_disp_draw_buf_init(&disp_buf, lv_disp_buf, NULL, SCREEN_BUFFER_SIZE);

    /*Initialize the display*/
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = astro_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

#ifdef USE_TOUCH
    if (touch.init())
    {
        touch.setRotation(1);
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lv_touchpad_read;
        lv_indev_drv_register(&indev_drv);
    }
    else
    {
        DEBUG_PRINTLN("Failed to initialize touch screen");
    }
#endif

    lv_display_gif_until_complete(SPIFFS.open("/splash.gif"));

    button1.attachLongPressStart([]() {
        sStatusDisplay.wake();
    });
    button1.attachClick([]() {
        sStatusDisplay.wake();
        preferences.putBool(PREFERENCE_WIFI_ENABLED, !wifiEnabled);
        sStatusDisplay.showStatus(
            ((wifiEnabled) ? "WiFi Off" : "WiFi On"),
            []() {
                reboot();
            }
        );
    });
    button2.attachClick([]() {
        sStatusDisplay.wake();
    #ifdef USE_DROID_REMOTE
        if (SMQ::isPairing())
        {
            DEBUG_PRINTLN("Pairing Stopped ...");
            sStatusDisplay.showStatus("Stopped", []() {});
            SMQ::stopPairing();
        }
        else
        {
            DEBUG_PRINTLN("Pairing Started ...");
            SMQ::startPairing();
            sStatusDisplay.showStatus("Pairing");
        }
    #endif
    });
    button2.setPressTicks(LONGPRESS_DELAY);
    button2.attachLongPressStart([]() {
        sStatusDisplay.wake();
        if (preferences.remove(PREFERENCE_REMOTE_PAIRED))
        {
            DEBUG_PRINTLN("Unpairing Success...");
            sStatusDisplay.showStatus("Unpaired", []() {
                reboot();
            });
        }
        else
        {
            DEBUG_PRINTLN("Not Paired...");
        }
    });
}
#endif

void reboot()
{
    Serial.println("Restarting...");
#ifdef USE_DROID_REMOTE
    DisconnectRemote();
#endif
    unmountFileSystems();
    preferences.end();
    ESP.restart();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels -1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
/////////////////

#if defined(USE_WIFI) || defined(USE_DROID_REMOTE) || defined(USE_LVGL_DISPLAY)
void eventLoopTask(void* )
{
    for (;;)
    {
        if (wifiActive)
        {
        #ifdef USE_OTA
            ArduinoOTA.handle();
        #endif
        #ifdef USE_WIFI_WEB
            webServer.handle();
        #endif
        }
    #ifdef USE_DROID_REMOTE
        if (remoteActive)
        {
        #ifdef USE_SMQ
            SMQ::process();
        #endif
        }
    #endif
    #ifdef USE_LVGL_DISPLAY
        sStatusDisplay.refresh();
    #endif
        vTaskDelay(1);
    }
}
#endif

/////////////////

void setup()
{
    REELTWO_READY();

    PrintReelTwoInfo(Serial, "DarthServo");
    if (!mountReadOnlyFileSystem())
    {
        DEBUG_PRINTLN("Failed to mount filesystem");
    }
    if (!mountWritableFileSystem())
    {
        DEBUG_PRINTLN("Failed to mount storage filesystem");
    }
    else
    {
        // ensure directory exists (ignore failure)
        STORAGE_FS.mkdir("/seq");
    }

    if (!preferences.begin("darthservo", false))
    {
        DEBUG_PRINTLN("Failed to init prefs");
    }
#ifdef USE_WIFI
    wifiEnabled = wifiActive = preferences.getBool(PREFERENCE_WIFI_ENABLED, WIFI_ENABLED);
#endif
#ifdef USE_DROID_REMOTE
    remoteEnabled = remoteActive = preferences.getBool(PREFERENCE_REMOTE_ENABLED, REMOTE_ENABLED);
#endif

#ifdef SDA2_PIN
    Wire.begin(SDA2_PIN, SCL2_PIN);
#else
    Wire.begin(SDA_PIN, SCL_PIN);
#endif
    // Wire.setClock(400000); //Set i2c frequency to 400 kHz.

    // servoDispatch.setOutputEnablePin(OUTPUT_ENABLED_PIN, true);
    servoDispatch.setClockCalibration((const uint32_t[]) { 27570000, 27190000 });

    SetupEvent::ready();

#ifndef STORAGE_FS
    sSettingsPartition = find_partition((esp_partition_type_t)0xBA, (esp_partition_subtype_t)0xCA, NULL); 
    sWorkingPartition = find_partition((esp_partition_type_t)0xFE, (esp_partition_subtype_t)0xCA, NULL);
    sStoragePartition = find_partition((esp_partition_type_t)0xCA, (esp_partition_subtype_t)0xFE, NULL);
    sVirtuosoScript = find_partition((esp_partition_type_t)0xBA, (esp_partition_subtype_t)0xBE, NULL);
#endif
    sVirtuosoBytes = find_partition((esp_partition_type_t)0xBE, (esp_partition_subtype_t)0xBA, NULL);

    uint32_t startMillis = millis();
    sPCA9685Count = countPCA9685Devices();
    uint32_t stopMillis = millis();
    printf("Scan time: %u\n", stopMillis - startMillis);
    Serial.println("PCA9685 Start: "+String(sPCA9685StartAddress));
    Serial.println("PCA9685 Count: "+String(sPCA9685Count));
    Serial.println("PCA9685 Seq:   "+String(!sPCA9685Nonsequential));

    Serial.println("SPIFFS.usedBytes: "+String(SPIFFS.usedBytes()));
    Serial.println("SPIFFS.totalBytes: "+String(SPIFFS.totalBytes()));

    if (sPCA9685Count == 0)
    {
        Serial.println("No PCA9685 controller found.");
    }

    sNumServoNames = sPCA9685Count * 16;
    sServoNames = new String[sNumServoNames];
#ifdef STORAGE_FS
    ServoAnimation::loadSettings(STORAGE_FS, "/settings.bin");
    listDir(STORAGE_FS, "/", 3);

    {
        File f = STORAGE_FS.open("/order.z");
        if (f == true && !f.isDirectory())
        {
            StreamBase64Encoder encoder(STORAGE_FS, "/order.z");
            if (encoder.read())
            {
                Serial.println();
                encoder.write(Serial);
                Serial.println();
            }
        }
    }

#else
    ServoAnimation::loadSettings(sSettingsPartition);
#endif
 
#ifdef USE_LVGL_DISPLAY
    setupLVGLDisplay();
#endif

#ifdef USE_DROID_REMOTE
    if (remoteEnabled)
    {
    #ifdef USE_SMQ
        WiFi.mode(WIFI_MODE_APSTA);
        if (SMQ::init(preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_HOSTNAME),
                        preferences.getString(PREFERENCE_REMOTE_SECRET, SMQ_SECRET)))
        {
            SMQLMK key;
            if (preferences.getBytes(PREFERENCE_REMOTE_LMK, &key, sizeof(SMQLMK)) == sizeof(SMQLMK))
            {
                SMQ::setLocalMasterKey(&key);
            }

            SMQAddressKey pairedHosts[SMQ_MAX_PAIRED_HOSTS];
            size_t pairedHostsSize = preferences.getBytesLength(PREFERENCE_REMOTE_PAIRED);
            unsigned numHosts = pairedHostsSize / sizeof(pairedHosts[0]);
            printf("numHosts: %d\n", numHosts);
            Serial.print("WiFi.macAddress() : "); Serial.println(WiFi.macAddress());
            if (numHosts != 0)
            {
                if (preferences.getBytes(PREFERENCE_REMOTE_PAIRED, pairedHosts, pairedHostsSize) == pairedHostsSize)
                {
                    SMQ::addPairedHosts(numHosts, pairedHosts);
                }
            }
            printf("Droid Remote Enabled %s:%s\n",
                preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_HOSTNAME).c_str(),
                    preferences.getString(PREFERENCE_REMOTE_SECRET, SMQ_SECRET).c_str());
            SMQ::setHostPairingCallback([](SMQHost* host) {
                if (host == nullptr)
                {
                    printf("Pairing timed out\n");
                #ifdef USE_LVGL_DISPLAY
                    sStatusDisplay.showStatus("Timeout", []() {});
                #endif
                }
                else //if (host->hasTopic("LCD"))
                {
                    switch (SMQ::masterKeyExchange(&host->fLMK))
                    {
                        case -1:
                            printf("Pairing Stopped\n");
                        #ifdef USE_LVGL_DISPLAY
                            sStatusDisplay.showStatus("Stopped", []() {});
                        #endif
                            SMQ::stopPairing();
                            return;
                        case 1:
                            // Save new master key
                            SMQLMK lmk;
                            SMQ::getLocalMasterKey(&lmk);
                            printf("Saved new master key\n");
                            preferences.putBytes(PREFERENCE_REMOTE_LMK, &lmk, sizeof(lmk));
                            break;
                        case 0:
                            // We had the master key
                            break;
                    }
                    bool success = false;
                    printf("Pairing: %s [%s]\n", host->getHostName().c_str(), host->fLMK.toString().c_str());
                    if (SMQ::addPairedHost(&host->fAddr, &host->fLMK))
                    {
                        SMQAddressKey pairedHosts[SMQ_MAX_PAIRED_HOSTS];
                        unsigned numHosts = SMQ::getPairedHostCount();
                        if (SMQ::getPairedHosts(pairedHosts, numHosts) == numHosts)
                        {
                            preferences.putBytes(PREFERENCE_REMOTE_PAIRED,
                                pairedHosts, numHosts*sizeof(pairedHosts[0]));
                            printf("Pairing Success\n");
                        #ifdef USE_LVGL_DISPLAY
                            sStatusDisplay.showStatus("Success", []() {});
                        #endif
                            success = true;
                        }
                    }
                    printf("Pairing Stopped\n");
                    SMQ::stopPairing();
                    if (!success)
                    {
                    #ifdef USE_LVGL_DISPLAY
                        sStatusDisplay.showStatus("Stopped", []() {});
                    #endif
                    }
                }
            });

            SMQ::setHostDiscoveryCallback([](SMQHost* host) {
                if (host->hasTopic("LCD"))
                {
                    printf("Remote Discovered: %s\n", host->getHostName().c_str());
                }
            });

            SMQ::setHostLostCallback([](SMQHost* host) {
                printf("Lost: %s [%s] [%s]\n", host->getHostName().c_str(), host->getHostAddress().c_str(),
                    sRemoteAddress.toString().c_str());
                if (sRemoteAddress.equals(host->fAddr))
                {
                    printf("DISABLING REMOTE\n");
                    sDisplay.setEnabled(false);
                }
            });
        }
        else
        {
            printf("Failed to activate Droid Remote\n");
        }
    #endif
    }
#endif
#ifdef USE_WIFI
    if (wifiEnabled)
    {
    #ifdef USE_WIFI_WEB
        // In preparation for adding WiFi settings web page
        wifiAccess.setNetworkCredentials(
            preferences.getString(PREFERENCE_WIFI_SSID, getHostName()),
            preferences.getString(PREFERENCE_WIFI_PASS, WIFI_AP_PASSPHRASE),
            preferences.getBool(PREFERENCE_WIFI_AP, WIFI_ACCESS_POINT),
            preferences.getBool(PREFERENCE_WIFI_ENABLED, WIFI_ENABLED));
    #ifdef USE_WIFI_MARCDUINO
        wifiMarcduinoReceiver.setEnabled(preferences.getBool(PREFERENCE_MARCWIFI_ENABLED, MARC_WIFI_ENABLED));
        if (wifiMarcduinoReceiver.enabled())
        {
            wifiMarcduinoReceiver.setCommandHandler([](const char* cmd) {
                printf("PROCESS: \"%s\"\n", cmd);
                Marcduino::processCommand(player, cmd);
                if (preferences.getBool(PREFERENCE_MARCWIFI_SERIAL_PASS, MARC_WIFI_SERIAL_PASS))
                {
                    // TODO Forward serial commands received from R2Touch App
                    // COMMAND_SERIAL.print(cmd); COMMAND_SERIAL.print('\r');
                }
            });
        }
    #endif
        wifiAccess.notifyWifiConnected([](WifiAccess &wifi) {
        #ifdef STATUSLED_PIN
            statusLED.setMode(sCurrentMode = kWifiMode);
        #endif
            Serial.print("Connect to http://"); Serial.println(wifi.getIPAddress());
        #ifdef USE_MDNS
            // No point in setting up mDNS if R2 is the access point
            if (!wifi.isSoftAP() && webServer.enabled())
            {
                String hostName = getHostName();
                Serial.print("Host name: "); Serial.println(hostName);
                if (!MDNS.begin(hostName.c_str()))
                {
                    DEBUG_PRINTLN("Error setting up MDNS responder!");
                }
            }
        #endif
        });
    #endif
    #ifdef USE_OTA
        ArduinoOTA.onStart([]()
        {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
            {
                type = "sketch";
            }
            else // U_SPIFFS
            {
                type = "filesystem";
            }
            DEBUG_PRINTLN("OTA START");
        })
        .onEnd([]()
        {
            DEBUG_PRINTLN("OTA END");
        })
        .onProgress([](unsigned int progress, unsigned int total)
        {
            // float range = (float)progress / (float)total;
        })
        .onError([](ota_error_t error)
        {
            String desc;
            if (error == OTA_AUTH_ERROR) desc = "Auth Failed";
            else if (error == OTA_BEGIN_ERROR) desc = "Begin Failed";
            else if (error == OTA_CONNECT_ERROR) desc = "Connect Failed";
            else if (error == OTA_RECEIVE_ERROR) desc = "Receive Failed";
            else if (error == OTA_END_ERROR) desc = "End Failed";
            else desc = "Error: "+String(error);
            DEBUG_PRINTLN(desc);
        });
    #endif
    }
#endif

#ifdef USE_WIFI_WEB
    webServer.setConnect([]() {
        // Callback for each connected web client
        // DEBUG_PRINTLN("Hello");
    });
    webServer.setActivity([]() {
        // Stop testing animation player on web activity.
        // File decompression takes up too much memory.
        // Must release the memory for the web server to function.
        animationPlayer.stop();
    });
#endif

#if defined(USE_WIFI) || defined(USE_DROID_REMOTE) || defined(USE_LVGL_DISPLAY)
    xTaskCreatePinnedToCore(
          eventLoopTask,
          "Events",
          6000,    // shrink stack size?
          NULL,
          1,
          &eventTask,
          0);
#endif
}

////////////////

#ifdef USE_SMQ
SMQMESSAGE(DIAL, {
    long newValue = msg.get_int32("new");
    long oldValue = msg.get_int32("old");
    sDisplay.remoteDialEvent(newValue, oldValue);
})

///////////////////////////////////////////////////////////////////////////////

SMQMESSAGE(BUTTON, {
    uint8_t id = msg.get_uint8("id");
    bool pressed = msg.get_uint8("pressed");
    bool repeat = msg.get_uint8("repeat");
    sDisplay.remoteButtonEvent(id, pressed, repeat);
})

///////////////////////////////////////////////////////////////////////////////

SMQMESSAGE(SELECT, {
    printf("REMOTE ACTIVE\n");
    sRemoteActive = true;
    sRemoteAddress = SMQ::messageSender();
    sMainScreen.init();
    sDisplay.remoteActive();
})
#endif

#ifdef USE_DROID_REMOTE
static void DisconnectRemote()
{
#ifdef USE_SMQ
    if (sRemoteActive)
    {
        sRemoteActive = false;
        if (SMQ::sendTopic("EXIT", "Remote"))
        {
            SMQ::sendString("addr", SMQ::getAddress());
            SMQ::sendEnd();
            sDisplay.setEnabled(false);
        #ifdef STATUSLED_PIN
            statusLED.setMode(sCurrentMode = kNormalMode);
        #endif
        }
    }
#endif
}
#endif

////////////////

void virtuosoRun()
{
    interpreter.setBlock(sVirtuosoBytes);
    interpreter.reset();
}

////////////////

#ifdef USE_WIFI_WEB
static void updateSettings()
{
    Serial.println("Updated");
    sUpdateSettings = false;
}
#endif

////////////////

void loop()
{
#ifdef USE_WIFI_WEB
    if (sUpdateSettings)
    {
        updateSettings();
    }
#endif
    AnimatedEvent::process();
    animationPlayer.play();

#ifdef USE_MENUS
    sDisplay.process();
#endif
#ifdef USE_DEBUG
    if (DEBUG_SERIAL.available())
    {        
        int ch = DEBUG_SERIAL.read();
        if (ch == 'o')
        {
            servoDispatch.setServosEasingMethod(GROUP1, Easing::CircularEaseIn);
            SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelWave, GROUP1, 1000);
        }
        else if (ch == 'v')
        {
            virtuosoRun();
        }
        else if (ch == 'p')
        {
        #ifdef STORAGE_FS
            animationPlayer.startPlaying(STORAGE_FS, "/seq/f8994c2a-1722-4dfc-be51-48f58677dcae.z", 0);
        #else
            animationPlayer.startPlaying(sWorkingPartition, 0);
        #endif
        }
        else if (ch == 'F')
        {
            printf("Formatting ...\n");
            STORAGE_FS.end();
            STORAGE_FS.format();
            mountWritableFileSystem();
            listDir(STORAGE_FS, "/", 3);
        }
        else if (ch == '?')
        {
            Serial.print("Total heap:  "); Serial.println(ESP.getHeapSize());
            Serial.print("Free heap:   "); Serial.println(ESP.getFreeHeap());
            Serial.print("Total PSRAM: "); Serial.println(ESP.getPsramSize());
            Serial.print("Free PSRAM:  "); Serial.println(ESP.getFreePsram());
        }
        else
        {
            DEBUG_PRINTLN("STOP ALL SERVOS");
            servoSequencer.stop();
            servoDispatch.stop();
        }
    }
#endif
}
