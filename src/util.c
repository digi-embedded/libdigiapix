/*
 * Copyright 2022, Digi International Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "_util.h"

char *delete_leading_spaces(char *str)
{
	int len = 0;
	char *p = str;

	if (str == NULL || strlen(str) == 0)
		return str;

	while (isspace(*p) || !isprint(*p))
		++p;

	len = strlen(p);
	memmove(str, p, len);
	str[len] = 0;

	return str;
}

char *delete_trailing_spaces(char *str)
{
	char *p = NULL;

	if (str == NULL || strlen(str) == 0)
		return str;

	p = str + strlen(str) - 1;

	while ((isspace(*p) || !isprint(*p) || *p == 0) && p >= str)
		--p;

	*++p = 0;

	return str;
}

char *trim(char *str)
{
	return delete_leading_spaces(delete_trailing_spaces(str));
}
