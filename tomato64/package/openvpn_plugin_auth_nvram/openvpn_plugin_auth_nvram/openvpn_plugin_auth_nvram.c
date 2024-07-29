/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 *
 * Fixes/updates (C) 2018 - 2024 pedro
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bcmnvram.h>
#ifdef USE_WOLFSSL
 #include <config.h>
 #include <wolfssl/options.h>
 #include <wolfssl/wolfcrypt/settings.h>
 #include <wolfssl/ssl.h>
 #include <wolfssl/error-ssl.h>
#endif
#include <openvpn-plugin.h>

#define NVRAM_KEY_MAX_LEN 32


struct nvram_context {
	char nvarm_key[NVRAM_KEY_MAX_LEN + 1];
};

static const char *get_env(const char *key, const char *env[])
{
	int i;

	if (!env)
		return (NULL);

	for (i = 0; env[i]; i++) {
		unsigned int keylen = strlen(key);

		if (keylen > strlen(env[i]))
			continue;

		if (!strncmp(key, env[i], keylen)) {
			const char *p = env[i] + keylen;
			if (*p == '=')
				return (p + 1);
		}
	}

	return (NULL);
}

/*
 * find password ptr of username
 */
static int nv_verify_pass(const char *nv_key, const char *username, const char *password)
{
	if (!nv_key || !username || !password)
		return 0;

	int i;
	const char *nv = nvram_safe_get(nv_key);
	const char *start = nv;
	int to_verify_len = strlen(username) + strlen(password) + 3; /* 1<username<password, add 3 bytes:'1 < <' */
	char *to_verify = malloc(to_verify_len + 1);
	sprintf(to_verify, "1<%s<%s", username, password); /* snprintf is not necessary */

	for (i = 0; ; i++) {
		if (nv[i] == '>') {
			if ((to_verify_len == nv + i - start) && !memcmp(to_verify, start, to_verify_len)) {
				free(to_verify);
				return 1;
			}
			start = nv + i + 1;
		}
		else if (nv[i] == 0) {
			free(to_verify);
			return 0;
		}
	}
	free(to_verify);

	return 0;
}

int string_array_len(const char *array[])
{
	int i = 0;
	if (array) {
		while (array[i])
			++i;
	}

	return i;
}

OPENVPN_EXPORT openvpn_plugin_handle_t openvpn_plugin_open_v1(unsigned int *type_mask, const char *argv[], const char *envp[])
{
	struct nvram_context *context = calloc(1, sizeof(struct nvram_context));
	*type_mask = OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY);

	if (string_array_len (argv) < 2) {
		fprintf (stderr, "AUTH-NVRAM: need NVRAM KEY parameter\n");
		goto error;
	}
	else
		strncpy(context->nvarm_key, argv[1], NVRAM_KEY_MAX_LEN);

	return (openvpn_plugin_handle_t) context;

error:
	if (context)
		free(context);

	return NULL;
}

OPENVPN_EXPORT void openvpn_plugin_close_v1(openvpn_plugin_handle_t handle)
{
	free (handle);
}

OPENVPN_EXPORT int openvpn_plugin_func_v1(openvpn_plugin_handle_t handle, const int type, const char *argv[], const char *envp[])
{
	struct nvram_context *context = (struct nvram_context *)handle;

	if (type == OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY) {
		const char *username = get_env("username", envp);
		const char *password = get_env("password", envp);

		if (username && password && nv_verify_pass(context->nvarm_key, username, password))
			return OPENVPN_PLUGIN_FUNC_SUCCESS;
	}

	return OPENVPN_PLUGIN_FUNC_ERROR;
}
