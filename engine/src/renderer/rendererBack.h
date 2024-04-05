#pragma once

#include "renderTypes.h"

b8 rendererCreate(rendererBackendAPI api, rendererBackend* rb);

b8 rendererDestroy(rendererBackend* rb);