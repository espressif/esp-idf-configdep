/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file configdep.c
 * @brief Main module for esp-idf-configdep: sdkconfig.h dependency optimizer.
 *
 * This tool acts as a compiler wrapper. It:
 *  1. Executes the real compiler command (forwarding all arguments).
 *  2. Locates the dependency file produced by the compiler (-MF flag).
 *  3. Parses the dependency file; if sdkconfig.h is listed as a dependency
 *     it is removed.
 *  4. Scans every prerequisite listed in the .d file (source, headers,
 *     .inc files, etc.) for CONFIG_* references. The source file is
 *     included because the compiler lists it in the .d file — no separate
 *     extraction from argv is needed.
 *  5. Rewrites the dependency file, replacing the single sdkconfig.h
 *     dependency with granular per-option dummy files (e.g.
 *     <sdkconfig_dir>/my/option.cdep for CONFIG_MY_OPTION). Any missing
 *     .cdep is created as an empty stub (parent directories as needed) so
 *     the path always exists for Ninja; kconfgen sync_deps updates
 *     contents when values change.
 *
 * This ensures that a source file is only rebuilt when the specific
 * configuration options it references change, rather than on every
 * sdkconfig.h modification.
 */
#include "membuf.h"
#include "utils.h"

#include <ctype.h>

/**
 * @brief Parsed representation of a compiler-generated dependency (.d) file.
 *
 * A dependency file has the form:
 *   target: dep1 dep2 ... depN
 *
 * This structure stores the target, the list of dependencies, whether
 * sdkconfig.h was among them, and (if so) the directory prefix leading
 * to sdkconfig.h so that per-option .cdep files can be located relative to it.
 */
struct depfile {
	struct membuf data;	     /**< Raw file content buffer. */
	struct membuf target;	     /**< The make target (left of ':'). */
	struct membuf sdkconfig_dir; /**< Directory prefix of sdkconfig.h. */
	struct membuf deps;	     /**< Array of membuf dependency entries. */
	size_t deps_cnt;	     /**< Number of entries in deps. */
	int sdkconfig;		     /**< Non-zero if sdkconfig.h was found. */
	char *fn;		     /**< Dependency file path. */
};

/**
 * @brief CONFIG_* options extracted from a source file.
 *
 * Stores unique CONFIG_XYZ option names (without the "CONFIG_" prefix)
 * found by scanning the source file contents.
 */
struct config {
	struct membuf data; /**< File content buffer (reused across scans). */
	struct membuf opts; /**< Array of membuf option-name entries. */
	size_t opts_cnt;    /**< Number of unique options found. */
};

/**
 * @brief Check if a dependency path is sdkconfig.h and record its directory.
 *
 * If the dependency ends with "/sdkconfig.h" (or is exactly "sdkconfig.h"),
 * sets depfile->sdkconfig = 1 and stores the directory prefix in
 * depfile->sdkconfig_dir. This information is used later to locate
 * per-option dummy files (.cdep) in the same directory.
 *
 * @param depfile  Dependency file context.
 * @param buf      Pointer to the dependency path string.
 * @param size     Length of the dependency path.
 * @return 0 if this was sdkconfig.h (consumed), 1 otherwise (not sdkconfig).
 */
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

/**
 * @brief Add a dependency entry to the depfile, filtering out sdkconfig.h.
 *
 * Empty entries are silently skipped. If the entry is sdkconfig.h it is
 * consumed by add_depfile_sdkconfig_dir() and not added to the deps list.
 * The deps array is dynamically doubled when it runs out of space.
 *
 * @param depfile  Dependency file context.
 * @param buf      Pointer to the dependency path string.
 * @param size     Length of the dependency path.
 * @return 0 on success, -1 on allocation error.
 */
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

/** Release resources held by a parsed depfile. */
void put_depfile(struct depfile *depfile)
{
	membuf_free(&depfile->deps);
	membuf_free(&depfile->data);
}

/**
 * @brief Parse a compiler-generated dependency file.
 *
 * Reads the file, extracts the make target (everything before ':'), and
 * splits the remaining content into individual dependency paths on
 * whitespace boundaries, honouring backslash-newline line continuations.
 * If sdkconfig.h is among the dependencies it is recorded but excluded
 * from the deps list.
 *
 * @param fn  Path to the dependency (.d) file.
 * @return Pointer to a static depfile structure, or NULL on error.
 */
