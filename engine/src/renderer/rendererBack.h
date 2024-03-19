#pragma once

#include "renderTypes.h"

b8 rendererCreate(renderer_backend_API api, rendererBackend* rb);

b8 rendererDestroy(rendererBackend* rb);