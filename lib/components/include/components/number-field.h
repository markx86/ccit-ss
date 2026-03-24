#ifndef NUMBER_FIELD_H_
#define NUMBER_FIELD_H_

#include <raylib.h>

int NumberField(Rectangle bounds, char* input, int inputSize, int* textBoxFocused);
int NumberFieldEx(Rectangle bounds, char* input, int inputSize, int* textBoxFocused, int allowHex);

#endif
