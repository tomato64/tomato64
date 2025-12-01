/*
 * rstats_migrate.c - Migrate Tomato64 rstats history from old format to new format
 *
 * This tool converts rstats history files from:
 *   OLD: MAX_NMONTHLY=25 (2 years of monthly history)
 *   NEW: MAX_NMONTHLY=121 (10 years of monthly history)
 *
 * Compile: gcc -o rstats_migrate rstats_migrate.c -lz
 * Usage:   ./rstats_migrate <old_file_or_dir> <new_file_or_dir>
 *
 * IMPORTANT: Run this BEFORE upgrading to the new firmware!
 *            This tool reads old format and writes new format.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <zlib.h>

/* Constants from rstats.c */
#define MAX_NDAILY      62
#define MAX_COUNTER     2
#define CURRENT_ID      0x31305352

/* Old format constants (pre-commit) */
#define OLD_MAX_NMONTHLY    25

/* New format constants (post-commit) */
#define NEW_MAX_NMONTHLY    121

/* Structure definitions */
typedef struct {
	uint32_t xtime;
	uint64_t counter[MAX_COUNTER];
} data_t;

/* Old format history structure */
typedef struct {
	uint32_t id;
	data_t daily[MAX_NDAILY];
	int dailyp;
	data_t monthly[OLD_MAX_NMONTHLY];
	int monthlyp;
} history_old_t;

/* New format history structure */
typedef struct {
	uint32_t id;
	data_t daily[MAX_NDAILY];
	int dailyp;
	data_t monthly[NEW_MAX_NMONTHLY];
	int monthlyp;
} history_new_t;

/* Function to decompress a gzip file */
static int decompress_file(const char *filename, void *buffer, size_t max_size)
{
	gzFile gz;
	int n;

	gz = gzopen(filename, "rb");
	if (!gz) {
		fprintf(stderr, "Error: Cannot open %s for reading\n", filename);
		return -1;
	}

	n = gzread(gz, buffer, max_size);
	gzclose(gz);

	if (n < 0) {
		fprintf(stderr, "Error: Failed to decompress %s\n", filename);
		return -1;
	}

	return n;
}

/* Function to compress and write a gzip file */
static int compress_file(const char *filename, const void *buffer, size_t size)
{
	gzFile gz;
	int n;

	gz = gzopen(filename, "wb");
	if (!gz) {
		fprintf(stderr, "Error: Cannot open %s for writing\n", filename);
		return -1;
	}

	n = gzwrite(gz, buffer, size);
	gzclose(gz);

	if (n != size) {
		fprintf(stderr, "Error: Failed to compress %s (wrote %d of %zu bytes)\n",
			filename, n, size);
		return -1;
	}

	return 0;
}

/* Migrate history file (monthly/daily bandwidth data) */
static int migrate_history(const char *old_file, const char *new_file)
{
	history_old_t old_hist;
	history_new_t new_hist;
	int bytes_read;

	printf("Migrating bandwidth history...\n");
	printf("  Old format: %d monthly entries (MAX_NMONTHLY=%d)\n",
		OLD_MAX_NMONTHLY, OLD_MAX_NMONTHLY);
	printf("  New format: %d monthly entries (MAX_NMONTHLY=%d)\n",
		NEW_MAX_NMONTHLY, NEW_MAX_NMONTHLY);

	/* Read old format */
	bytes_read = decompress_file(old_file, &old_hist, sizeof(old_hist));
	if (bytes_read < 0) {
		return -1;
	}

	if (bytes_read != sizeof(old_hist)) {
		fprintf(stderr, "Error: Old history file size mismatch (expected %zu, got %d)\n",
			sizeof(old_hist), bytes_read);
		fprintf(stderr, "This may be from an incompatible Tomato version.\n");
		return -1;
	}

	/* Check ID */
	if (old_hist.id != CURRENT_ID) {
		fprintf(stderr, "Error: Invalid history file ID (0x%08x, expected 0x%08x)\n",
			old_hist.id, CURRENT_ID);
		fprintf(stderr, "This file may be corrupted or from an incompatible version.\n");
		return -1;
	}

	printf("  Old file loaded successfully\n");
	printf("  Daily pointer: %d, Monthly pointer: %d\n", old_hist.dailyp, old_hist.monthlyp);

	/* Initialize new structure (zero out extended areas) */
	memset(&new_hist, 0, sizeof(new_hist));

	/* Copy header */
	new_hist.id = old_hist.id;
	new_hist.dailyp = old_hist.dailyp;
	new_hist.monthlyp = old_hist.monthlyp;

	/* Copy daily data (same size in both formats) */
	memcpy(new_hist.daily, old_hist.daily, sizeof(old_hist.daily));

	/* Copy monthly data (old data goes into beginning of new array) */
	memcpy(new_hist.monthly, old_hist.monthly, sizeof(old_hist.monthly));

	/* Write new format */
	if (compress_file(new_file, &new_hist, sizeof(new_hist)) < 0) {
		return -1;
	}

	printf("  New file written successfully\n");
	printf("  File sizes: Old=%zu bytes, New=%zu bytes\n",
		sizeof(old_hist), sizeof(new_hist));
	printf("  Capacity expanded: +%d months available for future data\n",
		NEW_MAX_NMONTHLY - OLD_MAX_NMONTHLY);

	return 0;
}

