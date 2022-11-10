/******************************************************************************
 * 
 * @file    str_list.h
 * @date    31 Aug 2021
 * @author  Mikhail Malyarenko <malyarenko.md@gmail.com>
 * 
 * @brief   String List library API
 * 
 * API header for the String List library for C language. String List depends
 * on the String library.
 * 
 *****************************************************************************/

#ifndef __STR_LIST_H__
#define __STR_LIST_H__

#include <stdbool.h>

#include "str.h"

/**
 * @struct String List type
 */
typedef struct __str_list str_list_t;

/**
 * @brief Creates new instance of an empty string list with default capacity.
 * 
 * @return Pointer to the allocated instance of an empty string list
 */
str_list_t* str_list_new();

/**
 * @brief Created an empty string list with the given capacity.
 * 
 * @param capacity Capacity of the buffer
 * @return Pointer to the allocated instance of the string list
 * @note If capacity is 0 no memory allocation is performed
 *       and buffer is empty
 */
str_list_t* str_list_with_capacity(size_t capacity);

/**
 * @brief Creates the new copy of the string list.
 * 
 * Function creates a copy of the given string list by 
 * creating deep copies of each string that buffer contains.
 * 
 * @param other String list to be copied
 * @return Pointer to the new instance of string list which is
 *         a copy of the given string list.
 *         If the given pointer is @c NULL an empty string list is returned.
 */
str_list_t* str_list_copy(const str_list_t* other);

/**
 * @brief Drops the buffer of the string list and sets all attributes to 0.
 * 
 * @param self Pointer to the initialised string list
 * @return void
 * @note If @code{self} is @c NULL function does nothing
 * @warning After string list is dropped it must not be used
 */
void str_list_drop(str_list_t** self);

/**
 * @brief Adds string instance to the end of the list
 * 
 * Function adds pointer to the string to the end
 * of the string list. Function reallocates memory if buffer has not
 * enough space for holding data.
 * 
 * @param self Pointer to the initialised string list
 * @param string Pointer to the string to be added to the string list
 * @return void
 * @note String list takes ownership of the string
 */
void str_list_push(str_list_t* self, str_t* string);

/**
 * @brief Pops the last string in the string list
 * 
 * Function pops the last string in the string list if any and
 * deletes it from string list.
 * 
 * @param self Pointer to the initialised string list
 * @return Pointer to the last string in the string list;
 *         @c NULL if string list is empty or uninitialized
 */
str_t* str_list_pop(str_list_t* self);

/**
 * @brief Returns the size of the string list
 * 
 * @param self Pointer to the initialised string list
 * @return Size of the string list:
 *         0 if the string list is empty or @c NULL
 */
size_t str_list_size(const str_list_t* self);

/**
 * @brief Returns the capacity of the string list
 * 
 * @param self Pointer to the initialised string list
 * @return Capacity of the string list;
 *         0 if the string list's buffer is empty or if string list is @c NULL
 */
size_t str_list_cap(const str_list_t* self);

/**
 * @brief Checks if the string list is empty
 * 
 * @param self Pointer to the initialised string list
 * @return @c true if string list has at least one element
 *         0 if the string list is empty or @c NULL
 */
bool str_list_is_empty(const str_list_t* self);

/**
 * @brief Returns the string with the given index in the string list
 * starting with 0. Function provides bound checking.
 * 
 * @param self Pointer to the initialised string list
 * @param idx Position of the string in the given string list
 * @return String of the string list with the given index; @c 0 if @c idx
 *      is greater or equal to the size of the string or if the string is
 *      empty or @c NULL
 */
str_t* str_list_at(const str_list_t* self, const size_t idx);

/**
 * @brief Splits the string around the given delimeter
 * 
 * Function splits the string to one or more strings around any sequence of any
 * character provided by the given delimeter string.
 * Resulting string chunks are new instances @c str_t type.
 * 
 * @param self Pointer to the initialised string
 * @param delim Delimeter. Null-terminated string literal with
 *              valid ASCII characters.
 * @return Pointer to the new string list that contains all resulting
 *      string chunks: Empty string list if @c self is @c NULL or empty
 * 
 * @example
 * 
 *      str_t* string = str_new("One, Two; Three");
 *      str_list_t* list = str_split(string, " ,;");
 *      // Pseudocode:
 *      // list = ["One", "Two", "Three"]
 */
str_list_t* str_split(const str_t* self, const char* delim);

/**
 * @brief Joins all provided strings and puts delimeter sequence between them
 * 
 * Function joins provided string by putting them together with the given
 * delimeter sequence.
 * 
 * @param self Pointer to the initialised string list
 * @param delim Delimeter: null-terminated string literal with
 *              valid ASCII characters
 * @return Pointer to the new instance of the string that contains 
 *         joined strings; Empty string if @c self is @c NULL
 * @note If delimeter is @c NULL it considered as an empty string 
 */
str_t* str_list_join(const str_list_t* self, const char* delim);

// TODO implement str_list_sort
// void str_list_sort(str_list_t* self, bool asc);

str_list_t* str_split_whitespace(const str_t* self);

// TODO implement str_split_once
// str_list_t* str_split_once(const str_t* self, const char* delim);

// TODO implement str_rsplit_once
// str_list_t* str_rsplit_once(const str_t* self, const char* delim);

bool str_list_contains(const str_list_t* self, const str_t* string);

#endif /* __STR_LIST_H__ */
