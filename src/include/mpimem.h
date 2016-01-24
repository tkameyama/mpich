/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIMEM_H_INCLUDED
#define MPIMEM_H_INCLUDED

#ifndef MPICHCONF_H_INCLUDED
#error 'mpimem.h requires that mpichconf.h be included first'
#endif

/* Make sure that we have the definitions for the malloc routines and size_t */
#include <stdio.h>
#include <stdlib.h>
/* strdup is often declared in string.h, so if we plan to redefine strdup, 
   we need to include string first.  That is done below, only in the
   case where we redefine strdup */

#if defined(__cplusplus)
extern "C" {
#endif

#include "mpichconf.h"
#include "mpl.h"

/* Define attribute as empty if it has no definition */
#ifndef ATTRIBUTE
#define ATTRIBUTE(a)
#endif

#if defined (MPL_USE_DBG_LOGGING)
extern MPL_DBG_Class MPIR_DBG_STRING;
#endif /* MPL_USE_DBG_LOGGING */

/* ------------------------------------------------------------------------- */
/* mpimem.h */
/* ------------------------------------------------------------------------- */
/* Memory allocation */
/* style: allow:malloc:2 sig:0 */
/* style: allow:free:2 sig:0 */
/* style: allow:strdup:2 sig:0 */
/* style: allow:calloc:2 sig:0 */
/* style: allow:realloc:1 sig:0 */
/* style: allow:alloca:1 sig:0 */
/* style: define:__strdup:1 sig:0 */
/* style: define:strdup:1 sig:0 */
/* style: allow:fprintf:5 sig:0 */   /* For handle debugging ONLY */
/* style: allow:snprintf:1 sig:0 */

/*D
  Memory - Memory Management Routines

  Rules for memory management:

  MPICH explicity prohibits the appearence of 'malloc', 'free', 
  'calloc', 'realloc', or 'strdup' in any code implementing a device or 
  MPI call (of course, users may use any of these calls in their code).  
  Instead, you must use 'MPL_malloc' etc.; if these are defined
  as 'malloc', that is allowed, but an explicit use of 'malloc' instead of
  'MPL_malloc' in the source code is not allowed.  This restriction is
  made to simplify the use of portable tools to test for memory leaks, 
  overwrites, and other consistency checks.

  Most memory should be allocated at the time that 'MPID_Init' is 
  called and released with 'MPID_Finalize' is called.  If at all possible,
  no other MPID routine should fail because memory could not be allocated
  (for example, because the user has allocated large arrays after 'MPI_Init').
  
  The implementation of the MPI routines will strive to avoid memory allocation
  as well; however, operations such as 'MPI_Type_index' that create a new
  data type that reflects data that must be copied from an array of arbitrary
  size will have to allocate memory (and can fail; note that there is an
  MPI error class for out-of-memory).

  Question:
  Do we want to have an aligned allocation routine?  E.g., one that
  aligns memory on a cache-line.
  D*/

/* Define the string copy and duplication functions */
/* ---------------------------------------------------------------------- */
/* FIXME: Global types like this need to be discussed and agreed to */
typedef int MPIU_BOOL;

/* ------------------------------------------------------------------------- */

#ifdef USE_MEMORY_TRACING

/* Define these as invalid C to catch their use in the code */
#if 0
#define malloc(a)         'Error use MPL_malloc' :::
#define calloc(a,b)       'Error use MPL_calloc' :::
#define free(a)           'Error use MPL_free'   :::
#endif
#define realloc(a)        'Error use MPL_realloc' :::
#if defined(strdup) || defined(__strdup)
#undef strdup
#endif
    /* The ::: should cause the compiler to choke; the string 
       will give the explanation */
#undef strdup /* in case strdup is a macro */
#define strdup(a)         'Error use MPL_strdup' :::

#endif /* USE_MEMORY_TRACING */


/* Memory allocation macros. See document. */

/* Standard macro for generating error codes.  We set the error to be
 * recoverable by default, but this can be changed. */
#ifdef HAVE_ERROR_CHECKING
#define MPIU_CHKMEM_SETERR(rc_,nbytes_,name_) \
     rc_=MPIR_Err_create_code( MPI_SUCCESS, \
          MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, \
          MPI_ERR_OTHER, "**nomem2", "**nomem2 %d %s", nbytes_, name_ )
#else
#define MPIU_CHKMEM_SETERR(rc_,nbytes_,name_) rc_=MPI_ERR_OTHER
#endif

    /* CHKPMEM_REGISTER is used for memory allocated within another routine */

/* Memory used and freed within the current scopy (alloca if feasible) */
/* Configure with --enable-alloca to set USE_ALLOCA */
#if defined(HAVE_ALLOCA) && defined(USE_ALLOCA)
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
/* Define decl with a dummy definition to allow us to put a semi-colon
   after the macro without causing the declaration block to end (restriction
   imposed by C) */
#define MPIU_CHKLMEM_DECL(n_) int dummy_ ATTRIBUTE((unused))
#define MPIU_CHKLMEM_FREEALL()
#define MPIU_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)alloca(nbytes_); \
    if (!(pointer_) && (nbytes_ > 0)) {	   \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#else
#define MPIU_CHKLMEM_DECL(n_) \
 void *(mpiu_chklmem_stk_[n_]); \
 int mpiu_chklmem_stk_sp_=0;\
 MPIU_AssertDeclValue(const int mpiu_chklmem_stk_sz_,n_)

#define MPIU_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPL_malloc(nbytes_); \
if (pointer_) { \
    MPIU_Assert(mpiu_chklmem_stk_sp_<mpiu_chklmem_stk_sz_);\
    mpiu_chklmem_stk_[mpiu_chklmem_stk_sp_++] = pointer_;\
 } else if (nbytes_ > 0) {				 \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#define MPIU_CHKLMEM_FREEALL() \
    do { while (mpiu_chklmem_stk_sp_ > 0) {\
       MPL_free( mpiu_chklmem_stk_[--mpiu_chklmem_stk_sp_] ); } } while(0)
#endif /* HAVE_ALLOCA */
#define MPIU_CHKLMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKLMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* In some cases, we need to allocate large amounts of memory. This can
   be a problem if alloca is used, as the available stack space may be small.
   This is the same approach for the temporary memory as is used when alloca
   is not available. */
#define MPIU_CHKLBIGMEM_DECL(n_) \
 void *(mpiu_chklbigmem_stk_[n_]);\
 int mpiu_chklbigmem_stk_sp_=0;\
 MPIU_AssertDeclValue(const int mpiu_chklbigmem_stk_sz_,n_)

#define MPIU_CHKLBIGMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPL_malloc(nbytes_); \
if (pointer_) { \
    MPIU_Assert(mpiu_chklbigmem_stk_sp_<mpiu_chklbigmem_stk_sz_);\
    mpiu_chklbigmem_stk_[mpiu_chklbigmem_stk_sp_++] = pointer_;\
 } else if (nbytes_ > 0) {				       \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#define MPIU_CHKLBIGMEM_FREEALL() \
    { while (mpiu_chklbigmem_stk_sp_ > 0) {\
       MPL_free( mpiu_chklbigmem_stk_[--mpiu_chklbigmem_stk_sp_] ); } }

#define MPIU_CHKLBIGMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLBIGMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKLBIGMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKLBIGMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* Persistent memory that we may want to recover if something goes wrong */
#define MPIU_CHKPMEM_DECL(n_) \
 void *(mpiu_chkpmem_stk_[n_]) = { NULL };     \
 int mpiu_chkpmem_stk_sp_=0;\
 MPIU_AssertDeclValue(const int mpiu_chkpmem_stk_sz_,n_)
#define MPIU_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPL_malloc(nbytes_); \
if (pointer_) { \
    MPIU_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);\
    mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;\
 } else if (nbytes_ > 0) {				 \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}
#define MPIU_CHKPMEM_REGISTER(pointer_) \
    {MPIU_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);\
    mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;}
#define MPIU_CHKPMEM_REAP() \
    { while (mpiu_chkpmem_stk_sp_ > 0) {\
       MPL_free( mpiu_chkpmem_stk_[--mpiu_chkpmem_stk_sp_] ); } }
#define MPIU_CHKPMEM_COMMIT() \
    mpiu_chkpmem_stk_sp_ = 0
#define MPIU_CHKPMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKPMEM_MALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* now the CALLOC version for zeroed memory */
#define MPIU_CHKPMEM_CALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_CALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_)
#define MPIU_CHKPMEM_CALLOC_ORJUMP(pointer_,type_,nbytes_,rc_,name_) \
    MPIU_CHKPMEM_CALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)
#define MPIU_CHKPMEM_CALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_) \
    do {                                                                   \
        pointer_ = (type_)MPL_calloc(1, (nbytes_));                       \
        if (pointer_) {                                                    \
            MPIU_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_);        \
            mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_;          \
        }                                                                  \
        else if (nbytes_ > 0) {                                            \
            MPIU_CHKMEM_SETERR(rc_,nbytes_,name_);                         \
            stmt_;                                                         \
        }                                                                  \
    } while (0)

