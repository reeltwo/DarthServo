#!/bin/sh
cd ..
python tools/compact_font.python
cat html/site/index.html | sed -f html/site/font.sed > data/animate.html
gzip -f data/animate.html