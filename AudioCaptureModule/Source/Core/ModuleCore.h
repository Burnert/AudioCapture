#pragma once

#define ACM_API __declspec(dllexport)

#define KILL_ON(x) \
if (x) { \
	_Kill(); \
}
#define KILL_ON_MB(x, message, caption) \
if (x) { \
	MessageBoxA(NULL, message, caption, MB_ICONERROR | MB_OK); \
	_Kill(); \
}

#define KILL_ON_FALSE(x)                        KILL_ON(!(x))
#define KILL_ON_FALSE_MB(x, message, caption)   KILL_ON_MB(!(x), message, caption)

#define KILL_ON_NULL(x)                         KILL_ON_FALSE(x)
#define KILL_ON_NULL_MB(x, message, caption)    KILL_ON_FALSE_MB(x, message, caption)

#define KILL_ON_INVALID_HANDLE(x)                        KILL_ON((x) == INVALID_HANDLE_VALUE)
#define KILL_ON_INVALID_HANDLE_MB(x, message, caption)   KILL_ON_MB((x) == INVALID_HANDLE_VALUE, message, caption)
