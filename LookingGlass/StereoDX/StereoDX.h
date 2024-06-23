#pragma once

#include "resource.h"

#define USE_TEXTURE
#include "../HoloDX.h"

#define USE_CV
//#define USE_CUDA
#include "../CVDX.h"

#define USE_LEAP
#include "../Leap.h"

class StereoDX : public DX, public Leap {};
