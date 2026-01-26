#include "membuf.h"

int membuf_init(struct membuf *mb, void *b, size_t s)
{
	if (s > MEMBUF_MAX_SIZE) {
		err("size %zu exceeds maximum allowed %zu", MEMBUF_MAX_SIZE, s);
		return -1;
	}

	mb->buf = b;
	mb->size = s;
	membuf_clear_allocated(mb);

	return 0;
}

int membuf_grow(struct membuf *mb, size_t s)
{
	if (s <= membuf_size(mb))
		return 0;

	membuf_free(mb);

	return membuf_init_alloc(mb, s);
}

int membuf_empty(struct membuf *mb)
{
	return !membuf_buf(mb) || !membuf_size(mb);
}

int membuf_init_str(struct membuf *mb, char *s)
{
	return membuf_init(mb, s, strlen(s));
}

int membuf_init_alloc(struct membuf *mb, size_t s)
{
	void *b;

	if (s > MEMBUF_MAX_SIZE) {
		err("size %zu exceeds maximum allowed %zu", MEMBUF_MAX_SIZE, s);
		return -1;
	}

	b = malloc(s);
	if (!b) {
		err_errno("malloc failed for %zu bytes", s);
		return -1;
	}

	mb->buf = b;
	mb->size = s;
	membuf_set_allocated(mb);

	return 0;
}

void membuf_free(struct membuf *mb)
{
	if (membuf_is_allocated(mb))
		free(membuf_buf(mb));

	mb->buf = NULL;
	mb->size = 0;
	membuf_clear_allocated(mb);
}

int membuf_realloc(struct membuf *mb, size_t s)
{
	void *b;

	if (s <= membuf_size(mb))
		return 0;

	if (s > MEMBUF_MAX_SIZE) {
		err("cannot grow beyond maximum size %zu", MEMBUF_MAX_SIZE);
		return -1;
	}

	b = malloc(s);
	if (!b) {
		err("malloc failed for %zu bytes", s);
		return -1;
	}

	memcpy(b, membuf_buf(mb), membuf_size(mb)); // NOLINT

	membuf_free(mb);

	mb->buf = b;
	mb->size = s;
	membuf_set_allocated(mb);

	return 0;
}

void membuf_lower(struct membuf *mb)
{
	char *end = membuf_end(mb);
	char *c = membuf_buf(mb);

	while (c < end) {
		*c = tolower(*c);
		c++;
	}
}

void membuf_replace(struct membuf *mb, char from, char to)
{
	char *end = membuf_end(mb);
	char *c = membuf_buf(mb);

	while (c < end) {
		if (*c == from)
			*c = to;
		c++;
	}
}

ssize_t membuf_fread(struct membuf *mb, FILE *fp)
{
	ssize_t size;

	if (fseek(fp, 0, SEEK_END)) {
		err_errno("fseek");
		return -1;
	}

	size = ftell(fp);
	if (size == -1) {
		err_errno("ftell");
		return -1;
	}

	if (membuf_grow(mb, size))
		return -1;

	errno = 0;
	rewind(fp);
	if (errno) {
		err_errno("rewind");
		return -1;
	}

	if (fread(membuf_buf(mb), 1, size, fp) != size) {
		err("fread");
		return -1;
	}

	return size;
}

ssize_t membuf_fwrite(struct membuf *mb, FILE *fp)
{
	if (fwrite(membuf_buf(mb), 1, membuf_size(mb), fp) != membuf_size(mb)) {
		err_errno("fwrite");
		return -1;
	}
	return membuf_size(mb);
}

void *membuf_chr(struct membuf *mb, int c)
{
	return memchr(membuf_buf(mb), c, membuf_size(mb));
}

void *membuf_rchr(struct membuf *mb, int c)
{
	unsigned char *end = membuf_buf(mb);
	unsigned char *beg = membuf_end(mb);

	while (beg < end) {
		if (*--end == c)
			return end;
	}

	return NULL;
}

void *membuf_endswith(struct membuf *mb, char *s)
{
	size_t s_len = strlen(s);
	char *end = membuf_end(mb);

	if (membuf_size(mb) < s_len)
		return NULL;
	if (memcmp(end - s_len, s, s_len))
		return NULL;

	return end - s_len;
}

int membuf_cmp(struct membuf *mb1, struct membuf *mb2)
{
	int rv;

	rv = memcmp(membuf_buf(mb1), membuf_buf(mb2),
		    membuf_size(mb1) < membuf_size(mb2) ? membuf_size(mb1)
							: membuf_size(mb2));
	if (!rv && membuf_size(mb1) != membuf_size(mb2))
		return membuf_size(mb1) < membuf_size(mb2) ? -1 : 1;

	return rv;
}

ssize_t membuf_cat_mbs(struct membuf *dst, struct membuf **src, size_t src_cnt)
{

	ssize_t size = 0;
	char *c;

	for (int i = 0; i < src_cnt; i++)
		size += membuf_size(src[i]);

	if (membuf_grow(dst, size + 1))
		return -1;

	c = membuf_buf(dst);

	for (int i = 0; i < src_cnt; c += membuf_size(src[i++]))
		memcpy(c, membuf_buf(src[i]), membuf_size(src[i])); // NOLINT

	*c = '\0';

	return size;
}
