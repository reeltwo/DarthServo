#!/bin/sh
INSTALLER_DIR=$HOME/Installers/reeltwo-darthservo-installer/docs/artifacts
rm -f data/splash.gif

make clean && make 4MB=1 firmware_data && cp -f .build/DarthServo*-*.bin $INSTALLER_DIR/

make clean && make firmware_data && cp -f .build/DarthServo*-*.bin $INSTALLER_DIR/

make clean && make PENUMBRA=1 firmware_data && cp -f .build/DarthServo*-*.bin $INSTALLER_DIR/

make clean && make BACKPACK=1 firmware_data && cp -f .build/DarthServo*-*.bin $INSTALLER_DIR/

make clean && make TARGET=ESP32S3 TOUCH=1 firmware_data && cp -f .build/DarthServo*-*.bin $INSTALLER_DIR/
