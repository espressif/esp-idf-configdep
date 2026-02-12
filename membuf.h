/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file membuf.h
 * @brief Lightweight memory buffer abstraction for non-NUL-terminated data.
 *
 * membuf provides a thin wrapper around a raw pointer and a size, supporting
 * both statically allocated (stack/global) and dynamically allocated (heap)
 * buffers. The highest bit of the size field is used as a flag to track
 * whether the buffer was dynamically allocated, so that membuf_free() only
 * calls free() when appropriate.
 *
 * Typical usage patterns:
 *  - DEFINE_MEMBUF / DEFINE_MEMBUF_STR to create stack-backed buffers.
 *  - membuf_init_alloc / membuf_grow / membuf_realloc for heap-backed
 *    buffers that can expand as needed.
 *  - membuf_fread / membuf_fwrite for file I/O.
 *  - membuf_cat for concatenating multiple buffers into a destination.
 */
#ifndef _MEMBUF_H_
#define _MEMBUF_H_

#include "utils.h"

/**
 * @brief Memory buffer descriptor.
 *
 * Wraps a raw pointer and its byte length. The highest bit of @c size is
 * reserved as an allocation flag (see MEMBUF_ALLOC_MASK); the effective
 * size is obtained by masking it out.
 */
struct membuf {
	void *buf;   /**< Pointer to the buffer data. */
	size_t size; /**< Byte length with allocation flag in the MSB. */
};

/** Bitmask for the allocation flag stored in the MSB of membuf::size. */
#define MEMBUF_ALLOC_MASK ((size_t)1 << (sizeof(size_t) * 8 - 1))

/** Maximum representable buffer size (all bits except the allocation flag). */
#define MEMBUF_MAX_SIZE ((size_t)-1 & ~MEMBUF_ALLOC_MASK)

/** Static initializer for a membuf backed by existing storage @p b of @p s
 * bytes. */
#define INIT_MEMBUF(b, s) {.buf = (b), .size = (s) & ~MEMBUF_ALLOC_MASK}

/** Define and initialize a membuf variable backed by buffer @p b of @p s bytes.
 */
#define DEFINE_MEMBUF(name, b, s) struct membuf name = INIT_MEMBUF((b), (s))

/** Define an empty (NULL, 0) membuf variable. */
#define DEFINE_MEMBUF_EMPTY(name) struct membuf name = INIT_MEMBUF(NULL, 0)

/** Define a membuf variable wrapping a C string literal @p str (excluding NUL).
 */
#define DEFINE_MEMBUF_STR(name, str)                                           \
	struct membuf name = INIT_MEMBUF((str), strlen((str)))

/** Return the raw data pointer of @p mb. */
#define membuf_buf(mb) ((mb)->buf)

/** Return the effective byte size of @p mb (allocation flag masked out). */
#define membuf_size(mb) ((mb)->size & ~MEMBUF_ALLOC_MASK)

/** Return a pointer past the last byte of @p mb. */
#define membuf_end(mb) ((void *)((char *)membuf_buf(mb) + membuf_size(mb)))

/** Test whether @p mb was dynamically allocated. */
#define membuf_is_allocated(mb) (!!((mb)->size & MEMBUF_ALLOC_MASK))

/** Mark @p mb as dynamically allocated. */
#define membuf_set_allocated(mb) ((mb)->size |= MEMBUF_ALLOC_MASK)

/** Clear the dynamically-allocated flag on @p mb. */
#define membuf_clear_allocated(mb) ((mb)->size &= ~MEMBUF_ALLOC_MASK)

/**
 * @brief Initialize a membuf with an existing buffer.
 *
 * The buffer is not copied; @p mb simply wraps the provided pointer.
 * The allocation flag is cleared (the caller owns the storage).
 *
 * @param mb  Buffer descriptor to initialize.
 * @param b   Pointer to existing storage.
 * @param s   Size of the storage in bytes.
 * @return 0 on success, -1 if @p s exceeds MEMBUF_MAX_SIZE.
 */
int membuf_init(struct membuf *mb, void *b, size_t s);

/**
 * @brief Grow the buffer to at least @p s bytes, allocating if needed.
 *
 * If the current size is already >= @p s, this is a no-op. Otherwise the
 * existing buffer is freed and a new heap buffer of @p s bytes is allocated.
 *
 * @param mb  Buffer descriptor.
 * @param s   Minimum required size in bytes.
 * @return 0 on success, -1 on allocation failure.
 */
int membuf_grow(struct membuf *mb, size_t s);

/**
 * @brief Check whether a membuf is empty (NULL pointer or zero size).
 *
 * @param mb  Buffer descriptor.
 * @return Non-zero if the buffer is empty, 0 otherwise.
 */
int membuf_empty(struct membuf *mb);

/**
 * @brief Initialize a membuf from a NUL-terminated C string.
 *
 * Equivalent to membuf_init(mb, s, strlen(s)).
 *
 * @param mb  Buffer descriptor to initialize.
 * @param s   NUL-terminated string (not copied).
 * @return 0 on success, -1 on error.
 */
