#!/bin/sh

SELF_LIB_ROOT=/opt/aure

export PATH=$PATH:$SELF_LIB_ROOT/bin:$SELF_LIB_ROOT/sbin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SELF_LIB_ROOT/lib
export GST_PLUGIN_PATH=$SELF_LIB_ROOT/lib/gstreamer-1.0

# gst-launch-1.0 filesrc location=/root/movie.mp4 ! decodebin name=decoder decoder. ! queue ! videoconvert ! fbdevsink &
