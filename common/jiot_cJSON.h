/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJiotJSON__h
#define cJiotJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* project version */
#define cJiotJSON_VERSION_MAJOR 1
#define cJiotJSON_VERSION_MINOR 7
#define cJiotJSON_VERSION_PATCH 7

#include <stddef.h>

/* cJiotJSON Types: */
#define cJiotJSON_Invalid (0)
#define cJiotJSON_False  (1 << 0)
#define cJiotJSON_True   (1 << 1)
#define cJiotJSON_NULL   (1 << 2)
#define cJiotJSON_Number (1 << 3)
#define cJiotJSON_Int64  (1 << 4)
#define cJiotJSON_String (1 << 5)
#define cJiotJSON_Array  (1 << 6)
#define cJiotJSON_Object (1 << 7)
#define cJiotJSON_Raw    (1 << 8)/* raw json */

#define cJiotJSON_IsReference 256
#define cJiotJSON_StringIsConst 512

/* The cJiotJSON structure: */
typedef struct cJiotJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJiotJSON *next;
    struct cJiotJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct cJiotJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==cJiotJSON_String  and type == cJiotJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJiotJSON_SetNumberValue instead */
    long long  valueint64;
    /* The item's number, if type==cJiotJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} cJiotJSON;

typedef struct cJiotJSON_Hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJiotJSON_Hooks;

typedef int cJiotJSON_bool;

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif
#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 2 define options:

cJiotJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
cJiotJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
cJiotJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the cJiotJSON_API_VISIBILITY flag to "export" the same symbols the way cJiotJSON_EXPORT_SYMBOLS does

*/

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(cJiotJSON_HIDE_SYMBOLS) && !defined(cJiotJSON_IMPORT_SYMBOLS) && !defined(cJiotJSON_EXPORT_SYMBOLS)
#define cJiotJSON_EXPORT_SYMBOLS
#endif

#if defined(cJiotJSON_HIDE_SYMBOLS)
#define cJiotJSON_PUBLIC(type)   type __stdcall
#elif defined(cJiotJSON_EXPORT_SYMBOLS)
#define cJiotJSON_PUBLIC(type)   __declspec(dllexport) type __stdcall
#elif defined(cJiotJSON_IMPORT_SYMBOLS)
#define cJiotJSON_PUBLIC(type)   __declspec(dllimport) type __stdcall
#endif
#else /* !WIN32 */
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(cJiotJSON_API_VISIBILITY)
#define cJiotJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define cJiotJSON_PUBLIC(type) type
#endif
#endif

/* Limits how deeply nested arrays/objects can be before cJiotJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef cJiotJSON_NESTING_LIMIT
#define cJiotJSON_NESTING_LIMIT 1000
#endif

/* returns the version of cJiotJSON as a string */
cJiotJSON_PUBLIC(const char*) cJiotJSON_Version(void);

/* Supply malloc, realloc and free functions to cJiotJSON */
cJiotJSON_PUBLIC(void) cJiotJSON_InitHooks(cJiotJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of cJiotJSON_Parse (with cJiotJSON_Delete) and cJiotJSON_Print (with stdlib free, cJiotJSON_Hooks.free_fn, or cJiotJSON_free as appropriate). The exception is cJiotJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJiotJSON object you can interrogate. */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_Parse(const char *value);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match cJiotJSON_GetErrorPtr(). */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJiotJSON_bool require_null_terminated);

/* Render a cJiotJSON entity to text for transfer/storage. */
cJiotJSON_PUBLIC(char *) cJiotJSON_Print(const cJiotJSON *item);
/* Render a cJiotJSON entity to text for transfer/storage without any formatting. */
cJiotJSON_PUBLIC(char *) cJiotJSON_PrintUnformatted(const cJiotJSON *item);
/* Render a cJiotJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
cJiotJSON_PUBLIC(char *) cJiotJSON_PrintBuffered(const cJiotJSON *item, int prebuffer, cJiotJSON_bool fmt);
/* Render a cJiotJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJiotJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_PrintPreallocated(cJiotJSON *item, char *buffer, const int length, const cJiotJSON_bool format);
/* Delete a cJiotJSON entity and all subentities. */
cJiotJSON_PUBLIC(void) cJiotJSON_Delete(cJiotJSON *c);

