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

#include "llvm/SupportHPVM/HPVMHint.h"

#ifdef __cplusplus
extern "C" {
void __hpvm__hint(hpvm::Target);
//void __hpvm__wait(void*);
#else
void __hpvm__hint(enum Target);
//void __hpvm__wait(unsigned);
#endif

#ifdef __cplusplus
//void* __hpvm__node(...);
//void* __hpvm__createNode(...);
//void* __hpvm__createNode1D(...);
//void* __hpvm__createNode2D(...);
//void* __hpvm__createNode3D(...);
//void __hpvm__return(...);
#endif

void* __hpvm__createNodeND(unsigned,...);
void __hpvm__return(unsigned, ...);

void __hpvm__attributes(unsigned, ...);
void __hpvm__init();
void __hpvm__cleanup();

void __hpvm__bindIn(void*, unsigned, unsigned, unsigned);
void __hpvm__bindOut(void*, unsigned, unsigned, unsigned);
void* __hpvm__edge(void*, void*, unsigned, unsigned, unsigned, unsigned);
void __hpvm__push(void*, void*);
void* __hpvm__pop(void*);
void* __hpvm__launch(unsigned, ...);
void __hpvm__wait(void*);

void* __hpvm__getNode();
void* __hpvm__getParentNode(void*);
void __hpvm__barrier();
void* __hpvm__malloc(long);
long __hpvm__getNodeInstanceID_x(void*);
long __hpvm__getNodeInstanceID_y(void*);
long __hpvm__getNodeInstanceID_z(void*);
long __hpvm__getNumNodeInstances_x(void*);
long __hpvm__getNumNodeInstances_y(void*);
long __hpvm__getNumNodeInstances_z(void*);

// Atomic
// signed int
int __hpvm__atomic_cmpxchg(int*, int, int);
int __hpvm__atomic_add(int*, int);
int __hpvm__atomic_sub(int*, int);
int __hpvm__atomic_xchg(int*, int);
int __hpvm__atomic_inc(int*);
int __hpvm__atomic_dec(int*);
int __hpvm__atomic_min(int*, int);
int __hpvm__atomic_max(int*, int);
int __hpvm__atomic_umax(int*, int);
int __hpvm__atomic_umin(int*, int);
int __hpvm__atomic_and(int*, int);
int __hpvm__atomic_or(int*, int);
int __hpvm__atomic_xor(int*, int);

// Special Func
float __hpvm__floor(float);
float __hpvm__rsqrt(float);
float __hpvm__sqrt(float);
float __hpvm__sin(float);
float __hpvm__cos(float);
// unsigned int
//unsigned __hpvm__atomic_cmpxchg(unsigned*, unsigned, unsigned);
//unsigned __hpvm__atomic_add(unsigned*, unsigned);
//unsigned __hpvm__atomic_sub(unsigned*, unsigned);
//unsigned __hpvm__atomic_xchg(unsigned*, unsigned);
//unsigned __hpvm__atomic_inc(unsigned*);
//unsigned __hpvm__atomic_dec(unsigned*);
//unsigned __hpvm__atomic_min(unsigned*, unsigned);
//unsigned __hpvm__atomic_max(unsigned*, unsigned);
//unsigned __hpvm__atomic_and(unsigned*, unsigned);
//unsigned __hpvm__atomic_or(unsigned*, unsigned);
//unsigned __hpvm__atomic_xor(unsigned*, unsigned);


#include <unistd.h>

long get_global_id(int);
long get_group_id(int);
long get_local_id(int);
long get_local_size(int);


void llvm_hpvm_track_mem(void*, size_t);
void llvm_hpvm_untrack_mem(void*);
void llvm_hpvm_request_mem(void*, size_t);

#ifdef __cplusplus
}
#endif

