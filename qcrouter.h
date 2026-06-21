#pragma once

#if defined(_MSC_VER) && !defined(LIBAVOID_NO_DLL)
#define CIVITAS_EXPORT __declspec(dllexport)
#else
#define CIVITAS_EXPORT
#endif

#include <iterator>
#include "libavoid.h"

typedef std::vector<Avoid::ShapeRef*> ShapeSet;
typedef std::vector<Avoid::ConnRef*> ConnectorSet;
