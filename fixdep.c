#include "membuf.h"
#include "utils.h"

struct depfile {
	struct membuf data;
	struct membuf target;
	struct membuf sdkconfig_dir;
	struct membuf deps;
	size_t deps_cnt;
	int sdkconfig;
	char *fn;
};

struct config {
	struct membuf data;
	struct membuf opts;
	size_t opts_cnt;
	char *fn;
};

int add_depfile_sdkconfig_dir(struct depfile *depfile, char *buf, size_t size)
{
	struct membuf dep;

	if (depfile->sdkconfig)
		return 1;

	if (membuf_init(&dep, buf, size))
		return 1;

	char *found = membuf_endswith(&dep, "/sdkconfig.h");
	if (!found) {
		DEFINE_MEMBUF_STR(basename, "sdkconfig.h");
		if (membuf_cmp(&dep, &basename))
			return 1;
		found = buf;
	}

	depfile->sdkconfig = 1;

	if (membuf_init(&depfile->sdkconfig_dir, buf, found - buf))
		return 1;

	return 0;
}

int add_depfile_dep(struct depfile *depfile, char *buf, size_t size)
{
	struct membuf *deps = membuf_buf(&depfile->deps);

	if (!size)
		return 0;

	if (!add_depfile_sdkconfig_dir(depfile, buf, size))
		return 0;

	if (membuf_init(&deps[depfile->deps_cnt++], buf, size))
		return -1;

	if (depfile->deps_cnt * sizeof(struct membuf) <
	    membuf_size(&depfile->deps))
		return 0;

	if (membuf_realloc(&depfile->deps, membuf_size(&depfile->deps) * 2))
		return -1;

	return 0;
}

void put_depfile(struct depfile *depfile)
{
	membuf_free(&depfile->deps);
	membuf_free(&depfile->data);
}

struct depfile *get_depfile(char *fn)
{
#define DEPS_CNT (512)
#define DATA_SIZE (1024 * 7)

	static struct membuf deps[DEPS_CNT];
	static char data[DATA_SIZE];
	ssize_t data_size;
	static struct depfile depfile;
	FILE *fp = NULL;

	depfile.fn = fn;

	if (membuf_init(&depfile.deps, deps, DEPS_CNT * sizeof(struct membuf)))
		goto err;

	if (membuf_init(&depfile.data, data, DATA_SIZE))
		goto err;

	fp = fopen(fn, "rb");
	if (!fp) {
		err_errno("cannot open '%s'", fn);
		goto err;
	}

	data_size = membuf_fread(&depfile.data, fp);
	if (data_size == -1) {
		err("cannot read '%s'", fn);
		goto err;
	}

	char *beg = membuf_buf(&depfile.data);
	char *end = beg + data_size;

	/* target */
	char *c = membuf_chr(&depfile.data, ':');
	if (!c) {
		err("cannot find target in dep file '%s'", fn);
		goto err;
	}

	if (membuf_init(&depfile.target, beg, c - beg))
		goto err;

	char *dep = ++c;
	char last = 0;

	/* target dependencies */
	while (c < end) {
		if ((*c == '\n' || *c == '\r') && last == '\\') {
			dep = c + 1;

		} else if ((*c == ' ' || *c == '\n' || *c == '\r') &&
			   last != '\\') {
			if (add_depfile_dep(&depfile, dep, c - dep))
				goto err;
			dep = c + 1;
		}

		last = *c++;
	}

	if (add_depfile_dep(&depfile, dep, c - dep))
		goto err;

	if (fp)
		fclose(fp);

	return &depfile;
err:
	if (fp)
		fclose(fp);

	put_depfile(&depfile);

	return NULL;

#undef DEPS_CNT
#undef DATA_SIZE
}

int add_config_opt(struct config *config, char *opt, size_t opt_size)
{
	struct membuf *opts = membuf_buf(&config->opts);
	struct membuf opt_mb;

	if (!opt_size)
		return 0;

	if (membuf_init(&opt_mb, opt, opt_size))
		return -1;

	for (int i = 0; i < config->opts_cnt; i++) {
		if (!membuf_cmp(&opts[i], &opt_mb))
			return 0;
	}

	membuf_init(&opts[config->opts_cnt++], opt, opt_size);

	if (config->opts_cnt * sizeof(struct membuf) <
	    membuf_size(&config->opts))
		return 0;

	if (membuf_realloc(&config->opts, membuf_size(&config->opts) * 2))
		return -1;

	return 0;
}

void put_config(struct config *config)
{
	membuf_free(&config->opts);
	membuf_free(&config->data);
}

struct config *get_config(char *fn)
{
#define OPTS_CNT (256)
#define DATA_SIZE (1024 * 10 * 3)

	static struct membuf opts[OPTS_CNT];
	static int data[DATA_SIZE];
	ssize_t data_size;
	static struct config config;
	FILE *fp = NULL;

	config.fn = fn;

	if (membuf_init(&config.opts, opts, OPTS_CNT * sizeof(struct membuf)))
		goto err;

	if (membuf_init(&config.data, data, DATA_SIZE))
		goto err;

	fp = fopen(fn, "rb");
	if (!fp) {
		err_errno("cannot open '%s'", fn);
		goto err;
	}

	data_size = membuf_fread(&config.data, fp);
	if (data_size == -1) {
		err("cannot read '%s'", fn);
		goto err;
	}

	char *c = membuf_buf(&config.data);
	char *end = c + data_size;

	DEFINE_MEMBUF_STR(prefix, "CONFIG_");

