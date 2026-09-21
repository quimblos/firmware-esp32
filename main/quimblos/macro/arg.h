#include "deparen.h"

#define _ARG0(X, ...) X
#define ARG0(X) _ARG0(X)

#define _ARG1OF2(X, ...) __VA_ARGS__
#define ARG1OF2(X) _ARG1OF2(X)

#define _ARG1OF3(X, ...) _ARG0(__VA_ARGS__)
#define ARG1OF3(X) _ARG1OF3(X)

#define _ARG2OF3(X, ...) _ARG1OF2(__VA_ARGS__)
#define ARG2OF3(X) _ARG2OF3(X)