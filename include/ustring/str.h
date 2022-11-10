/******************************************************************************
 * 
 * @file    str.h
 * @date    14 Nov 2021
 * @author  Mikhail Malyarenko <malyarenko.md@gmail.com>
 * 
 * @brief   String library API
 * 
 * API header for the String library.
 * 
 * Library provides dynamic heap-based string type and set of functions
 * implementing basic string operations.
 * 
 *****************************************************************************/

#ifndef __STR_H__
#define __STR_H__

#include <stddef.h>
#include <stdbool.h>

/**
 * @typedef String type
 */
typedef struct __str str_t;

/**
 * @brief Creates new instance of string.
 * 
 * Constructs new heap-allocated string instance based
 * on the provided null-terminated ASCII string.
 * Constructed string instance contains only valid ASCII
 * characters. Non-ASCII characters are replaced with '?' character. 
 * 
 * @param string Null-terminated byte string of valid ASCII characters
 * @return Pointer to the allocated instance of the string
 * @note If string is @c NULL an empty string with default capacity is created
 * @note Non-ASCII characters will be replaced with the '?' character
 */
str_t* str_new(const char* string);

/**
 * @brief Creates an empty string with the buffer of the given capacity.
 * 
 * @param capacity Capacity of the buffer
 * @return Pointer to the allocated instance of the string
 * @note If capacity is set to 0 no memory allocation
 *      is performed, string buffer is empty
 */
str_t* str_with_capacity(size_t capacity);

/**
 * @brief Creates copy of the string.
 * 
 * @param other Pointer to the initialized string instance to be copied
 * @return Pointer to the new instance of string which is
 *      a copy of the given string;
 *      If given pointer is @c NULL returns an empty string
 */
str_t* str_copy(const str_t* other);

/**
 * @brief Drops the string instance.
 * 
 * Frees the allocated memory and sets string
 * instance pointer to @c NULL  
 * 
 * @param self Pointer to pointer to the initialised string instance
 * @return void
 * @note If @c self is @c NULL or if @c *self is @c NULL function does nothing
 * @warning After string is dropped it must not be used
 */
void str_drop(str_t** self);

/**
 * @brief Returns the length of the string.
 * 
 * @return Length of the string. If @c self is @c NULL , 0 is returned.
 */
size_t str_len(const str_t* self);

/**
 * @brief Returns the capacity of the string buffer.
 * 
 * @param self Pointer to the initialised string instance
 * @return Capacity of the string's buffer;
 *      If @c self is @c NULL 0 is returned
 */
size_t str_cap(const str_t* self);

/**
 * @brief Checks if string is empty.
 * 
 * @param self Pointer to the initialised string instance
 * @return @c true if @c self is empty;
 *      @c false otherwise or if the string @c NULL
 */
bool str_is_empty(const str_t* self);

/**
 * @brief Returns character of the string at the given position
 * starting with 0. Function provides bound checking.
 * 
 * @param self Pointer to the initialised string instance
 * @param pos Position of the character in the given string
 * @return Character of the string on the given position;
 *      @c 0 if @c pos is greater or equal to the length
 *      of the string or if the string is empty or @c NULL
 */
char str_at(const str_t* self, const size_t pos);

/**
 * @brief Returns a pointer to the string buffer.
 * 
 * String buffer is holding valid null-terminated ASCII string literal,
 * so pointer to the string buffer may be used as
 * normal @c const @c char* string literal.
 * 
 * @param self Pointer to the initialised string instance
 * @return Pointer to the @c self buffer; @c NULL if @c self is @c NULL
 */
const char* str_as_ptr(const str_t* self);

/**
 * @brief Appends one string to the end of another string.
 * 
 * Function changes the @c self string by adding all characters of
 * the provided @c string to it. Second string stays unchanged.
 * Function does nothing to @c self if @c string is @c NULL or empty
 * 
 * @param self Pointer to the string instance, which will be altered
 * @param string Null-terminated string literal with valid ASCII characters
 * @return Pointer to @c self
 */
str_t* str_append(str_t* self, const char* string);

/**
 * @brief Creates new string which is a result of concatenation of
 * the given strings.
 * 
 * @param str_a,str_b Pointers to the initialized string instances
 * @return Pointer to the new string instance - a result
 *      of concatenation of the given strings
 */
str_t* str_concat(const str_t* str_a, const str_t* str_b);

/**
 * @brief Trims leading and trailing whitespace characters in the string.
 * 
 * Trims leading and trailing @b space (' '), @b h-tab (\t),
 * @b v-tab (\v), @b newline (\n) symbols if any exitsts.
 * 
 * @param self Pointesr to the @c str_t instances
 * @returns void
 */
void str_trim(str_t* self);

/**
 * @brief Compares two strings.
 * 
 * @param a,b Pointers to the initialized string instances
 * @returns @c true if strings are equal; @c false otherwise.
 *      If one of strings is @c NULL @c false is returned
 */
bool str_eq(const str_t* a, const str_t* b);

/**
 * @brief Truncates the string to the given length.
 * 
 * Function shortens the string to the given length
 * keeping the beginning of it.
 * Function does nothing if @c len is greater then @c self length.
 * 
 * @param a,b Pointers to the string instances
 * @returns @c true if strings are equal; @c false otherwise.
 *      If one of strings is @c NULL @c false is returned
 */
void str_truncate(str_t* self, size_t len);


