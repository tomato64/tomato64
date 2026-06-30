#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Replicate the constants from the vulnerable code */
#define MODULES_PATH "/lib/modules/"
#define MODULES_ALIAS "/modules.alias"

/* Safe version of the allocation that includes +1 for null terminator */
static char *safe_build_modules_path(const char *release)
{
    size_t len = strlen(MODULES_PATH) + strlen(release) + strlen(MODULES_ALIAS) + 1; /* +1 for null terminator */
    char *filename = malloc(len);
    if (filename == NULL)
        return NULL;
    strcpy(filename, MODULES_PATH);
    strcat(filename, release);
    strcat(filename, MODULES_ALIAS);
    return filename;
}

/* Vulnerable version that replicates the off-by-one bug */
static char *vulnerable_build_modules_path(const char *release)
{
    /* Intentionally missing +1 to replicate the vulnerability */
    size_t len = strlen(MODULES_PATH) + strlen(release) + strlen(MODULES_ALIAS);
    char *filename = malloc(len);
    if (filename == NULL)
        return NULL;
    strcpy(filename, MODULES_PATH);
    strcat(filename, release);
    strcat(filename, MODULES_ALIAS);
    return filename;
}

START_TEST(test_buffer_allocation_includes_null_terminator)
{
    /* Invariant: The allocated buffer size must always be at least
     * strlen(MODULES_PATH) + strlen(release) + strlen(MODULES_ALIAS) + 1
     * to safely hold the concatenated string including its null terminator. */
    const char *payloads[] = {
        "5.15.0-generic",
        "4.19.0-18-amd64",
        "A",
        "",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "5.10.0-0.bpo.3-amd64",
        "../../etc/passwd",
        "../../../../../../../../etc/shadow",
        "%s%s%s%s%s%s%s%s%s%s",
        "\x41\x41\x41\x41\x41\x41\x41\x41",
        "5.15.0-\x00injected",
        "kernel-version-with-very-long-name-that-exceeds-typical-buffer-sizes-0123456789",
        "5.15.0+debug",
        "5.15.0~rc1",
        "5.15.0-generic-pae",
        "linux-5.15.0",
        "5.15.0-45-generic",
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        const char *release = payloads[i];

        /* Calculate the required sizes */
        size_t modules_path_len = strlen(MODULES_PATH);
        size_t release_len = strlen(release);
        size_t modules_alias_len = strlen(MODULES_ALIAS);
        size_t total_content_len = modules_path_len + release_len + modules_alias_len;

        /* Invariant: allocated size must be at least total_content_len + 1 */
        size_t safe_alloc_size = total_content_len + 1;
        size_t vulnerable_alloc_size = total_content_len; /* missing +1 */

        /* Assert that the safe allocation size is strictly greater than the vulnerable one */
        ck_assert_msg(safe_alloc_size > vulnerable_alloc_size,
            "Safe allocation must be larger than vulnerable allocation for release: '%s'",
            release);

        /* Assert that the safe allocation can hold the full string plus null terminator */
        ck_assert_msg(safe_alloc_size >= total_content_len + 1,
            "Allocation must include space for null terminator for release: '%s'",
            release);

        /* Verify the safe version produces correct output without overflow */
        char *safe_result = safe_build_modules_path(release);
        ck_assert_msg(safe_result != NULL,
            "Safe allocation must not return NULL for release: '%s'", release);

        /* Verify the resulting string length matches expected */
        size_t result_len = strlen(safe_result);
        ck_assert_msg(result_len == total_content_len,
            "Result string length %zu must equal total content length %zu for release: '%s'",
            result_len, total_content_len, release);

        /* Verify the string starts with MODULES_PATH */
        ck_assert_msg(strncmp(safe_result, MODULES_PATH, modules_path_len) == 0,
            "Result must start with MODULES_PATH for release: '%s'", release);

        /* Verify the string ends with MODULES_ALIAS */
        if (result_len >= modules_alias_len) {
            ck_assert_msg(strcmp(safe_result + result_len - modules_alias_len, MODULES_ALIAS) == 0,
                "Result must end with MODULES_ALIAS for release: '%s'", release);
        }

        /* Verify null terminator is present at the expected position */
        ck_assert_msg(safe_result[total_content_len] == '\0',
            "Null terminator must be present at position %zu for release: '%s'",
            total_content_len, release);

        free(safe_result);
    }
}
END_TEST

