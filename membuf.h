#ifndef _MEMBUF_H_
#define _MEMBUF_H_

#include "utils.h"

struct membuf {
	void *buf;
	size_t size;
};

#define MEMBUF_ALLOC_MASK ((size_t)1 << (sizeof(size_t) * 8 - 1))
#define MEMBUF_MAX_SIZE ((size_t)-1 & ~MEMBUF_ALLOC_MASK)

#define INIT_MEMBUF(b, s) {.buf = (b), .size = (s) & ~MEMBUF_ALLOC_MASK}

#define DEFINE_MEMBUF(name, b, s) struct membuf name = INIT_MEMBUF((b), (s))
#define DEFINE_MEMBUF_EMPTY(name) struct membuf name = INIT_MEMBUF(NULL, 0)
#define DEFINE_MEMBUF_STR(name, str)                                           \
	struct membuf name = INIT_MEMBUF((str), strlen((str)))

#define membuf_buf(mb) ((mb)->buf)
#define membuf_size(mb) ((mb)->size & ~MEMBUF_ALLOC_MASK)
#define membuf_end(mb) ((void *)((char *)membuf_buf(mb) + membuf_size(mb)))

#define membuf_is_allocated(mb) (!!((mb)->size & MEMBUF_ALLOC_MASK))
#define membuf_set_allocated(mb) ((mb)->size |= MEMBUF_ALLOC_MASK)
#define membuf_clear_allocated(mb) ((mb)->size &= ~MEMBUF_ALLOC_MASK)

int membuf_init(struct membuf *mb, void *b, size_t s);
int membuf_grow(struct membuf *mb, size_t s);
int membuf_empty(struct membuf *mb);
int membuf_init_str(struct membuf *mb, char *s);
int membuf_init_alloc(struct membuf *mb, size_t s);
void membuf_free(struct membuf *mb);
int membuf_realloc(struct membuf *mb, size_t s);
int membuf_extend(struct membuf *mb, size_t s, int cp);
void membuf_lower(struct membuf *mb);
void membuf_replace(struct membuf *mb, char from, char to);
ssize_t membuf_fread(struct membuf *mb, FILE *fp);
ssize_t membuf_fwrite(struct membuf *mb, FILE *fp);
void *membuf_chr(struct membuf *mb, int c);
void *membuf_rchr(struct membuf *mb, int c);
void *membuf_endswith(struct membuf *mb, char *s);
int membuf_cmp(struct membuf *mb1, struct membuf *mb2);
void membuf_dump(struct membuf *mb);
ssize_t membuf_cat_mbs(struct membuf *dst, struct membuf **src, size_t src_cnt);

#define membuf_cat(dest, ...)                                                  \
	membuf_cat_mbs(dest, (struct membuf *[]){__VA_ARGS__},                 \
		       sizeof((struct membuf *[]){__VA_ARGS__}) /              \
			   sizeof(struct membuf *))

#endif // _MEMBUF_H_
