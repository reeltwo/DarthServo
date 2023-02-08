#include "inc/web-images.h"
#include "wifi/URLQueryString.h"

////////////////////////////////
// Main Menu
////////////////////////////////

WMenuData mainMenu[] = {
    { "Animate", "/animate" },
    { "Setup", "/setup" }
};

WElement mainContents[] = {
    WVerticalMenu("menu", mainMenu, SizeOfArray(mainMenu)),
    rseriesSVG
};

////////////////////////////////
// Setup Menu
////////////////////////////////

WMenuData setupMenu[] = {
    { "Home", "/" },
    { "PCA9685", "/pca9685" },
    { "Marcduino", "/marcduino" },
    { "WiFi", "/wifi" },
    { "Firmware", "/firmware" },
    { "Back", "/" }
};

WElement setupContents[] = {
    WVerticalMenu("setup", setupMenu, SizeOfArray(setupMenu)),
    rseriesSVG
};

////////////////////////////////
// PCA9685 Setup
////////////////////////////////

class WPCA9685Addresses : public WDynamic
{
public:
    WPCA9685Addresses(String id) :
        fID(id)
    {
    }

    virtual void emitCSS(Print& out) const override
    {
        out.println(String(fID)+"._css { width: 300px; }");
    }

    virtual void emitBody(Print& out) const override
    {
        byte previousAddress = 0;
        bool sequential = true;
        for (unsigned i = 0; i < PCA9685_END_ADDRESS - PCA9685_START_ADDRESS; i++)
        {
            byte addr = i + PCA9685_START_ADDRESS;
            if (sPCA9685Connected[i])
            {
                if (sequential && (!previousAddress || previousAddress+1 == addr))
                {
                    out.println("<p><label class='"+fID+"_css'>Found 0x"+String(addr, HEX)+" ("+String(addr)+")</label></p>");
                    previousAddress = addr;
                }
                else
                {
                    out.println("<p><label class='"+fID+"_css'>Non-sequential 0x"+String(addr, HEX)+" ("+String(addr)+") <b>Inactive</b></label></p>");
                    sequential = false;
                }
            }
        }
    }

protected:
    String fID;
};

WPCA9685Addresses pca9685Connected("connected");

WElement pca9685Contents[] = {
    W1("PCA9685 Setup"),
    WTextFieldIntegerRange("Count:", "pca9685",
        1, PCA9685_END_ADDRESS - PCA9685_START_ADDRESS,
        []()->String { return String(sPCA9685Count); },
        [](String val) { ; } ),
    WLabel("Currently Connected:", "label"),
    WDynamicElement(pca9685Connected),
    WButton("Back", "back", "/setup"),
    WHorizontalAlign(),
    WButton("Home", "home", "/"),
    WVerticalAlign(),
    rseriesSVG
};

////////////////////////////////
// Marcduino Setup
////////////////////////////////

String swBaudRates[] = {
    "2400",
    "9600",
};

int marcSerial1Baud;
int marcSerial2Baud;
bool marcSerialPass;
bool marcSerialEnabled;
bool marcWifiEnabled;
bool marcWifiSerialPass;