int membuf_init_str(struct membuf *mb, char *s);

/**
 * @brief Allocate a heap buffer of @p s bytes and initialize the membuf.
 *
 * @param mb  Buffer descriptor to initialize.
 * @param s   Size in bytes to allocate.
 * @return 0 on success, -1 on allocation failure or size overflow.
 */
int membuf_init_alloc(struct membuf *mb, size_t s);

/**
 * @brief Free the buffer if it was dynamically allocated, then reset.
 *
 * After this call the membuf is empty (NULL, 0). Safe to call on
 * statically-backed buffers (no-op for the free).
 *
 * @param mb  Buffer descriptor.
 */
void membuf_free(struct membuf *mb);

/**
 * @brief Reallocate the buffer to @p s bytes, preserving existing content.
 *
 * If the current size is already >= @p s this is a no-op. A new buffer is
 * allocated, existing data is copied, and the old buffer is freed.
 *
 * @param mb  Buffer descriptor.
 * @param s   New size in bytes (must be > current size to take effect).
 * @return 0 on success, -1 on allocation failure or size overflow.
 */
int membuf_realloc(struct membuf *mb, size_t s);

/**
 * @brief Extend the buffer by @p s bytes, optionally preserving content.
 *
 * @param mb  Buffer descriptor.
 * @param s   Number of additional bytes.
 * @param cp  If non-zero, copy existing data into the new buffer.
 * @return 0 on success, -1 on error.
 */
int membuf_extend(struct membuf *mb, size_t s, int cp);

/**
 * @brief Convert all ASCII characters in the buffer to lowercase in-place.
 *
 * @param mb  Buffer descriptor.
 */
void membuf_lower(struct membuf *mb);

/**
 * @brief Replace every occurrence of byte @p from with @p to in-place.
 *
 * @param mb    Buffer descriptor.
 * @param from  Byte value to search for.
 * @param to    Replacement byte value.
 */
void membuf_replace(struct membuf *mb, char from, char to);

/**
 * @brief Read an entire file into the membuf, growing it if necessary.
 *
 * The file position is determined via fseek/ftell, rewound, then read.
 *
 * @param mb  Buffer descriptor (grown to file size if needed).
 * @param fp  Open file pointer (must support seeking).
 * @return Number of bytes read on success, -1 on error.
 */
ssize_t membuf_fread(struct membuf *mb, FILE *fp);

/**
 * @brief Write the entire membuf contents to a file.
 *
 * @param mb  Buffer descriptor.
 * @param fp  Open file pointer for writing.
 * @return Number of bytes written on success, -1 on error.
 */
ssize_t membuf_fwrite(struct membuf *mb, FILE *fp);

/**
 * @brief Search for the first occurrence of byte @p c in the buffer.
 *
 * @param mb  Buffer descriptor.
 * @param c   Byte value to search for.
 * @return Pointer to the found byte, or NULL if not found.
 */
void *membuf_chr(struct membuf *mb, int c);

/**
 * @brief Search for the last occurrence of byte @p c in the buffer.
 *
 * @param mb  Buffer descriptor.
 * @param c   Byte value to search for.
 * @return Pointer to the found byte, or NULL if not found.
 */
void *membuf_rchr(struct membuf *mb, int c);

/**
 * @brief Check whether the buffer ends with the C string @p s.
 *
 * @param mb  Buffer descriptor.
 * @param s   NUL-terminated suffix to check for.
 * @return Pointer to the start of the suffix inside the buffer, or NULL.
 */
void *membuf_endswith(struct membuf *mb, char *s);

/**
 * @brief Compare two membufs lexicographically (memcmp semantics).
 *
 * @param mb1  First buffer.
 * @param mb2  Second buffer.
 * @return 0 if equal, negative if mb1 < mb2, positive if mb1 > mb2.
 */
int membuf_cmp(struct membuf *mb1, struct membuf *mb2);

/**
 * @brief Dump the membuf contents to stdout for debugging.
 *
 * @param mb  Buffer descriptor.
 */
void membuf_dump(struct membuf *mb);

/**
 * @brief Concatenate multiple membufs into @p dst.
 *
 * The destination is grown to hold all source data plus a NUL terminator.
 * After concatenation the result is NUL-terminated for convenience.
 *
 * @param dst      Destination buffer (grown as needed).
 * @param src      Array of pointers to source buffers.
 * @param src_cnt  Number of source buffers.
 * @return Total bytes written (excluding NUL), or -1 on error.
 */
ssize_t membuf_cat_mbs(struct membuf *dst, struct membuf **src, size_t src_cnt);

/**
 * @brief Convenience macro to concatenate a variadic list of membuf pointers.
 *
 * Usage: membuf_cat(&dst, &a, &b, &c)
 */
#define membuf_cat(dest, ...)                                                  \
	membuf_cat_mbs(dest, (struct membuf *[]){__VA_ARGS__},                 \
		       sizeof((struct membuf *[]){__VA_ARGS__}) /              \
			   sizeof(struct membuf *))

#endif // _MEMBUF_H_
