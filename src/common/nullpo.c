/****************************************************************************!
*                _           _   _   _                                       *
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *
*                                                                            *
*                            www.brathena.org                                *
******************************************************************************
* src/common/nullpo.c                                                        *
******************************************************************************
* Copyright (c) brAthena Dev Team                                            *
* Copyright (c) Hercules Dev Team                                            *
* Copyright (c) Athena Dev Teams                                             *
*                                                                            *
* Licenciado sob a licen?a GNU GPL                                           *
* Para mais informa??es leia o arquivo LICENSE na ra?z do emulador           *
*****************************************************************************/

#define BRATHENA_CORE

#include "nullpo.h"

#include "common/showmsg.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_EXECINFO
#include <execinfo.h>
#endif // HAVE_EXECINFO

struct nullpo_interface nullpo_s;
struct nullpo_interface *nullpo;

/**
 * Reports failed assertions or NULL pointers
 *
 * @param file       Source file where the error was detected
 * @param line       Line
 * @param func       Function
 * @param targetname Name of the checked symbol
 * @param title      Message title to display (i.e. failed assertion or nullpo info)
 */
void assert_report(const char *file, int line, const char *func, const char *targetname, const char *title) {
#ifdef HAVE_EXECINFO
	void *array[10];
	int size;
	char **strings;
	int i;
#endif // HAVE_EXECINFO
	if (file == NULL)
		file = "??";

	if (func == NULL || *func == '\0')
		func = "unknown";

	ShowError("--- %s --------------------------------------------\n", title);
	ShowError("%s:%d: '%s' na funcao `%s'\n", file, line, targetname, func);
#ifdef HAVE_EXECINFO
	size = (int)backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	for (i = 0; i < size; i++)
		ShowError("%s\n", strings[i]);
	free(strings);
#endif // HAVE_EXECINFO
	ShowError("--- fim %s ----------------------------------------\n", title);
}

/**
 *
 **/
void nullpo_defaults(void) {
	nullpo = &nullpo_s;
	nullpo->assert_report = assert_report;
}