WElement marcduinoContents[] = {
    WSelect("Serial1 Baud Rate", "serial1baud",
        swBaudRates, SizeOfArray(swBaudRates),
        []() { return (marcSerial1Baud = (preferences.getInt(PREFERENCE_MARCSERIAL1, MARC_SERIAL1_BAUD_RATE)) == 2400) ? 0 : 1; },
        [](int val) { marcSerial1Baud = (val == 0) ? 2400 : 9600; } ),
    WSelect("Serial2 Baud Rate", "serial2baud",
        swBaudRates, SizeOfArray(swBaudRates),
        []() { return (marcSerial2Baud = (preferences.getInt(PREFERENCE_MARCSERIAL2, MARC_SERIAL2_BAUD_RATE)) == 2400) ? 0 : 1; },
        [](int val) { marcSerial2Baud = (val == 0) ? 2400 : 9600; } ),
    WCheckbox("Serial1 pass-through to Serial2", "serialpass",
        []() { return (marcSerialPass = (preferences.getBool(PREFERENCE_MARCSERIAL_PASS, MARC_SERIAL_PASS))); },
        [](bool val) { marcSerialPass = val; } ),
    WCheckbox("Marcduino on Serial1", "enabled",
        []() { return (marcSerialEnabled = (preferences.getBool(PREFERENCE_MARCSERIAL_ENABLED, MARC_SERIAL_ENABLED))); },
        [](bool val) { marcSerialEnabled = val; } ),
    WCheckbox("Marcduino on Wifi (port 2000)", "wifienabled",
        []() { return (marcWifiEnabled = (preferences.getBool(PREFERENCE_MARCWIFI_ENABLED, MARC_WIFI_ENABLED))); },
        [](bool val) { marcWifiEnabled = val; } ),
    WCheckbox("Marcduino Wifi pass-through to Serial2", "wifipass",
        []() { return (marcWifiSerialPass = (preferences.getBool(PREFERENCE_MARCWIFI_SERIAL_PASS, MARC_WIFI_SERIAL_PASS))); },
        [](bool val) { marcWifiSerialPass = val; } ),
    WButton("Save", "save", []() {
        preferences.putInt(PREFERENCE_MARCSERIAL1, marcSerial1Baud);
        preferences.putInt(PREFERENCE_MARCSERIAL2, marcSerial2Baud);
        preferences.putBool(PREFERENCE_MARCSERIAL_PASS, marcSerialPass);
        preferences.putBool(PREFERENCE_MARCSERIAL_ENABLED, marcSerialEnabled);
        preferences.putBool(PREFERENCE_MARCWIFI_ENABLED, marcWifiEnabled);
        preferences.putBool(PREFERENCE_MARCWIFI_SERIAL_PASS, marcWifiSerialPass);
    }),
    WHorizontalAlign(),
    WButton("Back", "back", "/setup"),
    WHorizontalAlign(),
    WButton("Home", "home", "/"),
    WVerticalAlign(),
    rseriesSVG
};

////////////////////////////////
// Wifi Setup
////////////////////////////////

String wifiSSID;
String wifiPass;
bool wifiAP;

WElement wifiContents[] = {
    W1("WiFi Setup"),
    WCheckbox("WiFi Enabled", "enabled",
        []() { return (wifiEnabled = preferences.getBool(PREFERENCE_WIFI_ENABLED, WIFI_ENABLED)); },
        [](bool val) { wifiEnabled = val; } ),
    WCheckbox("Access Point", "apmode",
        []() { return (wifiAP = preferences.getBool(PREFERENCE_WIFI_AP, WIFI_ACCESS_POINT)); },
        [](bool val) { wifiAP = val; } ),
    WTextField("WiFi:", "wifi",
        []()->String { return (wifiSSID = preferences.getString(PREFERENCE_WIFI_SSID, WIFI_AP_NAME)); },
        [](String val) { wifiSSID = val; } ),
    WPassword("Password:", "password",
        []()->String { return (wifiPass = preferences.getString(PREFERENCE_WIFI_PASS, WIFI_AP_PASSPHRASE)); },
        [](String val) { wifiPass = val; } ),
    WButton("Save", "save", []() {
        DEBUG_PRINTLN("WiFi Changed");
        preferences.putBool(PREFERENCE_WIFI_ENABLED, wifiEnabled);
        preferences.putBool(PREFERENCE_WIFI_AP, wifiAP);
        preferences.putString(PREFERENCE_WIFI_SSID, wifiSSID);
        preferences.putString(PREFERENCE_WIFI_PASS, wifiPass);
        DEBUG_PRINTLN("Restarting");
        preferences.end();
        ESP.restart();
    }),
    WHorizontalAlign(),
    WButton("Back", "back", "/setup"),
    WHorizontalAlign(),
    WButton("Home", "home", "/"),
    WVerticalAlign(),
    rseriesSVG
};