	while (c < end) {
		int *n = membuf_buf(&prefix);
		c = memchr(c, *n, end - c);
		if (!c)
			break;

		if (end - c < membuf_size(&prefix))
			break;

		if (memcmp(c, membuf_buf(&prefix), membuf_size(&prefix))) {
			c++;
			continue;
		}

		c += membuf_size(&prefix);
		char *opt = c;
		while (c < end && (isupper(*c) || isdigit(*c) || *c == '_'))
			c++;

		if (add_config_opt(&config, opt, c - opt))
			goto err;
	}

	if (fp)
		fclose(fp);

	return &config;

err:
	if (fp)
		fclose(fp);

	put_config(&config);

	return NULL;

#undef OPTS_CNT
#undef DATA_SIZE
}

char *get_dep_fn(int argc, char *argv[])
{
	do {
		if (!strcmp(*argv, "-MF"))
			return *++argv;

	} while (*++argv);

	return NULL;
}

char *get_src_fn(int argc, char *argv[]) { return argv[argc - 1]; }

int fix_dep_file(struct depfile *depfile, struct config *config)
{
#define DEP_FN_SIZE (1024)

	struct membuf *deps = membuf_buf(&depfile->deps);
	struct membuf *opts = membuf_buf(&config->opts);

	static char dep_fn[DEP_FN_SIZE];
	int rv = -1;

	DEFINE_MEMBUF(dep_buf, dep_fn, DEP_FN_SIZE);

	DEFINE_MEMBUF_STR(sep, " \\" EOL " ");
	DEFINE_MEMBUF_STR(semi, ":");
	DEFINE_MEMBUF_STR(ext, ".h");
	DEFINE_MEMBUF_STR(slash, "/");
	DEFINE_MEMBUF_STR(eol, EOL);

	FILE *fp = fopen(depfile->fn, "wb");
	if (!fp) {
		err_errno("open '%s'", depfile->fn);
		goto err;
	}

	if (membuf_fwrite(&depfile->target, fp) == -1)
		goto err;

	if (membuf_fwrite(&semi, fp) == -1)
		goto err;

	for (int i = 0; i < depfile->deps_cnt; i++) {
		if (membuf_fwrite(&sep, fp) == -1)
			goto err;
		if (membuf_fwrite(&deps[i], fp) == -1)
			goto err;
	}

	for (int i = 0; i < config->opts_cnt; i++) {
		ssize_t dep_size;

		membuf_lower(&opts[i]);
		membuf_replace(&opts[i], '_', '/');

		if (membuf_empty(&depfile->sdkconfig_dir)) {
			dep_size = membuf_cat(&dep_buf, &opts[i], &ext);
			if (dep_size == -1)
				goto err;
		} else {
			dep_size = membuf_cat(&dep_buf, &depfile->sdkconfig_dir,
					      &slash, &opts[i], &ext);
			if (dep_size == -1)
				goto err;
		}

		DEFINE_MEMBUF(dep, membuf_buf(&dep_buf), dep_size);

		if (access(membuf_buf(&dep), F_OK))
			continue;

		if (membuf_fwrite(&sep, fp) == -1)
			goto err;

		if (membuf_fwrite(&dep, fp) == -1)
			goto err;
	}

	if (membuf_fwrite(&eol, fp) == -1)
		goto err;

	rv = 0;
err:
	membuf_free(&dep_buf);
	if (fp)
		fclose(fp);
	return rv;

#undef DEP_FN_SIZE
}

void dump_depfile(struct depfile *d)
{
	char *b;
	int s;

	printf("depfile: %s\n", d->fn);
	b = membuf_buf(&d->target);
	s = membuf_size(&d->target);
	printf("target: '%.*s' (%u)\n", s, b, s);

	printf("sdkconfig: '%d'\n", d->sdkconfig);

	b = membuf_buf(&d->sdkconfig_dir);
	s = membuf_size(&d->sdkconfig_dir);
	printf("sdkconfig_dir: '%.*s' (%u)\n", s, b, s);

	struct membuf *deps = membuf_buf(&d->deps);
	for (int i = 0; i < d->deps_cnt; i++) {
		b = membuf_buf(&deps[i]);
		s = membuf_size(&deps[i]);
		printf("dep: '%.*s' (%u)\n", s, b, s);
	}
}

void dump_config(struct config *c)
{
	char *b;
	int s;

	printf("source: %s\n", c->fn);

	struct membuf *opts = membuf_buf(&c->opts);
	for (int i = 0; i < c->opts_cnt; i++) {
		b = membuf_buf(&opts[i]);
		s = membuf_size(&opts[i]);
		printf("opt: '%.*s' (%u)\n", s, b, s);
	}
}

int main(int argc, char **argv)
{
	struct depfile *depfile = NULL;
	struct config *config = NULL;
	char *dep_fn;
	int rv;
	int exit_code = EXIT_FAILURE;

	if (argc < 2) {
		fprintf(stderr, "version: %s\n", VERSION);
		fprintf(stderr, "usage: esp-idf-configdep <cmd> <arg...>\n");
		exit_code = EXIT_SUCCESS;
		goto done;
	}

	rv = exec_process(argv);
	if (rv) {
		err("exec_process exited with error code: %d", rv);
		goto done;
	}

	dep_fn = get_dep_fn(argc, argv);
	if (!dep_fn) {
		exit_code = EXIT_SUCCESS;
		goto done;
	}

	depfile = get_depfile(dep_fn);
	if (!depfile)
		goto done;

	// dump_depfile(depfile);

	if (!depfile->sdkconfig) {
		exit_code = EXIT_SUCCESS;
		goto done;
	}

	config = get_config(get_src_fn(argc, argv));
	if (!config)
		goto done;

	// dump_config(config);

	if (fix_dep_file(depfile, config))
		goto done;

	exit_code = EXIT_SUCCESS;
done:
	if (depfile)
		put_depfile(depfile);
	if (config)
		put_config(config);

	return exit_code;
}
