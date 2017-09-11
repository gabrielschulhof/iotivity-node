#include <stdarg.h>
#include <string.h>
#include <ocstack.h>
#include <stdlib.h>
#include "boilerplate.h"

#ifdef __unix__
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * x)
#endif

static FILE *my_open(const char *filename, const char *mode) {
	bool needFree = false;
	char *processedFileName = processFileName(filename, &needFree);
	FILE *returnValue = fopen(processedFileName, mode);
	if (needFree) {
		free(processedFileName);
	}
	return returnValue;
}

static int my_unlink(const char *filename) {
	bool needFree = false;
	char *processedFileName = processFileName(filename, &needFree);
	int returnValue = unlink(processedFileName);
	if (needFree) {
		free(processedFileName);
	}
	return returnValue;
}

char *strdup_printf(char *format, ...) {
	char *return_value = NULL;
	va_list list;
	int size;

	va_start(list, format);
	size = vsnprintf(NULL, 0, format, list);
	va_end(list);
	return_value = malloc(size + 1);
	va_start(list, format);
	vsnprintf(return_value, size + 1, format, list);
	va_end(list);
	return_value[size] = 0;

	return return_value;
}

int
main(int argc, char **argv) {
	OCPersistentStorage storage = {
		.open = my_open,
		.read = fread,
		.write = fwrite,
		.close = fclose,
		.unlink = my_unlink
	};

	printf("OCRegisterPersistentStorageHandler: %d\n",
		OCRegisterPersistentStorageHandler(&storage));

	printf("OCInit: %d\n",
		OCInit(NULL, 0, OC_CLIENT_SERVER));

	doIoT();

	while (true) {
		OCStackResult result = OCProcess();
		if (result != OC_STACK_OK) {
			printf("OCProcess: %d\n", result);
		}
		sleep(1);
	}

	return 0;
}
