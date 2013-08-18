/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

/* Optimized CPU kernel entry points. This file is compiled with SSE2
 * optimization flags and nearly all functions inlined, while kernel.cpp
 * is compiled without for other CPU's. */

#ifdef WITH_OPTIMIZED_KERNEL

#define __KERNEL_SSE2__

#include "kernel.h"
#include "kernel_compat_cpu.h"
#include "kernel_math.h"
#include "kernel_types.h"
#include "kernel_globals.h"
#include "kernel_film.h"
#include "kernel_path.h"
#include "kernel_displace.h"

CCL_NAMESPACE_BEGIN

/* Path Tracing */

void kernel_cpu_sse2_path_trace(KernelGlobals *kg, float *buffer, unsigned int *rng_state, int sample, int x, int y, int offset, int stride)
{
#ifdef __NON_PROGRESSIVE__
	if(!kernel_data.integrator.progressive)
		kernel_path_trace_non_progressive(kg, buffer, rng_state, sample, x, y, offset, stride);
	else
#endif
		kernel_path_trace_progressive(kg, buffer, rng_state, sample, x, y, offset, stride);
}

/* Tonemapping */

void kernel_cpu_sse2_tonemap(KernelGlobals *kg, uchar4 *rgba, float *buffer, int sample, int x, int y, int offset, int stride)
{
	kernel_film_tonemap(kg, rgba, buffer, sample, x, y, offset, stride);
}

/* Shader Evaluate */

void kernel_cpu_sse2_shader(KernelGlobals *kg, uint4 *input, float4 *output, int type, int i)
{
	kernel_shader_evaluate(kg, input, output, (ShaderEvalType)type, i);
}

CCL_NAMESPACE_END

#endif