/* Check if path is a directory */
static int is_directory(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0) {
		return S_ISDIR(st.st_mode);
	}
	return 0;
}

/* Check if file exists */
static int file_exists(const char *path)
{
	struct stat st;
	return (stat(path, &st) == 0);
}

int main(int argc, char *argv[])
{
	char old_history[512];
	char new_history[512];
	const char *old_input;
	const char *new_input;

	printf("Tomato64 rstats History Migration Tool\n");
	printf("=======================================\n\n");

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <old_file_or_dir> <new_file_or_dir>\n\n", argv[0]);
		fprintf(stderr, "Examples:\n");
		fprintf(stderr, "  # Migrate from directory to directory:\n");
		fprintf(stderr, "  %s /opt/bandwidth_old/ /opt/bandwidth_new/\n\n", argv[0]);
		fprintf(stderr, "  # Migrate specific file:\n");
		fprintf(stderr, "  %s /opt/bandwidth/bandwidth_old /opt/bandwidth/bandwidth_new\n\n", argv[0]);
		fprintf(stderr, "Migration workflow:\n");
		fprintf(stderr, "  Before upgrading (on old firmware):\n");
		fprintf(stderr, "    1. Disable bandwidth monitoring in web UI\n");
		fprintf(stderr, "    2. Backup bandwidth monitoring file to safe location\n");
		fprintf(stderr, "    3. Upgrade firmware and reboot\n\n");
		fprintf(stderr, "  After upgrading (on new firmware):\n");
		fprintf(stderr, "    4. Run migration on file in configured save location\n");
		fprintf(stderr, "    5. Re-enable bandwidth monitoring in web UI\n");
		fprintf(stderr, "    6. Verify statistics appear: Tools -> Bandwidth\n");
		return 1;
	}

	old_input = argv[1];
	new_input = argv[2];

	/* Determine old file path */
	if (is_directory(old_input)) {
		snprintf(old_history, sizeof(old_history), "%s/rstats-history.gz", old_input);
		printf("Source: Directory '%s'\n", old_input);
		printf("  Looking for: %s\n\n", old_history);
	} else {
		snprintf(old_history, sizeof(old_history), "%s", old_input);
		printf("Source: File '%s'\n\n", old_history);
	}

	/* Determine new file path */
	if (is_directory(new_input) || new_input[strlen(new_input)-1] == '/') {
		/* If new_input is a directory or ends with /, use same filename */
		char *old_basename = basename(strdup(old_history));
		snprintf(new_history, sizeof(new_history), "%s/%s", new_input, old_basename);
		printf("Destination: Directory '%s'\n", new_input);
		printf("  Will create: %s\n\n", new_history);
	} else {
		snprintf(new_history, sizeof(new_history), "%s", new_input);
		printf("Destination: File '%s'\n\n", new_history);
	}

	/* Check if old file exists */
	if (!file_exists(old_history)) {
		fprintf(stderr, "Error: Source file not found: %s\n\n", old_history);
		fprintf(stderr, "Searching for rstats files...\n");
		system("find /var /mnt -name '*rstats*.gz' 2>/dev/null | head -10");
		return 1;
	}

	/* Perform migration */
	if (migrate_history(old_history, new_history) < 0) {
		fprintf(stderr, "\nMigration failed!\n");
		fprintf(stderr, "Your original file is safe: %s\n", old_history);
		return 1;
	}

	/* Verify output */
	if (!file_exists(new_history)) {
		fprintf(stderr, "\nError: New file was not created: %s\n", new_history);
		return 1;
	}

	printf("\n=======================================\n");
	printf("Migration completed successfully!\n\n");
	printf("Files:\n");
	printf("  Original: %s (preserved)\n", old_history);
	printf("  Migrated: %s (ready to use)\n\n", new_history);
	printf("Next step:\n");
	printf("  Re-enable bandwidth monitoring in web UI\n");

	return 0;
}
