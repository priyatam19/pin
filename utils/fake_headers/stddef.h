#ifndef PIN_FAKE_STDDEF_H
#define PIN_FAKE_STDDEF_H
#ifdef __cplusplus
extern "C" {
#endif
#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long size_t;
#endif
#if defined(__PTRDIFF_TYPE__)
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#else
typedef long ptrdiff_t;
#endif
typedef long intptr_t;
typedef unsigned long uintptr_t;
#define NULL ((void*)0)
#ifdef __cplusplus
}
#endif
#endif