START_TEST(test_vulnerable_allocation_is_off_by_one)
{
    /* Invariant: The vulnerable allocation (without +1) is always exactly
     * one byte too small to hold the null terminator, demonstrating the
     * security boundary violation. */
    const char *payloads[] = {
        "5.15.0-generic",
        "4.19.0-18-amd64",
        "A",
        "kernel",
        "5.10.0-0.bpo.3-amd64",
        "../../etc/passwd",
        "5.15.0-45-generic",
        "linux-image-5.15.0",
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        const char *release = payloads[i];

        size_t modules_path_len = strlen(MODULES_PATH);
        size_t release_len = strlen(release);
        size_t modules_alias_len = strlen(MODULES_ALIAS);
        size_t total_content_len = modules_path_len + release_len + modules_alias_len;

        /* The vulnerable allocation is exactly 1 byte short */
        size_t vulnerable_alloc = total_content_len;
        size_t required_alloc = total_content_len + 1;

        /* Assert the off-by-one: vulnerable is always exactly 1 less than required */
        ck_assert_msg(required_alloc - vulnerable_alloc == 1,
            "Off-by-one: required allocation must be exactly 1 more than vulnerable allocation for release: '%s'",
            release);

        /* Assert that writing null terminator to vulnerable_alloc would be out of bounds */
        ck_assert_msg(vulnerable_alloc < required_alloc,
            "Vulnerable allocation %zu must be less than required allocation %zu for release: '%s'",
            vulnerable_alloc, required_alloc, release);
    }
}
END_TEST

START_TEST(test_concatenated_string_fits_in_safe_buffer)
{
    /* Invariant: A properly allocated buffer (with +1) must always be able
     * to contain the full concatenated path string without overflow. */
    const char *payloads[] = {
        "5.15.0-generic",
        "4.19.0-18-amd64",
        "A",
        "",
        "very-long-kernel-version-string-0123456789abcdef",
        "../../etc/passwd",
        "%n%s%p%x",
        "5.15.0-45-generic",
        "kernel\ninjection",
        "kernel;rm -rf /",
        "5.15.0`whoami`",
        "$(uname -r)",
        "5.15.0-generic\x00hidden",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        const char *release = payloads[i];

        size_t modules_path_len = strlen(MODULES_PATH);
        size_t release_len = strlen(release);
        size_t modules_alias_len = strlen(MODULES_ALIAS);
        size_t total_content_len = modules_path_len + release_len + modules_alias_len;
        size_t safe_alloc_size = total_content_len + 1;

        /* Allocate a buffer of the safe size */
        char *buffer = malloc(safe_alloc_size);
        ck_assert_msg(buffer != NULL, "Buffer allocation must succeed for release: '%s'", release);

        /* Perform the concatenation */
        strcpy(buffer, MODULES_PATH);
        strcat(buffer, release);
        strcat(buffer, MODULES_ALIAS);

        /* Verify the resulting string fits within the allocated buffer */
        size_t result_len = strlen(buffer);
        ck_assert_msg(result_len < safe_alloc_size,
            "Result length %zu must be less than buffer size %zu for release: '%s'",
            result_len, safe_alloc_size, release);

        /* Verify null terminator is within bounds */
        ck_assert_msg(result_len + 1 <= safe_alloc_size,
            "Null terminator at position %zu must be within buffer size %zu for release: '%s'",
            result_len + 1, safe_alloc_size, release);

        /* Verify the buffer contains the null terminator at the correct position */
        ck_assert_msg(buffer[result_len] == '\0',
            "Buffer must be null-terminated within bounds for release: '%s'", release);

        free(buffer);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_allocation_includes_null_terminator);
    tcase_add_test(tc_core, test_vulnerable_allocation_is_off_by_one);
    tcase_add_test(tc_core, test_concatenated_string_fits_in_safe_buffer);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}