/* Returns the number of items in an array (or object). */
cJiotJSON_PUBLIC(int) cJiotJSON_GetArraySize(const cJiotJSON *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_GetArrayItem(const cJiotJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_GetObjectItem(const cJiotJSON * const object, const char * const string);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_GetObjectItemCaseSensitive(const cJiotJSON * const object, const char * const string);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_HasObjectItem(const cJiotJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJiotJSON_Parse() returns 0. 0 when cJiotJSON_Parse() succeeds. */
cJiotJSON_PUBLIC(const char *) cJiotJSON_GetErrorPtr(void);

/* Check if the item is a string and return its valuestring */
cJiotJSON_PUBLIC(char *) cJiotJSON_GetStringValue(cJiotJSON *item);

/* These functions check the type of an item */
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsInvalid(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsFalse(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsTrue(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsBool(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsNull(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsNumber(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsInt64(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsString(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsArray(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsObject(const cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_IsRaw(const cJiotJSON * const item);

/* These calls create a cJiotJSON item of the appropriate type. */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateNull(void);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateTrue(void);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateFalse(void);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateBool(cJiotJSON_bool boolean);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateNumber(double num);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateInt64(long long num);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateString(const char *string);
/* raw json */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateRaw(const char *raw);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateArray(void);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by cJiotJSON_Delete */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateStringReference(const char *string);
/* Create an object/arrray that only references it's elements so
 * they will not be freed by cJiotJSON_Delete */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateObjectReference(const cJiotJSON *child);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateArrayReference(const cJiotJSON *child);

/* These utilities create an Array of count items. */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateIntArray(const int *numbers, int count);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateFloatArray(const float *numbers, int count);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateDoubleArray(const double *numbers, int count);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateInt64Array(const long long *numbers, int count);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_CreateStringArray(const char **strings, int count);

/* Append item to the specified array/object. */
cJiotJSON_PUBLIC(void) cJiotJSON_AddItemToArray(cJiotJSON *array, cJiotJSON *item);
cJiotJSON_PUBLIC(void) cJiotJSON_AddItemToObject(cJiotJSON *object, const char *string, cJiotJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJiotJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & cJiotJSON_StringIsConst) is zero before
 * writing to `item->string` */
cJiotJSON_PUBLIC(void) cJiotJSON_AddItemToObjectCS(cJiotJSON *object, const char *string, cJiotJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJiotJSON to a new cJiotJSON, but don't want to corrupt your existing cJiotJSON. */
cJiotJSON_PUBLIC(void) cJiotJSON_AddItemReferenceToArray(cJiotJSON *array, cJiotJSON *item);
cJiotJSON_PUBLIC(void) cJiotJSON_AddItemReferenceToObject(cJiotJSON *object, const char *string, cJiotJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_DetachItemViaPointer(cJiotJSON *parent, cJiotJSON * const item);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_DetachItemFromArray(cJiotJSON *array, int which);
cJiotJSON_PUBLIC(void) cJiotJSON_DeleteItemFromArray(cJiotJSON *array, int which);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_DetachItemFromObject(cJiotJSON *object, const char *string);
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_DetachItemFromObjectCaseSensitive(cJiotJSON *object, const char *string);
cJiotJSON_PUBLIC(void) cJiotJSON_DeleteItemFromObject(cJiotJSON *object, const char *string);
cJiotJSON_PUBLIC(void) cJiotJSON_DeleteItemFromObjectCaseSensitive(cJiotJSON *object, const char *string);

/* Update array items. */
cJiotJSON_PUBLIC(void) cJiotJSON_InsertItemInArray(cJiotJSON *array, int which, cJiotJSON *newitem); /* Shifts pre-existing items to the right. */
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_ReplaceItemViaPointer(cJiotJSON * const parent, cJiotJSON * const item, cJiotJSON * replacement);
cJiotJSON_PUBLIC(void) cJiotJSON_ReplaceItemInArray(cJiotJSON *array, int which, cJiotJSON *newitem);
cJiotJSON_PUBLIC(void) cJiotJSON_ReplaceItemInObject(cJiotJSON *object,const char *string,cJiotJSON *newitem);
cJiotJSON_PUBLIC(void) cJiotJSON_ReplaceItemInObjectCaseSensitive(cJiotJSON *object,const char *string,cJiotJSON *newitem);

/* Duplicate a cJiotJSON item */
cJiotJSON_PUBLIC(cJiotJSON *) cJiotJSON_Duplicate(const cJiotJSON *item, cJiotJSON_bool recurse);
/* Duplicate will create a new, identical cJiotJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJiotJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
cJiotJSON_PUBLIC(cJiotJSON_bool) cJiotJSON_Compare(const cJiotJSON * const a, const cJiotJSON * const b, const cJiotJSON_bool case_sensitive);


cJiotJSON_PUBLIC(void) cJiotJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddNullToObject(cJiotJSON * const object, const char * const name);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddTrueToObject(cJiotJSON * const object, const char * const name);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddFalseToObject(cJiotJSON * const object, const char * const name);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddBoolToObject(cJiotJSON * const object, const char * const name, const cJiotJSON_bool boolean);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddNumberToObject(cJiotJSON * const object, const char * const name, const double number);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddInt64ToObject(cJiotJSON * const object, const char * const name, const long long number);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddStringToObject(cJiotJSON * const object, const char * const name, const char * const string);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddRawToObject(cJiotJSON * const object, const char * const name, const char * const raw);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddObjectToObject(cJiotJSON * const object, const char * const name);
cJiotJSON_PUBLIC(cJiotJSON*) cJiotJSON_AddArrayToObject(cJiotJSON * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJiotJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the cJiotJSON_SetNumberValue macro */
cJiotJSON_PUBLIC(double) cJiotJSON_SetNumberHelper(cJiotJSON *object, double number);
#define cJiotJSON_SetNumberValue(object, number) ((object != NULL) ? cJiotJSON_SetNumberHelper(object, (double)number) : (number))

/* Macro for iterating over an array or object */
#define cJiotJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with cJiotJSON_InitHooks */
cJiotJSON_PUBLIC(void *) cJiotJSON_malloc(size_t size);
cJiotJSON_PUBLIC(void) cJiotJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
