/*
 * Module: semaphore.h
 *
 * Purpose:
 *	Semaphores aren't actually part of the PThreads standard.
 *	They are defined by the POSIX Standard:
 *
 *		POSIX 1003.1b-1993	(POSIX.1b)
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads4w - POSIX Threads for Windows
 *      Copyright 1998 John E. Bossom
 *      Copyright 1999-2018, Pthreads4w contributors
 *
 *      Homepage: https://sourceforge.net/projects/pthreads4w/
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *
 *      https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#if !defined( SEMAPHORE_H )
#define SEMAPHORE_H

/* FIXME: POSIX.1 says that _POSIX_SEMAPHORES should be defined
 * in <unistd.h>, not here; for later POSIX.1 versions, its value
 * should match the corresponding _POSIX_VERSION number, but in
 * the case of POSIX.1b-1993, the value is unspecified.
 *
 * Notwithstanding the above, since POSIX semaphores, (and indeed
 * having any <unistd.h> to #include), are not a standard feature
 * on MS-Windows, it is convenient to retain this definition here;
 * we may consider adding a hook, to make it selectively available
 * for inclusion by <unistd.h>, in those cases (e.g. MinGW) where
 * <unistd.h> is provided.
 */
#define _POSIX_SEMAPHORES

/* Internal macros, common to the public interfaces for various
 * pthreads-win32 components, are defined in <_ptw32.h>; we must
 * include them here.
 */
#include <_ptw32.h>

/* The sem_timedwait() function was added in POSIX.1-2001; it
 * requires struct timespec to be defined, at least as a partial
 * (a.k.a. incomplete) data type.  Forward declare it as such,
 * then include <time.h> selectively, to acquire a complete
 * definition, (if available).
 */
struct timespec;
#define __need_struct_timespec
#include <time.h>

/* The data type used to represent our semaphore implementation,
 * as required by POSIX.1; FIXME: consider renaming the underlying
 * structure tag, to avoid possible pollution of user namespace.
 */
typedef struct sem_t_ * sem_t;

/* POSIX.1b (and later) mandates SEM_FAILED as the value to be
 * returned on failure of sem_open(); (our implementation is a
 * stub, which will always return this).
 */
#define SEM_FAILED  (sem_t *)(-1)

__PTW32_BEGIN_C_DECLS

/* Function prototypes: some are implemented as stubs, which
 * always fail; (FIXME: identify them).
 */
__PTW32_DLLPORT int  __PTW32_CDECL sem_init (sem_t * sem,
					int pshared,
					unsigned int value);

__PTW32_DLLPORT int  __PTW32_CDECL sem_destroy (sem_t * sem);

__PTW32_DLLPORT int  __PTW32_CDECL sem_trywait (sem_t * sem);

__PTW32_DLLPORT int  __PTW32_CDECL sem_wait (sem_t * sem);

__PTW32_DLLPORT int  __PTW32_CDECL sem_timedwait (sem_t * sem,
					     const struct timespec * abstime);

__PTW32_DLLPORT int  __PTW32_CDECL sem_post (sem_t * sem);

__PTW32_DLLPORT int  __PTW32_CDECL sem_post_multiple (sem_t * sem,
						 int count);

__PTW32_DLLPORT sem_t *  __PTW32_CDECL sem_open (const char *, int, ...);

__PTW32_DLLPORT int  __PTW32_CDECL sem_close (sem_t * sem);

__PTW32_DLLPORT int  __PTW32_CDECL sem_unlink (const char * name);

__PTW32_DLLPORT int  __PTW32_CDECL sem_getvalue (sem_t * sem,
					    int * sval);

__PTW32_END_C_DECLS

#endif				/* !SEMAPHORE_H */