////////////////////////////////
// Firmware Upgrade
////////////////////////////////

WElement firmwareContents[] = {
    W1("Firmware Setup"),
    WFirmwareFile("Firmware:", "firmware"),
    WFirmwareUpload("Reflash", "firmware"),
    WLabel("Current Firmware Build Date:", "label"),
    WLabel(__DATE__, "date"),
    WButton("Clear Prefs", "clear", []() {
        DEBUG_PRINTLN("Clear all preference settings");
        preferences.clear();
    }),
    WHorizontalAlign(),
    WButton("Reboot", "reboot", []() {
        DEBUG_PRINTLN("Rebooting");
        preferences.end();
        ESP.restart();
    }),
    WHorizontalAlign(),
    WButton("Back", "back", "/setup"),
    WHorizontalAlign(),
    WButton("Home", "home", "/"),
    WVerticalAlign(),
    rseriesSVG
};

//////////////////////////////////////////////////////////////////

WPage pages[] = {
    WPage("/", mainContents, SizeOfArray(mainContents), "Darth Servo"),
    WPage("/animate", &SYSTEM_FS, "/animate.html", nullptr, 0),
    WPage("/css/main.css", &SYSTEM_FS, "/css/main.css"),
    WPage("/favicon.ico", &SYSTEM_FS, "image/x-icon"),
    WPage("/font.woff2", &SYSTEM_FS, "font/woff2"),
    WPage("/ace.js", &SYSTEM_FS, "text/javascript"),
    WPage("/FileSaver.js", &SYSTEM_FS, "text/javascript"),
    WPage("/virtuoso.js", &SYSTEM_FS, "text/javascript"),
    WPage("/virtuoso.wasm", &SYSTEM_FS, "application/wasm"),
    WAPI("/api/load", [](Print& out, String query) {
        out.println("HTTP/1.0 200 OK");
        out.println("Content-type:application/json");
        out.println("Connection: close");
        out.println();
        out.print("{\"seq\":");
        ServoAnimation::printSequencesJSON(out);
        out.print(",\"settings\":");
        ServoAnimation::printSettingsJSON(out);

        StreamBase64Encoder encoder(STORAGE_FS, "/script.z");
        if (encoder.read())
        {
            out.print(",\"virtuoso\":\"");
            encoder.write(out);
            out.print("\"");
        }
        encoder.reset(STORAGE_FS, "/order.z");
        if (encoder.read())
        {
            out.print(",\"order\":\"");
            encoder.write(out);
            out.print("\"");
        }
        out.println("}");
    }),
    WAPI("/api/getSequences", [](Print& out, String query) {
        out.println("HTTP/1.0 200 OK");
        out.println("Content-type:application/json");
        out.println("Connection: close");
        out.println();

        ServoAnimation::printSequencesJSON(out);
    }),
    WAPI("/api/getSettings", [](Print& out, String query) {
        out.println("HTTP/1.0 200 OK");
        out.println("Content-type:application/json");
        out.println("Connection: close");
        out.println();
        ServoAnimation::printSettingsJSON(out);
    }),
    WAPI("/api/play", [](Print& out, String query) {
        String id;
        URLQueryString params(query);
        if (params.get("id", id))
        {
            String str;
            unsigned startFrame = 0;
            if (params.getOptional("num", str))
            {
                startFrame = str.toInt();
            }
            String fileName = String("/seq/")+id+".z";
            printf("Playing: %s starting at %d\n", fileName.c_str(), startFrame);
            if (animationPlayer.startPlaying(STORAGE_FS, fileName.c_str(), startFrame))
            {
                out.println("HTTP/1.0 200 OK");
                out.println("Content-type:application/json");
                out.println("Connection: close");
                out.println();
            }
            else
            {
                out.println("HTTP/1.0 404 NOT FOUND");
                out.println("Connection: close");
                out.println();
            }
        }
        else
        {
            out.println("HTTP/1.0 400 BAD REQUEST");
            out.println("Connection: close");
            out.println();
        }
    }),
    WAPI("/api/stop", [](Print& out, String query) {
        // Stop everything
        animationPlayer.stop();
        servoSequencer.stop();
        servoDispatch.stop();
        out.println("HTTP/1.0 200 OK");
        out.println("Content-type:application/json");
        out.println("Connection: close");
        out.println();
    }),
    WAPI("/api/delete", [](Print& out, String query) {
        String id;
        URLQueryString params(query);
        if (params.get("id", id))
        {
            String fileName = String("/seq/")+id+".z";
            printf("Deleting: %s\n", fileName.c_str());
            if (STORAGE_FS.remove(fileName.c_str()))
            {
                printf("Deleted\n");
                out.println("HTTP/1.0 200 OK");
                out.println("Content-type:application/json");
                out.println("Connection: close");
                out.println();
            }
            else
            {
                printf("Not Found?\n");
                out.println("HTTP/1.0 404 NOT FOUND");
                out.println("Connection: close");
                out.println();
            }
        }
        else
        {
            out.println("HTTP/1.0 400 BAD REQUEST");
            out.println("Connection: close");
            out.println();
        }
    }),
    WAPI("/api/changeSetting", [](Print& out, String query) {
        String str;
        URLQueryString params(query);
        if (params.get("srv", str))
        {
            bool ok = false;
            if (str.equalsIgnoreCase("ALL"))
            {
                if (params.getOptional("ena", str))
                {
                    bool value = str.equalsIgnoreCase("true");
                    for (unsigned i = 0; i < SizeOfArray(servoSettings); i++)
                    {
                        servoDispatch.setPin(i, value ? servoSettings[i].pinNum : 0);
                    }
                    ok = true;
                }
            }
            else
            {
                unsigned servoIndex = str.toInt();
                bool inRange = (servoIndex < SizeOfArray(servoSettings));
                if (params.getOptional("ena", str))
                {
                    if (inRange) {
                        servoDispatch.setPin(servoIndex,
                            str.equalsIgnoreCase("true") ? servoSettings[servoIndex].pinNum : 0);
                        ok = true;
                    }
                }
                if (params.getOptional("nam", str))
                {
                    DEBUG_PRINT("NAME: "); DEBUG_PRINTLN(str);
                    setServoName(servoIndex, str);
                    ok = inRange;
                }
                if (params.getOptional("min", str))
                {
                    DEBUG_PRINT("MIN: "); DEBUG_PRINTLN(str);
                    servoDispatch.setStart(servoIndex, str.toInt());
                    ok = inRange;
                }
                if (params.getOptional("max", str))
                {
                    DEBUG_PRINT("MAX: "); DEBUG_PRINTLN(str);
                    servoDispatch.setEnd(servoIndex, str.toInt());
                    ok = inRange;
                }
                if (params.getOptional("useerr", str))
                {
                    DEBUG_PRINT("USERR: "); DEBUG_PRINTLN(str);
                    ok = inRange;
                }
                if (params.getOptional("err", str))
                {
                    DEBUG_PRINT("ERR: "); DEBUG_PRINTLN(str);
                    servoDispatch.setNeutral(servoIndex, str.toInt());
                    ok = inRange;
                }
                if (params.getOptional("pos", str))
                {
                    uint16_t pos = str.toInt();
                    DEBUG_PRINT("SET POS: "); DEBUG_PRINTLN(pos);
                    servoDispatch.setServoEasingMethod(servoIndex, nullptr);
                    servoDispatch.moveTo(servoIndex, pos / 1000.0);
                    ok = inRange;
                }
            }
            if (ok)
            {
                out.println("HTTP/1.0 200 OK");
                out.println("Content-type:application/json");
                out.println("Connection: close");
                out.println();
            }
            else
            {
                out.println("HTTP/1.0 404 NOT FOUND");
                out.println("Connection: close");
                out.println();
            }
        }
        else
        {
            out.println("HTTP/1.0 400 BAD REQUEST");
            out.println("Connection: close");
            out.println();
        }
    }),
    WAPI("/api/download/script", [](Print& out, String query) {
        StreamBase64Encoder encoder(STORAGE_FS, "/script.z");

        out.println("HTTP/1.0 200 OK");
        out.println("Content-type:application/base64");
        out.println("Connection: close");
        out.println();
        if (encoder.read())
        {
            encoder.write(out);
        }
    }),
    WAPI("/api/download/order", [](Print& out, String query) {
        StreamBase64Encoder encoder(STORAGE_FS, "/order4");

        out.println("HTTP/1.0 200 OK");
        out.println("Content-type:application/base64");
        out.println("Connection: close");
        out.println();
        if (encoder.read())
        {
            encoder.write(out);
        }
    }),
    WUpload("/api/upload/sequence",
        [](Client& client)
        {
            if (streamBase64Decoder.successful())
            {
                client.println("HTTP/1.0 200 OK");
                client.println();
                client.stop();
            }
            else
            {
                client.println("HTTP/1.0 500 Internal Server Error");
                client.println();
                client.stop();
            }
        },
        [](WUploader& upload)
        {
            if (upload.status == UPLOAD_FILE_START)
            {
                String id;
                URLQueryString params(upload.queryString);
                if (params.get("id", id))
                {
                    char filename[64];
                    snprintf(filename, sizeof(filename)-1, "/seq/%s.z", id.c_str());

                    // ensure directory exists (ignore failure)
                    STORAGE_FS.mkdir("/seq");
                    streamBase64Decoder.init(STORAGE_FS, filename);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                float range = (float)(upload.receivedSize+upload.currentSize) / (float)upload.fileSize;
                printf("Received: %d%% receivedSize=%d fileSize=%d\n", int(round(range*100)), upload.receivedSize, upload.fileSize);
                streamBase64Decoder.received(upload.buf, upload.currentSize);
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                streamBase64Decoder.end();
            }
        }
    ),
    WUpload("/api/upload/script",
        [](Client& client)
        {
            if (streamBase64Decoder.successful())
            {
                client.println("HTTP/1.0 200 OK");
                client.println();
                client.stop();
            }
            else
            {
                client.println("HTTP/1.0 500 Internal Server Error");
                client.println();
                client.stop();
            }
        },
        [](WUploader& upload)
        {
            if (upload.status == UPLOAD_FILE_START)
            {
                streamBase64Decoder.init(STORAGE_FS, "/script.z");
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                float range = (float)(upload.receivedSize+upload.currentSize) / (float)upload.fileSize;
                printf("Received: %d%% receivedSize=%d fileSize=%d\n", int(round(range*100)), upload.receivedSize, upload.fileSize);
                streamBase64Decoder.received(upload.buf, upload.currentSize);
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                streamBase64Decoder.end();
            }
        }
    ),
    WUpload("/api/upload/order",
        [](Client& client)
        {
            if (streamBase64Decoder.successful())
            {
                client.println("HTTP/1.0 200 OK");
                client.println();
                client.stop();
            }
            else
            {
                client.println("HTTP/1.0 500 Internal Server Error");
                client.println();
                client.stop();
            }
        },
        [](WUploader& upload)
        {
            if (upload.status == UPLOAD_FILE_START)
            {
                streamBase64Decoder.init(STORAGE_FS, "/order.z");
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                float range = (float)(upload.receivedSize+upload.currentSize) / (float)upload.fileSize;
                printf("Received: %d%% receivedSize=%d fileSize=%d\n", int(round(range*100)), upload.receivedSize, upload.fileSize);
                streamBase64Decoder.received(upload.buf, upload.currentSize);
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                streamBase64Decoder.end();
            }
        }
    ),
    WUpload("/api/upload/settings",
        [](Client& client)
        {
            if (streamBase64Decoder.successful())
            {
                client.println("HTTP/1.0 200 OK");
                client.println();
                client.stop();
            }
            else
            {
                client.println("HTTP/1.0 500 Internal Server Error");
                client.println();
                client.stop();
            }
        },
        [](WUploader& upload)
        {
            if (upload.status == UPLOAD_FILE_START)
            {
                streamBase64Decoder.init(STORAGE_FS, "/settings.bin");
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                float range = (float)(upload.receivedSize+upload.currentSize) / (float)upload.fileSize;
                printf("Received: %d%% receivedSize=%d fileSize=%d\n", int(round(range*100)), upload.receivedSize, upload.fileSize);
                streamBase64Decoder.received(upload.buf, upload.currentSize);
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                streamBase64Decoder.end();
            }
        }
    ),
    WUpload("/api/upload/virtuoso",
        [](Client& client)
        {
            if (streamBase64Decoder.successful())
            {
                client.println("HTTP/1.0 200 OK");
                client.println();
                client.stop();
            }
            else
            {
                client.println("HTTP/1.0 500 Internal Server Error");
                client.println();
                client.stop();
            }
        },
        [](WUploader& upload)
        {
            if (upload.status == UPLOAD_FILE_START)
            {
                streamBase64Decoder.init(VIRTUOSO_MAGIC_SIGNATURE, sVirtuosoBytes);
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                float range = (float)(upload.receivedSize+upload.currentSize) / (float)upload.fileSize;
                printf("Received: %d%% receivedSize=%d fileSize=%d\n", int(round(range*100)), upload.receivedSize, upload.fileSize);
                streamBase64Decoder.received(upload.buf, upload.currentSize);
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                streamBase64Decoder.end();
            }
        }
    ),
    WPage("/setup", setupContents, SizeOfArray(setupContents), "Setup"),
      WPage("/pca9685", pca9685Contents, SizeOfArray(pca9685Contents), "PCA9685"),
      WPage("/marcduino", marcduinoContents, SizeOfArray(marcduinoContents), "Marcduino"),
      WPage("/wifi", wifiContents, SizeOfArray(wifiContents), "WiFi"),
      WPage("/firmware", firmwareContents, SizeOfArray(firmwareContents), "Firmware"),
            WUpload("/upload/firmware",
            [](Client& client)
            {
                if (Update.hasError())
                    client.println("HTTP/1.0 200 FAIL");
                else
                    client.println("HTTP/1.0 200 OK");
                client.println("Content-type:text/html");
                client.println("Vary: Accept-Encoding");
                client.println();
                client.println();
                client.stop();
                if (!Update.hasError())
                {
                    delay(1000);
                    preferences.end();
                    ESP.restart();
                }
                otaInProgress = false;
            },
            [](WUploader& upload)
            {
                if (upload.status == UPLOAD_FILE_START)
                {
                    otaInProgress = true;
                    unmountFileSystems();
                    Serial.printf("Update: %s\n", upload.filename.c_str());
                    if (!Update.begin(upload.fileSize))
                    {
                        //start with max available size
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_WRITE)
                {
                    float range = (float)(upload.receivedSize+upload.currentSize) / (float)upload.fileSize;
                    DEBUG_PRINTLN("Received: "+String(range*100)+"%");
                   /* flashing firmware to ESP*/
                    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                    {
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_END)
                {
                    DEBUG_PRINTLN("GAME OVER");
                    if (Update.end(true))
                    {
                        //true to set the size to the current progress
                        Serial.printf("Update Success: %u\nRebooting...\n", upload.receivedSize);
                    }
                    else
                    {
                        Update.printError(Serial);
                    }
                }
            })
};

WifiWebServer<10,SizeOfArray(pages)> webServer(pages, wifiAccess);
