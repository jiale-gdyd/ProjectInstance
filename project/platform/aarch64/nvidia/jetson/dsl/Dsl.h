#ifndef DSL_DSL_H
#define DSL_DSL_H

#include <cstdlib>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/rtsp-server/rtsp-server.h>
// #include <X11/Xlib.h>
// #include <X11/Xutil.h>

#include <queue>
#include <iomanip>
#include <iostream> 
#include <sstream>
#include <vector>
#include <map>
#include <list> 
#include <memory> 
#include <math.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <typeinfo>
#include <algorithm>
#include <random>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex>

#include <nvds_version.h>
#include <gstnvdsmeta.h>
#include <nvdsmeta_schema.h>
#include <gstnvdsinfer.h>
#include <gst-nvdssr.h>
#include <cuda_runtime_api.h>
#include <geos_c.h>
#include <curl/curl.h>

#include "DslUtilities.h"
#include "DslLog.h"
#include "DslMutex.h"
#include "DslCond.h"

#endif
