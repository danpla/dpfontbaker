
#pragma once


/**
 * Split key and value.
 *
 * The key is the first word in the string, and value is the rest.
 * Whitespace around the key and value is stripped. The string is
 * parsed till the end or the first #, which is a comment start
 * mark.
 *
 * The function will modify the string to set null terminators.
 * Both key and value arguments will be non-null after function
 * returns, pointing to str buffer.
 */
void splitKeyValue(char* str, char*& key, char*& value);
