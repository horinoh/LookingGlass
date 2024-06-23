#include "CV.h"

#ifdef USE_CUDA
#ifdef _DEBUG
#pragma comment(lib, "aded.lib")
#pragma comment(lib, "IlmImfd.lib")
#pragma comment(lib, "ippicvmt.lib")
#pragma comment(lib, "ippiwd.lib")
#pragma comment(lib, "ittnotifyd.lib")
#pragma comment(lib, "libjpeg-turbod.lib")
#pragma comment(lib, "libopenjp2d.lib")
#pragma comment(lib, "libpngd.lib")
#pragma comment(lib, "libprotobufd.lib")
#pragma comment(lib, "libtiffd.lib")
#pragma comment(lib, "libwebpd.lib")
#pragma comment(lib, "opencv_img_hash4100d.lib")
#pragma comment(lib, "opencv_world4100d.lib")
#pragma comment(lib, "zlibd.lib")
#else
#pragma comment(lib, "ade.lib")
#pragma comment(lib, "IlmImf.lib")
#pragma comment(lib, "ippicvmt.lib")
#pragma comment(lib, "ippiw.lib")
#pragma comment(lib, "ittnotify.lib")
#pragma comment(lib, "libjpeg-turbo.lib")
#pragma comment(lib, "libopenjp2.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "libtiff.lib")
#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "opencv_img_hash4100.lib")
#pragma comment(lib, "opencv_world4100.lib")
#pragma comment(lib, "zlib.lib")
#endif
#pragma comment(lib, "cudart_static.lib")
#else
#ifdef _DEBUG
#pragma comment(lib, "opencv_world490d.lib")
#else
#pragma comment(lib, "opencv_world490.lib")
#endif
#endif