/* A special version for routines that only allocate one item */
#define MPIU_CHKPMEM_MALLOC1(pointer_,type_,nbytes_,rc_,name_,stmt_) \
{pointer_ = (type_)MPL_malloc(nbytes_); \
    if (!(pointer_) && (nbytes_ > 0)) {	   \
    MPIU_CHKMEM_SETERR(rc_,nbytes_,name_); \
    stmt_;\
}}

/* Provides a easy way to use realloc safely and avoid the temptation to use
 * realloc unsafely (direct ptr assignment).  Zero-size reallocs returning NULL
 * are handled and are not considered an error. */
#define MPIU_REALLOC_OR_FREE_AND_JUMP(ptr_,size_,rc_) do { \
    void *realloc_tmp_ = MPL_realloc((ptr_), (size_)); \
    if ((size_) && !realloc_tmp_) { \
        MPL_free(ptr_); \
        MPIR_ERR_SETANDJUMP2(rc_,MPI_ERR_OTHER,"**nomem2","**nomem2 %d %s",(size_),MPL_QUOTE(ptr_)); \
    } \
    (ptr_) = realloc_tmp_; \
} while (0)
/* this version does not free ptr_ */
#define MPIU_REALLOC_ORJUMP(ptr_,size_,rc_) do { \
    void *realloc_tmp_ = MPL_realloc((ptr_), (size_)); \
    if (size_) \
        MPIR_ERR_CHKANDJUMP2(!realloc_tmp_,rc_,MPI_ERR_OTHER,"**nomem2","**nomem2 %d %s",(size_),MPL_QUOTE(ptr_)); \
    (ptr_) = realloc_tmp_; \
} while (0)

