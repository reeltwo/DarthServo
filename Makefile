TARGET?=ESP32
ifeq ("$(TARGET)", "ESP32S3")
PORT?=/dev/ttyACM1
#ESP32_DATA=data32
ESP32_PSRAM=opi
ESP32S3_CDCONBOOT=cdc
INCLUDE_SPLASH?=data/splash.gif
else
PORT?=/dev/ttyUSB0
INCLUDE_SPLASH?=
endif
ESP32_DEBUGLEVEL=verbose
#ESP32_FILESYSTEM=littlefs
ESP32_FILESYSTEM=spiffs
ESP32_FILESYSTEM_PART=spiffs
ESP32_PARTSCHEME=min_spiffs
ifeq ("$(4MB)","1")
ESP32_FLASHSIZE=4MB
else
ESP32_FLASHSIZE=16MB
endif
ESP32_DATADEPEND=data_dependency
GITHUB_REPOS= \
reeltwo/Reeltwo

FILESYSTEM := 

ifeq ("$(TARGET)", "ESP32S3")
ifeq ("$(TOUCH)","1")
ARDUINO_OPTS+='-prefs="compiler.cpp.extra_flags=-DAMIDALA_DISPLAY=1 -DST7789V_DRIVER=1"'
else
ARDUINO_OPTS+='-prefs="compiler.cpp.extra_flags=-DAMIDALA_DISPLAY=1"'
endif
ESP32_FIRMWARE_NAME=DarthServoAmidala
FILESYSTEM += data/splash.gif
else ifeq ("$(BACKPACK)","1")
ARDUINO_OPTS+='-prefs="compiler.cpp.extra_flags=-DPCA9685_BACKPACK=1"'
ESP32_FIRMWARE_NAME=DarthServoYoda
else ifeq ("$(PENUMBRA)","1")
ARDUINO_OPTS+='-prefs="compiler.cpp.extra_flags=-DPENUMBRA=1'
ESP32_FIRMWARE_NAME=DarthServoPenumbra
else
ESP32_FIRMWARE_NAME=DarthServoGeneric$(ESP32_FLASHSIZE)
endif

include ../Arduino.mk

FILESYSTEM += data/favicon.ico.gz
FILESYSTEM += data/animate.html.gz
FILESYSTEM += data/font.woff2.gz
FILESYSTEM += data/ace.js.gz
FILESYSTEM += data/FileSaver.js.gz
FILESYSTEM += data/virtuoso.js.gz
FILESYSTEM += data/virtuoso.wasm.gz
FILESYSTEM += $(INCLUDE_SPLASH)

data/splash.gif: splash.tape
	vhs splash.tape

html/node_modules:
	cd html && npm i

html/site/index.html: html/node_modules html/index.html html/src/js/main.js html/src/scss/main.scss
	@echo Building site
	cd html && npm run build

data/animate.html.gz: html/site/index.html
	@echo Creating minimal font resource
	python3 tools/compact_font.py
	cat html/site/index.html | sed -f html/site/font.sed > data/animate.html
	gzip -f data/animate.html

data/font.woff2.gz: html/site/font.mini.woff2
	gzip -c html/site/font.mini.woff2 > data/font.woff2.gz

data/favicon.ico.gz: html/site/favicon.ico
	gzip -c html/site/favicon.ico > data/favicon.ico.gz

data/ace.js.gz: html/site/ace.js
	gzip -c html/site/ace.js > data/ace.js.gz

data/FileSaver.js.gz: html/site/FileSaver.js
	gzip -c html/site/FileSaver.js > data/FileSaver.js.gz

data/virtuoso.js.gz: wasm/virtuoso.js
	gzip -c wasm/virtuoso.js > data/virtuoso.js.gz

data/virtuoso.wasm.gz: wasm/virtuoso.wasm
	gzip -c wasm/virtuoso.wasm > data/virtuoso.wasm.gz

wasm/virtuoso.wasm wasm/virtuoso.js: wasm/virtuoso.cpp inc/Virtuoso.h
	@echo Compiling webassembly
	cd wasm && ./compile.sh

data_dependency: $(FILESYSTEM)
