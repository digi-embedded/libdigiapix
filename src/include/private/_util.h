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

#ifndef _UTIL_H
#define _UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * delete_leading_spaces() - Delete leading spaces from the given string.
 *
 * @str:	String to delete leading spaces from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without leading white spaces.
 */
char *delete_leading_spaces(char *str);

/*
 * delete_trailing_spaces() - Delete trailing spaces from the given string.
 *
 * Trailing spaces also include new line '\n' and carriage return '\r' chars.
 *
 * @str:	String to delete trailing spaces from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without trailing white spaces.
 */
char *delete_trailing_spaces(char *str);

/*
 * trim() - Trim the given string removing leading and trailing spaces.
 *
 * Trailing spaces also include new line '\n' and carriage return '\r' chars.
 *
 * @str:	String to delete leading and trailing spaces from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without leading nor trailing white spaces.
 */
char *trim(char *str);

#ifdef __cplusplus
}
#endif

#endif /* _UTIL_H */