#if defined(HAVE_STRNCASECMP)
#   define MPIU_Strncasecmp strncasecmp
#elif defined(HAVE_STRNICMP)
#   define MPIU_Strncasecmp strnicmp
#else
/* FIXME: Provide a fallback function ? */
#   error "No function defined for case-insensitive strncmp"
#endif

/* Evaluates to a boolean expression, true if the given byte ranges overlap,
 * false otherwise.  That is, true iff [a_,a_+a_len_) overlaps with [b_,b_+b_len_) */
#define MPIU_MEM_RANGES_OVERLAP(a_,a_len_,b_,b_len_) \
    ( ((char *)(a_) >= (char *)(b_) && ((char *)(a_) < ((char *)(b_) + (b_len_)))) ||  \
      ((char *)(b_) >= (char *)(a_) && ((char *)(b_) < ((char *)(a_) + (a_len_)))) )
#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))

/* May be used to perform sanity and range checking on memcpy and mempcy-like
   function calls.  This macro will bail out much like an MPIU_Assert if any of
   the checks fail. */
#define MPIU_MEM_CHECK_MEMCPY(dst_,src_,len_)                                                                   \
    do {                                                                                                        \
        if (len_) {                                                                                             \
            MPIU_Assert((dst_) != NULL);                                                                        \
            MPIU_Assert((src_) != NULL);                                                                        \
            MPL_VG_CHECK_MEM_IS_ADDRESSABLE((dst_),(len_));                                                     \
            MPL_VG_CHECK_MEM_IS_ADDRESSABLE((src_),(len_));                                                     \
            if (MPIU_MEM_RANGES_OVERLAP((dst_),(len_),(src_),(len_))) {                                          \
                MPIU_Assert_fmt_msg(FALSE,("memcpy argument memory ranges overlap, dst_=%p src_=%p len_=%ld\n", \
                                           (dst_), (src_), (long)(len_)));                                      \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)
#else
#define MPIU_MEM_CHECK_MEMCPY(dst_,src_,len_) do {} while(0)
#endif

/* valgrind macros are now provided by MPL (via mpl.h included in mpiimpl.h) */

/* ------------------------------------------------------------------------- */
/* end of mpimem.h */
/* ------------------------------------------------------------------------- */

#if defined(__cplusplus)
}
#endif
#endif