/**
 * @brief Checks if the string contains a string pattern
 * 
 * The function checks if the entire pattern is included
 * in the string at least once.
 * 
 * @note Following conventions are used:
 *      - Empty string contains only empty pattern (zero-length pattern)
 *      - All strings contains an empty pattern
 * 
 * @param self Pointer to the initialised string instance
 * @param pattern Search pattern - null-terminated byte string
 *      of valid ASCII characters
 * @return @c true if the string contains @c pattern;
 *      @c false otherwise or if @c self is @c NULL or if @c pattern is @c NULL 
 */
bool str_contains(const str_t* self, const char* pattern);

/**
 * @brief Checks if the string contains at least one character of class
 *      determined by predicate function.
 * 
 * Function applies the predicate function to each character
 * in the string and checks if any character makes predicate function
 * return @c true 
 * 
 * @param self Pointer to the initialised string instance
 * @param fn Predicate function
 * @return @c true if the string contains at least one
 *      character @c ch : @code fn(ch) == true @endcode;
 *      @c false otherwise or if @c self is @c NULL or if @c fn is @c NULL
 */
bool str_contains_fn(const str_t* self, bool (*fn) (char));

/**
 * @brief Removes all occurrences of the pattern in the string
 * 
 * Function removes all occurrences of
 * the entire pattern in the string with greedy strategy:
 * string is scanned once from start to end and every time 
 * pattern occurs it is trimmed. If pattern or @c self string
 * is empty or @c NULL function does nothing. 
 * 
 * @param self Pointer to the initialised string instance
 * @param pattern Pattern to be removed - null-terminated byte string
 *      of valid ASCII characters
 * @return void
 */
void str_trim_matches(str_t* self, const char* pattern);

/**
 * @brief Removes all characters that matches predicate function
 * 
 * Function scans the string from start to end, applies provided
 * predicate function to each character and removes all characters
 * that matched predicate.
 * 
 * @param self Pointer to the initialised string instance
 * @param fn Predicate function
 */
void str_trim_matches_fn(str_t* self, bool (*fn) (char));

/**
 * @brief Removes prefix of the string that matches pattern
 * 
 * Function checks if string starts with the pattern
 * and if it is - matched prefix is removed. 
 * 
 * @param self Pointer to the initialised string instance
 * @param pattern Pattern of prefix to be removed -
 *      null-terminated byte string of valid ASCII characters
 */
void str_trim_start_matches(str_t* self, const char* pattern);

/**
 * @brief Removes all characters from the start of the string
 *      that matches the predicate function.
 * 
 * Function scans the string from start and removes characters
 * until they matches predicate function.
 * 
 * @param self Pointer to the initialised string instance
 * @param fn Predicate function
 */
void str_trim_start_matches_fn(str_t* self, bool (*fn) (char));

/**
 * @brief Removes suffix of the string that matches pattern
 * 
 * Function checks if string ends with the pattern
 * and if it is - matched suffix is removed. 
 * 
 * @param self Pointer to the initialised string instance
 * @param pattern Pattern of suffix to be removed -
 *      null-terminated byte string of valid ASCII characters
 */
void str_trim_end_matches(str_t* self, const char* pattern);

/**
 * @brief Removes all characters from the end of the string
 *      that matches the predicate function.
 * 
 * Function scans the string from end and removes characters
 * until they matches predicate function.
 * 
 * @param self Pointer to the initialised string instance
 * @param fn Predicate function
 */
void str_trim_end_matches_fn(str_t* self, bool (*fn) (char));

/**
 * @brief Replaces all occurrences of the pattern with the
 *      provided replacement substring
 * 
 * Function scans through string from start to end. If entire
 * pattern occurrence is detected it is replaced with provided
 * substring. Function uses greedy strategy and do not support
 * recursive replacement.  
 * 
 * @param self Pointer to the initialised string instance
 * @param pattern Pattern to be replaced
 * @param replacement Substring to be inserted in the string
 *      replacing pattern. 
 */
void str_replace(str_t* self, const char* pattern, const char* replacement);

/**
 * @brief Shrinks string buffer to fit its content.
 * 
 * Function truncates the buffer of the string so it occupies
 * only necessary amount of memory to hold string content.
 * If @c self is @c NULL or string buffer was not allocated
 * ( e.g. after @c str_with_capacity(0) ) function does nothing.
 * 
 * @param self Pointer to the string instance
 * @returns void
 */
void str_shrink_to_fit(str_t* self);


/**
 * @brief Checks if string starts with pattern.
 * 
 * Function checks if string matches the pattern (substring)
 * from the start. Every string begins with an empty pattern: @c "" .
 * 
 * @param self Pointer to the string instance
 * @param pattern Null-terminated byte string
 * @return @c true if string starts with @c pattern;
 *      @c false otherwise
 */
bool str_starts_with(const str_t* self, const char* pattern);

/**
 * @brief Checks if string ends with pattern.
 * 
 * Function checks if string matches the pattern (substring)
 * at the end. Every string ends with an empty pattern: @c "" .
 * 
 * @param self Pointer to the string instance
 * @param pattern Null-terminated byte string
 * @return @c true if string ends with @c pattern;
 *      @c false otherwise
 */
bool str_ends_with(const str_t* self, const char* pattern);

/**
 * @brief Converts string to lower case.
 * 
 * Function converts all ASCII letter characters
 * in the string to lower case. 
 * 
 * @param self Pointer to the string instance, which will be altered
 * @return void
 */
void str_to_lowercase(str_t* self);

/**
 * @brief Converts string to upper case.
 * 
 * Function converts all ASCII letter characters
 * in the string to upper case. 
 * 
 * @param self Pointer to the string instance, which will be altered
 * @return void
 */
void str_to_uppercase(str_t* self);

#endif /* __STR_H__ */