struct depfile *get_depfile(char *fn)
{
#define DEPS_CNT (512)
#define DATA_SIZE ((size_t)1024 * 7)

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

/**
 * @brief Add a unique CONFIG option name to the config's option list.
 *
 * Duplicates are detected by linear scan and silently skipped. The opts
 * array is dynamically doubled when it runs out of space.
 *
 * @param config    Config context.
 * @param opt       Pointer to the option name (after the "CONFIG_" prefix).
 * @param opt_size  Length of the option name.
 * @return 0 on success, -1 on allocation error.
 */
int add_config_opt(struct config *config, char *opt, size_t opt_size)
{
	struct membuf *opts = membuf_buf(&config->opts);
	struct membuf opt_key;

	if (!opt_size)
		return 0;

	if (membuf_init(&opt_key, opt, opt_size))
		return -1;

	for (int i = 0; i < config->opts_cnt; i++) {
		if (!membuf_cmp(&opts[i], &opt_key))
			return 0;
	}

	if (membuf_init_alloc(&opts[config->opts_cnt], opt_size))
		return -1;

	memcpy(membuf_buf(&opts[config->opts_cnt]), opt, opt_size);
	config->opts_cnt++;

	if (config->opts_cnt * sizeof(struct membuf) <
	    membuf_size(&config->opts))
		return 0;

	if (membuf_realloc(&config->opts, membuf_size(&config->opts) * 2))
		return -1;

	return 0;
}

/** Release resources held by a parsed config. */
void put_config(struct config *config)
{
	struct membuf *opts = membuf_buf(&config->opts);

	for (int i = 0; i < config->opts_cnt; i++)
		membuf_free(&opts[i]);
	config->opts_cnt = 0;

	membuf_free(&config->opts);
	membuf_free(&config->data);
}

/**
 * @brief Read one dependency file and scan it for CONFIG_* options.
 *
 * Opens the file, reads it into config->data (reusing the buffer across
 * calls to avoid repeated allocations), and extracts CONFIG_* references.
 *
 * @return 0 on success (including file-not-found), -1 on real error.
 */
static int config_scan_file(struct config *config, struct membuf *dep)
{
	if (membuf_empty(dep))
		return 0;

	/* membuf_cat builds a NUL-terminated path in config->data;
	 * membuf_fread below overwrites it with file content after
	 * fopen has already consumed the path string. */
	if (membuf_cat(&config->data, dep) == -1)
		return -1;

	FILE *fp = fopen(membuf_buf(&config->data), "rb");
	if (!fp) {
		if (errno == ENOENT)
			return 0;
		err_errno("cannot open '%s'",
			  (char *)membuf_buf(&config->data));
		return -1;
	}

	ssize_t size = membuf_fread(&config->data, fp);
	fclose(fp);

	if (size == -1)
		return -1;

	char *c = membuf_buf(&config->data);
	char *end = c + size;

	DEFINE_MEMBUF_STR(prefix, "CONFIG_");

	while (c < end) {
		char *n = membuf_buf(&prefix);
		c = memchr(c, *n, (size_t)(end - c));
		if (!c)
			break;

		if ((size_t)(end - c) < membuf_size(&prefix))
			break;

		if (memcmp(c, membuf_buf(&prefix), membuf_size(&prefix)) != 0) {
			c++;
			continue;
		}

		c += membuf_size(&prefix);
		char *opt = c;
		while (c < end && (isupper((unsigned char)*c) ||
				   isdigit((unsigned char)*c) || *c == '_'))
			c++;

		if (add_config_opt(config, opt, (size_t)(c - opt)))
			return -1;
	}

	return 0;
}

/**
 * @brief Scan all dependencies from the depfile for CONFIG_* references.
 *
 * Iterates every dependency path from the parsed .d file and scans each
 * one.  The source file is included because the compiler lists it in the
 * .d file.  config->data is reused across calls so that once a large file
 * grows the buffer, subsequent smaller files avoid re-allocation.
 */
struct config *get_config(struct depfile *depfile)
{
#define OPTS_CNT (256)
#define DATA_SIZE ((size_t)1024 * 10 * 3)

	static struct membuf opts[OPTS_CNT];
	static char data[DATA_SIZE];
	static struct config config;

	put_config(&config);

	if (membuf_init(&config.opts, opts, OPTS_CNT * sizeof(struct membuf)))
		return NULL;

	if (membuf_init(&config.data, data, DATA_SIZE))
		return NULL;

	struct membuf *deps = membuf_buf(&depfile->deps);

	for (size_t i = 0; i < depfile->deps_cnt; i++) {
		if (config_scan_file(&config, &deps[i]))
			goto err;
	}

