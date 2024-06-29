#include "CV.h"

#ifdef _DEBUG
#pragma comment(lib, "opencv_img_hash4100d.lib")
#pragma comment(lib, "opencv_world4100d.lib")
#else
#pragma comment(lib, "opencv_img_hash4100.lib")
#pragma comment(lib, "opencv_world4100.lib")
#endif

#ifdef USE_CUDA
#pragma comment(lib, "cudart_static.lib")
#endif