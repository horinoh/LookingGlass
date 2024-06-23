#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloVK.h"

#define USE_CV
//#define USE_CUDA
#include "../CVVK.h"

#define USE_LEAP
#include "../Leap.h"

class StereoVK : public VK, public Leap {};