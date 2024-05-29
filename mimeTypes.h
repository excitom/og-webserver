#ifndef __MIME_TYPES
#define __MIME_TYPES
// mime types list
typedef struct _mimeTypes {
	struct _mimeTypes *next;
	char *mimeType;
	char *extension;
}_mimeTypes;
#endif