	return &config;
err:
	put_config(&config);
	return NULL;

#undef OPTS_CNT
#undef DATA_SIZE
}

/**
 * @brief Extract the dependency file path from the compiler arguments.
 *
 * Searches for the "-MF" flag and returns the immediately following argument,
 * which is the path to the generated dependency file.
 *
 * @param argc  Argument count.
 * @param argv  Argument vector.
 * @return Path to the dependency file, or NULL if -MF is not present.
 */
char *get_dep_fn(int argc, char *argv[])
{
	do {
		if (!strcmp(*argv, "-MF"))
			return *++argv;

	} while (*++argv);

	return NULL;
}

/**
 * @brief Rewrite the dependency file with granular per-option dependencies.
 *
 * Overwrites the original dependency file with:
 *  1. The original make target and colon.
 *  2. All original dependencies *except* sdkconfig.h.
 *  3. For each CONFIG_XYZ option found in the source, the corresponding
 *     dummy file (e.g. <sdkconfig_dir>/my/option.cdep for
 *     CONFIG_MY_OPTION). Missing files are created empty (parent dirs as
 *     needed) so they can always be listed as dependencies.
 *
 * The option name is lowercased and underscores are replaced with '/'
 * to derive the file path.
 *
 * @param depfile  Parsed dependency file (sdkconfig.h already filtered out).
 * @param config   CONFIG_* options extracted from the source file.
 * @return 0 on success, -1 on error.
 */
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
	DEFINE_MEMBUF_STR(ext, ".cdep");
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
		membuf_replace(&opts[i],
			       (membuf_byte_pair){.from = '_', .to = '/'});

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

		if (access(membuf_buf(&dep), F_OK) &&
		    touch_file(membuf_buf(&dep)))
			goto err;

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

/** Debug helper: dump depfile contents to stdout. */
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

/** Debug helper: dump config options to stdout. */
void dump_config(struct config *c)
{
	char *b;
	int s;

	struct membuf *opts = membuf_buf(&c->opts);
	for (int i = 0; i < c->opts_cnt; i++) {
		b = membuf_buf(&opts[i]);
		s = membuf_size(&opts[i]);
		printf("opt: '%.*s' (%u)\n", s, b, s);
	}
}

/**
 * @brief Entry point for esp-idf-configdep.
 *
 * Workflow:
 *  1. Handle --version / -v flag.
 *  2. Execute the real compiler command via exec_process().
 *  3. Look for "-MF <file>" in the arguments to find the dependency file.
 *     If absent, the compiler was not asked to generate dependencies -- exit
 *     successfully (nothing to optimise).
 *  4. Parse the dependency file. If sdkconfig.h is not among the
 *     dependencies, exit successfully (no optimisation needed).
 *  5. Scan all .d prerequisites for CONFIG_* references.
 *  6. Rewrite the dependency file with per-option granular dependencies.
 *
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on any error.
 */
int main(int argc, char **argv)
{
	struct depfile *depfile = NULL;
	struct config *config = NULL;
	char *dep_fn;
	int rv;
	int exit_code = EXIT_FAILURE;

	if (argc == 2 &&
	    (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))) {
		fprintf(stderr, "version: %s\n", VERSION);
		return EXIT_SUCCESS;
	}

	if (argc < 2) {
		fprintf(stderr, "usage: esp-idf-configdep <cmd> <arg...>\n");
		goto done;
	}

	/* Step 1: Run the actual compiler command. */
	rv = exec_process(argv);
	if (rv) {
		err("exec_process exited with error code: %d", rv);
		goto done;
	}

	/* Step 2: Find the dependency file path (-MF argument). */
	dep_fn = get_dep_fn(argc, argv);
	if (!dep_fn) {
		/* No -MF flag; nothing to optimise. */
		exit_code = EXIT_SUCCESS;
		goto done;
	}

	/* Step 3: Parse the dependency file. */
	depfile = get_depfile(dep_fn);
	if (!depfile)
		goto done;

	// dump_depfile(depfile);

	if (!depfile->sdkconfig) {
		/* sdkconfig.h not in dependencies; nothing to optimise. */
		exit_code = EXIT_SUCCESS;
		goto done;
	}

	/* Step 4: Scan all deps from the .d file for CONFIG_* references. */
	config = get_config(depfile);
	if (!config)
		goto done;

	// dump_config(config);

	/* Step 5: Rewrite the dep file with granular dependencies. */
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
