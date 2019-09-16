/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/

#ifndef DEVICE
#define DEVICE GPU_TARGET
#endif

#include "llvm/SupportVISC/VISCHint.h"

#ifdef __cplusplus
extern "C" {
void __visc__hint(visc::Target);
//void __visc__wait(void*);
#else
void __visc__hint(enum Target);
//void __visc__wait(unsigned);
#endif

#ifdef __cplusplus
//void* __visc__node(...);
//void* __visc__createNode(...);
//void* __visc__createNode1D(...);
//void* __visc__createNode2D(...);
//void* __visc__createNode3D(...);
//void __visc__return(...);
#endif

void* __visc__createNodeND(unsigned,...);
void __visc__return(unsigned, ...);

void __visc__attributes(unsigned, ...);
void __visc__init();
void __visc__cleanup();

void __visc__bindIn(void*, unsigned, unsigned, unsigned);
void __visc__bindOut(void*, unsigned, unsigned, unsigned);
void* __visc__edge(void*, void*, unsigned, unsigned, unsigned, unsigned);
void __visc__push(void*, void*);
void* __visc__pop(void*);
void* __visc__launch(unsigned, ...);
void __visc__wait(void*);

void* __visc__getNode();
void* __visc__getParentNode(void*);
void __visc__barrier();
void* __visc__malloc(long);
long __visc__getNodeInstanceID_x(void*);
long __visc__getNodeInstanceID_y(void*);
long __visc__getNodeInstanceID_z(void*);
long __visc__getNumNodeInstances_x(void*);
long __visc__getNumNodeInstances_y(void*);
long __visc__getNumNodeInstances_z(void*);

// Atomic
// signed int
int __visc__atomic_cmpxchg(int*, int, int);
int __visc__atomic_add(int*, int);
int __visc__atomic_sub(int*, int);
int __visc__atomic_xchg(int*, int);
int __visc__atomic_inc(int*);
int __visc__atomic_dec(int*);
int __visc__atomic_min(int*, int);
int __visc__atomic_max(int*, int);
int __visc__atomic_umax(int*, int);
int __visc__atomic_umin(int*, int);
int __visc__atomic_and(int*, int);
int __visc__atomic_or(int*, int);
int __visc__atomic_xor(int*, int);

// Special Func
float __visc__floor(float);
float __visc__rsqrt(float);
float __visc__sqrt(float);
float __visc__sin(float);
float __visc__cos(float);
// unsigned int
//unsigned __visc__atomic_cmpxchg(unsigned*, unsigned, unsigned);
//unsigned __visc__atomic_add(unsigned*, unsigned);
//unsigned __visc__atomic_sub(unsigned*, unsigned);
//unsigned __visc__atomic_xchg(unsigned*, unsigned);
//unsigned __visc__atomic_inc(unsigned*);
//unsigned __visc__atomic_dec(unsigned*);
//unsigned __visc__atomic_min(unsigned*, unsigned);
//unsigned __visc__atomic_max(unsigned*, unsigned);
//unsigned __visc__atomic_and(unsigned*, unsigned);
//unsigned __visc__atomic_or(unsigned*, unsigned);
//unsigned __visc__atomic_xor(unsigned*, unsigned);


#include <unistd.h>

long get_global_id(int);
long get_group_id(int);
long get_local_id(int);
long get_local_size(int);


void llvm_visc_track_mem(void*, size_t);
void llvm_visc_untrack_mem(void*);
void llvm_visc_request_mem(void*, size_t);

#ifdef __cplusplus
}
#endif

