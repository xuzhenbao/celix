/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file celix_properties.h
 * @brief Header file for the Celix Properties API.
 *
 * The Celix Properties API provides a means for storing and manipulating key-value pairs, called properties,
 * which can be used to store configuration data or metadata for a service, component, or framework configuration.
 * Functions are provided for creating and destroying property sets, loading and storing properties from/to a file
 * or stream, and setting, getting, and unsetting individual properties. There are also functions for converting
 * property values to various types (e.g. long, bool, double) and for iterating over the properties in a set.
 *
 * Supported property value types include:
 *  - string (char*)
 *  - long
 *  - double
 *  - bool
 *  - celix_version_t*
 */

#ifndef CELIX_PROPERTIES_H_
#define CELIX_PROPERTIES_H_

#include <stdio.h>

#include "celix_properties_type.h"
#include "celix_cleanup.h"
#include "celix_compiler.h"
#include "celix_errno.h"
#include "celix_utils_export.h"
#include "celix_version.h"
#include "celix_array_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enum representing the possible types of a property value.
 */
typedef enum celix_properties_value_type {
    CELIX_PROPERTIES_VALUE_TYPE_UNSET = 0,   /**< Property value is not set. */
    CELIX_PROPERTIES_VALUE_TYPE_STRING = 1,  /**< Property value is a UTF-8 encoded string. */
    CELIX_PROPERTIES_VALUE_TYPE_LONG = 2,    /**< Property value is a long integer. */
    CELIX_PROPERTIES_VALUE_TYPE_DOUBLE = 3,  /**< Property value is a double. */
    CELIX_PROPERTIES_VALUE_TYPE_BOOL = 4,    /**< Property value is a boolean. */
    CELIX_PROPERTIES_VALUE_TYPE_VERSION = 5, /**< Property value is a Celix version. */
    CELIX_PROPERTIES_VALUE_TYPE_ARRAY_LIST =
        6, /**< Property value is an array list. The element type of the array list can be
              CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING, CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG,
              CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE, CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL or
              CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION. */
} celix_properties_value_type_e;

/**
 * @brief A structure representing a single value entry in a property set.
 */
typedef struct celix_properties_entry {
    const char* value;                       /**< The string value or string representation of a non-string
                                                  typed value.*/
    celix_properties_value_type_e valueType; /**< The type of the value of the entry */

    union {
        const char* strValue;                /**< The string value of the entry. */
        long longValue;                      /**< The long integer value of the entry. */
        double doubleValue;                  /**< The double-precision floating point value of the entry. */
        bool boolValue;                      /**< The boolean value of the entry. */
        const celix_version_t* versionValue; /**< The Celix version value of the entry. */
        const celix_array_list_t*
            arrayValue; /**< The array list of longs, doubles, bools, strings or versions value of the entry. */
    } typed;            /**< The typed values of the entry. Only valid if valueType
                             is not CELIX_PROPERTIES_VALUE_TYPE_UNSET and only the matching
                             value types should be used. E.g typed.boolValue if valueType is
                             CELIX_PROPERTIES_VALUE_TYPE_BOOL. */
} celix_properties_entry_t;

/**
 * @brief Represents an iterator for iterating over the entries in a celix_properties_t object.
 */
typedef struct celix_properties_iterator {
    /**
     * @brief The current key.
     */
    const char* key;

    /**
     * @brief The current value entry.
     */
    celix_properties_entry_t entry;

    /**
     * @brief Private data used to implement the iterator.
     */
    char _data[56];
} celix_properties_iterator_t;

/**
 * @brief Create a new empty property set.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @return A new empty property set.
 */
CELIX_UTILS_EXPORT celix_properties_t* celix_properties_create();

/**
 * @brief Destroy a property set, freeing all associated resources.
 *
 * @param[in] properties The property set to destroy. If properties is NULL, this function will do nothing.
 */
CELIX_UTILS_EXPORT void celix_properties_destroy(celix_properties_t* properties);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_properties_t, celix_properties_destroy)

/**
 * @brief Get the entry for a given key in a property set.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key to search for.
 * @return The entry for the given key, or a NULL if the key is not found.
 */
CELIX_UTILS_EXPORT const celix_properties_entry_t* celix_properties_getEntry(const celix_properties_t* properties,
                                                                       const char* key);

/**
 * @brief Get the string value or string representation of a property.
 *
 * @note identical to celix_properties_getAsString
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set.
 * @return The value of the property, or the default value if the property is not set.
 */
CELIX_UTILS_EXPORT const char*
celix_properties_get(const celix_properties_t* properties, const char* key, const char* defaultValue);

/**
 * @brief Get the type of a property value.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get the type of.
 * @return The type of the property value, or CELIX_PROPERTIES_VALUE_TYPE_UNSET if the property is not set.
 */
CELIX_UTILS_EXPORT celix_properties_value_type_e celix_properties_getType(const celix_properties_t* properties,
                                                                          const char* key);

/**
 * @brief Check if the properties set has the provided key.
 * @param[in] properties The property set to search.
 * @param[in] key The key to search for.
 * @return True if the property set has the provided key, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_properties_hasKey(const celix_properties_t* properties, const char* key);

/**
 * @brief Set the string value of a property.
 *
 * @note Identical to celix_properties_setString.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_STRING.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] value The value to set the property to.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_set(celix_properties_t* properties,
                                                       const char* key,
                                                       const char* value);

/**
 * @brief Set the value of a property without copying the value string.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_STRING.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set. This callee will take ownership of the key, so the key must not be
 *                used after calling this function. The key should be deallocated with free.
 * @param[in] value The value to set the property to. This callee will take ownership of the value, so the value must
 *                  not be used after calling this function. The value should be deallocated with free.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key or value is NULL.
 *         When an error status is returned, the key and value will be freed by this function.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_assign(celix_properties_t* properties,
                                                                  char* key,
                                                                  char* value);

/**
 * @brief Get the value of a property, if the property is set and the underlying type is a string.
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return The value of the property, or NULL if the property is not set or the value is not of the requested type.
 */
CELIX_UTILS_EXPORT const char*
celix_properties_getString(const celix_properties_t* properties, const char* key);

/**
 * @brief Get the string value or string representation of a property.
 *
 * @note identical to celix_properties_get
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set.
 * @return The value of the property, or the default value if the property is not set.
 */
CELIX_UTILS_EXPORT const char*
celix_properties_getAsString(const celix_properties_t* properties, const char* key, const char* defaultValue);

/**
 * @brief Set the string value of a property.
 *
 * @note Identical to celix_properties_set.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_STRING.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] value The value to set the property to.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setString(celix_properties_t* properties,
                                                             const char* key,
                                                             const char* value);

/**
 * @brief Assign the value of a property with the provided string pointer.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_STRING.
 *
 * This function take ownership of the provided string.
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] value The value to assign. The function take ownership of the provided version. Cannot be NULL.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL. When an error status is returned,
 *         the string will be free by this function.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_assignString(celix_properties_t* properties,
                                                                const char* key,
                                                                char* value);

/**
 * @brief Get the value of a property, if the property is set and the underlying type is a long.
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set or the value is not a long.
 * @return The value of the property, or the default value if the property is not set or the value is not of the
 * requested type.
 */
CELIX_UTILS_EXPORT long
celix_properties_getLong(const celix_properties_t* properties, const char* key, long defaultValue);

/**
 * @brief Get the value of a property as a long integer.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set, the value is not a long integer,
 *                     or if the value cannot be converted to a long integer.
 * @return The value of the property as a long integer, or the default value if the property is not set,
 *         the value is not a long integer, or if the value cannot be converted to a long integer.
 *         If the value is a string, it will be converted to a long integer if possible.
 */
CELIX_UTILS_EXPORT long
celix_properties_getAsLong(const celix_properties_t* properties, const char* key, long defaultValue);

/**
 * @brief Set the value of a property to a long integer.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_LONG.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] value The long value to set the property to.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setLong(celix_properties_t* properties, const char* key, long value);

/**
 * @brief Get the value of a property, if the property is set and the underlying type is a boolean.
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set or the value is not a boolean.
 * @return The value of the property, or the default value if the property is not set or the value is not of the
 * requested type.
 */
CELIX_UTILS_EXPORT bool
celix_properties_getBool(const celix_properties_t* properties, const char* key, bool defaultValue);

/**
 * @brief Get the value of a property as a boolean.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set, the value is not a boolean, or if the value
 *                     cannot be converted to a boolean.
 * @return The value of the property as a boolean, or the default value if the property is not set, the value is not a
 *         boolean, or if the value cannot be converted to a boolean. If the value is a string, it will be converted
 *         to a boolean if possible.
 */
CELIX_UTILS_EXPORT bool
celix_properties_getAsBool(const celix_properties_t* properties, const char* key, bool defaultValue);

/**
 * @brief Set the value of a property to a boolean.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_BOOL.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] val The boolean value to set the property to.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setBool(celix_properties_t* properties, const char* key, bool val);

/**
 * @brief Set the value of a property to a double.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_DOUBLE.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] val The double value to set the property to.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setDouble(celix_properties_t* properties,
                                                             const char* key,
                                                             double val);

/**
 * @brief Get the value of a property, if the property is set and the underlying type is a double.
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set or the value is not a double.
 * @return The value of the property, or the default value if the property is not set or the value is not of the
 * requested type.
 */
CELIX_UTILS_EXPORT double
celix_properties_getDouble(const celix_properties_t* properties, const char* key, double defaultValue);

/**
 * @brief Get the value of a property as a double.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set, the value is not a double,
 *                     or if the value cannot be converted to a double.
 * @return The value of the property as a double, or the default value if the property is not set, the value is not
 *         a double, or if the value cannot be converted to a double. If the value is a string, it will be converted
 *         to a double if possible.
 */
CELIX_UTILS_EXPORT double
celix_properties_getAsDouble(const celix_properties_t* properties, const char* key, double defaultValue);

/**
 * @brief Set the value of a property as a Celix version.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_VERSION.
 *
 * This function will make a copy of the provided celix_version_t object and store it in the property set.
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] version The value to set. The function will make a copy of this object and store it in the property set.
 * Cannot be NULL.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setVersion(celix_properties_t* properties,
                                                              const char* key,
                                                              const celix_version_t* version);

/**
 * @brief Assign the value of a property with the provided Celix version pointer.
 *
 * The set property type will be CELIX_PROPERTIES_VALUE_TYPE_VERSION.
 *
 * This function will store a reference to the provided celix_version_t object in the property set and takes
 * ownership of the provided version.
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] version The value to assign. The function will store a reference to this object in the property set and
 *                    takes ownership of the provided version. Cannot be NULL.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL. When an error status is returned,
 *         the version will be destroy with celix_version_destroy by this function.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_assignVersion(celix_properties_t* properties,
                                                                 const char* key,
                                                                 celix_version_t* version);

/**
 * @brief Get the Celix version value of a property without copying.
 *
 * This function provides a non-owning, read-only access to a Celix version contained in the properties.
 * It returns a const pointer to the Celix version value associated with the specified key.
 * This function does not perform any conversion from a string property value to a Celix version.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the Celix version if it is present and valid, or NULL if the
 * property is not set or the value is not a valid Celix version. The returned pointer should not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_version_t* celix_properties_getVersion(const celix_properties_t* properties,
                                                                      const char* key);

/**
 * @brief Get a value of a property as a copied Celix version.
 *
 * If the property value is a Celix version, a copy of the found version will be returned.
 * If the property value is present, but not a Celix version, this function will attempt to convert the property value
 * to a new Celix version.
 * If the property is not set or is not a valid Celix version string, a copy of the provided defaultValue is returned.
 *
 * @note The caller is responsible for deallocating the memory of the returned version.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The value to return if the property is not set or if the value is not a Celix version.
 * @param[out] version A copy of the found version, a new parsed version, or a copy of the default value if the
 *                 property is not set, its value is not an version or its value cannot be converted to an version.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to create the
 *        version. Note if the key is not found, the return status is still CELIX_SUCCESS.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_getAsVersion(const celix_properties_t* properties,
                                                                  const char* key,
                                                                  const celix_version_t* defaultValue,
                                                                  celix_version_t** version);

/**
 * @brief Set a pointer, long, double, bool, string or version array list array for a property value.
 *
 * The set property type cannot be CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED or CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER
 *
 * This function will make a copy of the provided celix_array_list_t object, using the celix_arrayList_copy function.
 *
 * If an error occurs, the error status is returned and a message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] values An array list of types values to set for the property. Cannot be NULL.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry,
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key or values is NULL or if the array list type is
 *         valid.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setArrayList(celix_properties_t* properties,
                                                                const char* key,
                                                                const celix_array_list_t* values);

/**
 * @brief Assign a pointer, long, double, bool, string or version array list array for a property value.
 *
 * The set property type cannot be CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED or CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER
 *
 * This function stores a reference to the provided celix_array_list_t object in the property set and takes
 * ownership of the array.
 * If an error occurs, the error status is returned, the provided array is destroyed and a
 * message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] values An array list of long values to assign to the property. Ownership of the array is transferred
 *                   to the properties set. Cannot be NULL.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry,
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL, values is NULL or the array list type is
 *         invalid. On error, the provided array list is destroyed.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_assignArrayList(celix_properties_t* properties,
                                                                   const char* key,
                                                                   celix_array_list_t* values);

/**
 * @brief Get the property value as an array without copying.
 *
 * This function provides a non-owning, read-only access to a array property value.
 * It returns a const pointer to the array. If the property is not set or its value is not an array, NULL is returned.
 *
 * The returned array will have a element type of:
 *  - CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING
 *  - CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG
 *  - CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE
 *  - CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL
 *  - CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION
 *
 *  Note that users should check the element type of the returned array list to determine the type of the elements.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the array list property value, or NULL if the property is not set or its value is not
 * an array list. The returned pointer should not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_array_list_t* celix_properties_getArrayList(const celix_properties_t* properties,
                                                                           const char* key);

/**
 * @brief Get a property value as an array of longs.
 *
 * This function retrieves the value of a property, interpreting it as an array of longs. If the underlying type of the
 * property value is a long array, a copy of the array is returned. If the underlying type is a string, the string is
 * converted to an array of longs if possible.
 * If the property is not set, its value is not an array of longs or its value cannot be converted to a long array,
 * the default value is returned as a copy.
 *
 * An celix err is logged if the default value is needed and not an array list with long values.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The default value to return if the property is not set or its value is not an array of longs.
 * @param[out] list A copy of the found list, a new array list with long values or a copy of the default value if the
 *                 property is not set, its value is not an array of longs or its value cannot be converted to an array
 *                 of longs.
 * @return CELIX_SUCCESS if the operation was successful. Note if the key is not found or the value cannot be converted
 * to an array of longs, the return status is still CELIX_SUCCESS.
 * @returnval CELIX_ENOMEM if there was not enough memory to create the array list.
 * @returnval CELIX_ILLEGAL_ARGUMENT if the provided default value is not NULL and not an array list with long values.
 *            In this case an error message is also logged to celix_err.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_getAsLongArrayList(const celix_properties_t* properties,
                                                                  const char* key,
                                                                  const celix_array_list_t* defaultValue,
                                                                  celix_array_list_t** list);

/**
 * @brief Get the property value as an array of longs without copying.
 *
 * This function provides a non-owning, read-only access to an array of longs property value.
 * It returns a const pointer to the array. If the property is not set or its value is not an array of longs,
 * NULL is returned.
 *
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the property value interpreted as an array of longs, or NULL if the property
 *         is not set or its value is not an array of longs. The returned pointer should not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_array_list_t* celix_properties_getLongArrayList(const celix_properties_t* properties,
                                                                            const char* key);

/**
 * @brief Get a property value as an array of doubles, making a copy of the array.
 *
 * This function retrieves the value of a property, interpreting it as an array of doubles. If the underlying type of
 * the property value is a double array, a copy of the array is returned. If the underlying type is a string, the string
 * is converted to an array of doubles if possible. If the property is not set, its value is not an array of doubles or
 * its value cannot be converted to a double array, the default value is returned as a copy.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The default value to return if the property is not set or its value is not an array of
 * doubles.
 * @param[out] list A copy of the found list, a new array list with double values or a copy of the default value if the
 *                 property is not set, its value is not an array doubles longs or its value cannot be converted to an
 * array of doubles.
 * @return CELIX_SUCCESS if the operation was successful. Note if the key is not found or the value cannot be converted
 * to an array of doubles, the return status is still CELIX_SUCCESS.
 * @returnval CELIX_ENOMEM if there was not enough memory to create the array list.
 * @returnval CELIX_ILLEGAL_ARGUMENT if the provided default value is not NULL and not an array list with double values.
 *            In this case an error message is also logged to celix_err.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_getAsDoubleArrayList(const celix_properties_t* properties,
                                                                        const char* key,
                                                                        const celix_array_list_t* defaultValue,
                                                                        celix_array_list_t** list);

/**
 * @brief Get the property value as an array of doubles without copying.
 *
 * This function provides a non-owning, read-only access to an array of doubles property value.
 * It returns a const pointer to the array. If the property is not set or its value is not an array of doubles,
 * NULL is returned.
 *
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the property value interpreted as an array of doubles, or NULL if the
 * property is not set or its value is not an array of doubles. The returned pointer should not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_array_list_t* celix_properties_getDoubleArrayList(const celix_properties_t* properties,
                                                                                 const char* key);

/**
 * @brief Get a property value as an array of booleans, making a copy of the array.
 *
 * This function retrieves the value of a property, interpreting it as an array of booleans. If the underlying type of
 * the property value is a boolean array, a copy of the array is returned. If the underlying type is a string, the
 * string is converted to an array of booleans if possible. If the property is not set, its value is not an array of
 * booleans or its value cannot be converted to a boolean array, the default value is returned as a copy.
 *
 * An celix err is logged if the default value is needed and not an array list with boolean values.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The default value to return if the property is not set or its value is not an array of
 * booleans.
 * @param[out] list A copy of the found list, a new array list with boolean values or a copy of the default value if the
 *                 property is not set, its value is not an array of booleans or its value cannot be converted to an
 * array of booleans.
 * @return CELIX_SUCCESS if the operation was successful. Note if the key is not found or the value cannot be converted
 * to an array of booleans, the return status is still CELIX_SUCCESS.
 * @returnval CELIX_ENOMEM if there was not enough memory to create the array list.
 * @returnval CELIX_ILLEGAL_ARGUMENT if the provided default value is not NULL and not an array list with bool values.
 *            In this case an error message is also logged to celix_err.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_getAsBoolArrayList(const celix_properties_t* properties,
                                                                      const char* key,
                                                                      const celix_array_list_t* defaultValue,
                                                                      celix_array_list_t** list);

/**
 * @brief Get the property value as an array of booleans without copying.
 *
 * This function provides a non-owning, read-only access to a array of booleans property value.
 * It returns a const pointer to the array. If the property is not set or its value is not an array of booleans,
 * NULL is returned.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the property value interpreted as an array of booleans, or NULL if the
 * property is not set or its value is not an array of booleans. The returned pointer should not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_array_list_t* celix_properties_getBoolArrayList(const celix_properties_t* properties,
                                                                               const char* key);

/**
 * @brief Get a property value as an array of strings, making a copy of the array.
 *
 * This function retrieves the value of a property, interpreting it as an array of strings. If the underlying type of
 * the property value is a string array, a copy of the array is returned. If the underlying type is a string, the string
 * is converted to an array of strings if possible. If the property is not set, its value is not an array of strings or
 *  its value cannot be converted to a string array, the default value is returned as a copy.
 *
 * The returned array list is configured with a remove callback so that the destruction of the array list will also
 * free the strings in the array list.
 *
 * An celix err is logged if the default value is needed and not an array list with string values.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The default value to return if the property is not set or its value is not an array of
 * strings.
 * @param[out] list A copy of the found list, a new array list with string values or a copy of the default value if the
 *                 property is not set, its value is not an array of strings or its value cannot be converted to an
 *                 array of strings.
 * @return CELIX_SUCCESS if the operation was successful. Note if the key is not found or the value cannot be converted
 * to an array of strings, the return status is still CELIX_SUCCESS.
 * @returnval CELIX_ENOMEM if there was not enough memory to create the array list.
 * @returnval CELIX_ILLEGAL_ARGUMENT if the provided default value is not NULL and not an array list with string values.
 *            In this case an error message is also logged to celix_err.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_getAsStringArrayList(const celix_properties_t* properties,
                                                                        const char* key,
                                                                        const celix_array_list_t* defaultValue,
                                                                        celix_array_list_t** list);

/**
 * @brief Get the property value as an array of strings without copying.
 *
 * This function provides a non-owning, read-only access to an array of string property value.
 * It returns a const pointer to the array. If the property is not set or its value is not an array of strings,
 * NULL is returned.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the property value interpreted as an array of strings, or NULL if the
 * property is not set or its value is not an array of strings. The returned pointer should not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_array_list_t* celix_properties_getStringArrayList(const celix_properties_t* properties,
                                                                                 const char* key);

/**
 * @brief Get a property value as an array of celix_version_t entries, making a copy of the array.
 *
 *
 * This function retrieves the value of a property, interpreting it as an array of celix_version_t* entries. If the
 * underlying type of the property value is a celix_version_t* array, a copy of the array is returned. If the underlying
 * type is a string, the string is converted to an array of celix_version_t* if possible. If the property is not set,
 * its value is not an array of celix_version_t* entries or its value cannot be converted to a celix_version_t* array,
 * the default value is returned as a copy.
 *
 * The returned array list is configured with a remove callback so that the destruction of the array list will also
 * free the celix_version_t entries in the array list.
 *
 * An celix err is logged if the default value is needed and not an array list with celix_version_t values.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @param[in] defaultValue The default value to return if the property is not set or its value is not an array of
 * celix_version_t entries.
 * @param[out] list A copy of the found list, a new array list with celix_version_t values or a copy of the default
 * value if the property is not set, its value is not an array of celix_version_t entries or its value cannot be
 * converted to an array of celix_version_t entries.
 * @return CELIX_SUCCESS if the operation was successful. Note if the key is not found or the value cannot be converted
 * to an array of celix_version_t, the return status is still CELIX_SUCCESS.
 * @returnval CELIX_ENOMEM if there was not enough memory to create the array list.
 * @returnval CELIX_ILLEGAL_ARGUMENT if the provided default value is not NULL and not an array list with
 * celix_version_t values. In this case an error message is also logged to celix_err.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_getAsVersionArrayList(const celix_properties_t* properties,
                                                                         const char* key,
                                                                         const celix_array_list_t* defaultValue,
                                                                         celix_array_list_t** list);

/**
 * @brief Get the property value as an array of celix_version_t entries without copying.
 *
 * This function provides a non-owning, read-only access to an array of celix_version_t property value.
 * entries. It returns a const pointer to the array. If the property is not set or its value is not an array of
 * celix_version_t entries, NULL is returned.
 *
 * @param[in] properties The property set to search.
 * @param[in] key The key of the property to get.
 * @return A const pointer to the property value interpreted as an array of celix_version_t entries, or NULL if the
 * property is not set or its value is not an array of celix_version_t entries. The returned pointer should
 * not be modified or freed.
 */
CELIX_UTILS_EXPORT const celix_array_list_t* celix_properties_getVersionArrayList(const celix_properties_t* properties,
                                                                                  const char* key);

/**
 * @brief Set the value of a property based on the provided property entry, maintaining underlying type.
 *
 * The typed entry value will be copied, which means that this function will use the entry.typed.strValue,
 * entry.typed.longValue, entry.typed.doubleValue, entry.typed.boolValue or entry.typed.versionValue depending on
 * the entry.valueType. The entry.strValue will be ignored.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to set.
 * @param[in] entry The entry to set the property to. The typed entry will be copied, so it can be freed after calling
 *                  this function.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ENOMEM if there was not enough memory to set the entry
 *         and CELIX_ILLEGAL_ARGUMENT if the provided key is NULL.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_setEntry(celix_properties_t* properties,
                                                            const char* key,
                                                            const celix_properties_entry_t* entry);

/**
 * @brief Unset a property, removing it from the property set.
 *
 * @param[in] properties The property set to modify.
 * @param[in] key The key of the property to unset.
 */
CELIX_UTILS_EXPORT void celix_properties_unset(celix_properties_t* properties, const char* key);

/**
 * @brief Make a copy of a properties set.
 *
 * If the return status is an error, an error message is logged to celix_err.
 *
 * @param[in] properties The property set to copy.
 * @return A copy of the given property set.
 */
CELIX_UTILS_EXPORT celix_properties_t* celix_properties_copy(const celix_properties_t* properties);

/**
 * @brief Get the number of properties in a property set.
 *
 * @param[in] properties The property set to get the size of.
 * @return The number of properties in the property set.
 */
CELIX_UTILS_EXPORT size_t celix_properties_size(const celix_properties_t* properties);

/**
 * @brief Check whether the provided property sets are equal.
 *
 * Equals means that both property sets have the same number of properties and that all properties in the first set
 * are also present in the second set and have the same value.
 *
 * @param[in] props1 The first property set to compare.
 * @param[in] props2 The second property set to compare.
 * @return true if the property sets are equal, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_properties_equals(const celix_properties_t* props1, const celix_properties_t* props2);

/**
 * @brief Construct an iterator pointing to the first entry in the properties object.
 *
 * @param[in] properties The properties object to iterate over.
 * @return The iterator pointing to the first entry in the properties object.
 */
CELIX_UTILS_EXPORT celix_properties_iterator_t celix_properties_begin(const celix_properties_t* properties);

/**
 * @brief Construct an iterator pointing to the past-the-end entry in the properties object.
 *
 * This iterator is used to mark the end of the properties object and is not associated with any element in the
 * properties object.
 *
 * @param[in] properties The properties object to iterate over.
 * @return The iterator pointing to the past-the-end entry in the properties object.
 */
CELIX_UTILS_EXPORT celix_properties_iterator_t celix_properties_end(const celix_properties_t* properties);

/**
 * @brief Advance the iterator to the next entry.
 *
 * @param[in, out] iter The iterator.
 */
CELIX_UTILS_EXPORT void celix_propertiesIterator_next(celix_properties_iterator_t* iter);

/**
 * @brief Determine whether the iterator is pointing to an end position.
 *
 * An iterator is at an end position if it has no more entries to visit.
 *
 * @param[in] iter The iterator.
 * @return true if the iterator is at an end position, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_propertiesIterator_isEnd(const celix_properties_iterator_t* iter);

/**
 * @brief Determine whether two iterators are equal.
 *
 * @param[in] a The first iterator to compare.
 * @param[in] b The second iterator to compare.
 * @return true if the iterators are equal, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_propertiesIterator_equals(const celix_properties_iterator_t* a,
                                                        const celix_properties_iterator_t* b);

/**
 * @brief Iterate over the entries in the specified celix_properties_t object.
 *
 * This macro allows you to easily iterate over the entries in a celix_properties_t object.
 * The loop variable `iterName` will be of type celix_properties_iterator_t and will contain the current
 * entry during each iteration.
 *
 * @param[in] props The properties object to iterate over.
 * @param[in] iterName The name of the iterator variable to use in the loop.
 *
 * Example usage:
 * @code{.c}
 * // Iterate over all entries in the properties object
 * CELIX_PROPERTIES_ITERATE(properties, iter) {
 *     // Print the key and value of the current entry
 *     printf("%s: %s\n", iter.entry.key, iter.entry.value);
 * }
 * @endcode
 */
#define CELIX_PROPERTIES_ITERATE(props, iterName)                                                                      \
    for (celix_properties_iterator_t iterName = celix_properties_begin((props));                                       \
         !celix_propertiesIterator_isEnd(&(iterName));                                                                 \
         celix_propertiesIterator_next(&(iterName)))

/**
 * @brief Flag to indicate that the encoded output should be pretty; e.g. encoded with additional whitespaces,
 * newlines and indentation.
 *
 * If this flag is not set, the encoded output will compact; e.g. without additional whitespaces, newlines and
 * indentation.
 */
#define CELIX_PROPERTIES_ENCODE_PRETTY 0x01

/**
 * @brief Flag to indicate that the encoded output should be flat; e.g. all properties entries are written as top level
 * field entries.
 *
 * E.g:
 * @code{.c}
 * celix_properties_t* properties = celix_properties_create();
 * celix_properties_setString(properties, "key/with/slash", "value1");
 * celix_properties_setString(properties, "key", "value2");
 * char* json;
 * celix_properties_saveToString(properties, CELIX_PROPERTIES_ENCODE_FLAT, &json);
 * // json will be: {"key/with/slash": "value1", "key": "value2"}
 * @endcode
 *
 * Note that encoding with a flat encoding style, all properties keys are unique JSON keys and can be written.
 *
 * If no encoding style flag is set, the encoded output will use the default encoding style.
 */
#define CELIX_PROPERTIES_ENCODE_FLAT_STYLE 0x02

/**
 * @brief Flag to indicate that the encoded output should be nested; e.g. properties entries are split on '/' and nested
 * in JSON objects.
 *
 * E.g:
 * @code{.c}
 * celix_properties_t* properties = celix_properties_create();
 * celix_properties_setString(properties, "key/with/slash", "value1");
 * celix_properties_setString(properties, "key", "value2");
 * char* json;
 * celix_properties_saveToString(properties, CELIX_PROPERTIES_ENCODE_NESTED, &json);
 * // json will be: {"key":{"with":{"slash": "value1"}}}
 * // or
 * // json will be: {"key": "value2"}
 * @endcode
 *
 * Note that encoding with a nested encoding style, it properties key can collide resulting in missing properties
 * entries or (if CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS is set) an error.
 *
 * If no encoding style flag is set, the encoded output will use the default encoding style.
 */
#define CELIX_PROPERTIES_ENCODE_NESTED_STYLE 0x04

/**
 * @brief Flag to indicate that the encoding should fail if the JSON representation will contain colliding keys.
 *
 * Note that colliding keys can only occur when using the nested encoding style.
 *
 * E.g. the following will lead to an error:
 * @code{.c}
 * celix_properties_t* properties = celix_properties_create();
 * celix_properties_setString(properties, "key/with/slash", "value1");
 * celix_properties_setString(properties, "key", "value2"); //collision
 * char* json;
 * celix_status_t status = celix_properties_saveToString(properties,
 *     CELIX_PROPERTIES_ENCODE_NESTED | | CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS, &json);
 * // status will be CELIX_ILLEGAL_ARGUMENT and a error message will be logged to celix_err
 * @endcode
 *
 * If this flag is set, the encoding will fail if the JSON representation will contain colliding keys and if this flag
 * is not set, the encoding will not fail and the colliding keys will be ignored.
 */
#define CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS 0x10

/**
 * @brief Flag to indicate that the encoding should fail if the JSON representation will contain empty arrays.
 *
 * Although empty arrays are valid in JSON, they cannot be decoded to a valid properties array entry and as such
 * empty arrays properties entries are not encoded.
 *
 * If this flag is set, the encoding will fail if the JSON representation will contain empty arrays and if this flag
 * is not set, the encoding will not fail and the empty arrays will be ignored.
 */
#define CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS 0x20

/**
 * @brief Flag to indicate that the encoding should fail if the JSON representation will contain NaN or Inf values.
 *
 * NaN, Inf and -Inf are not valid JSON values and as such properties entries with these values are not encoded.
 *
 * If this flag is set, the encoding will fail if the JSON representation will contain NaN or Inf values and if this
 * flag is not set, the encoding will not fail and the NaN and Inf entries will be ignored.
 */
#define CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF 0x40

/**
 * @brief Flag to indicate that all encode "error on" flags should be set.
 */
#define CELIX_PROPERTIES_ENCODE_STRICT                                                                                 \
    (CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS | CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS |                     \
     CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF)

/**
 * @brief Save (encode) as a JSON representation to a stream.
 *
 * The stream is expected to be a valid stream and is not reset or closed by this function.
 *
 * Properties are encoded as a JSON object.
 *
 * If no encoding style flag is set or when the CELIX_PROPERTIES_ENCODE_FLAT_STYLE flag is set, properties
 * entries are written as top level field entries.
 *
 * If the CELIX_PROPERTIES_ENCODE_NESTED_STYLE flag is set, properties entry keys are split on '/' and nested in
 * JSON objects. This leads to a more natural JSON representation, but if there are colliding properties keys (e.g.
 * `{"key": "value1", "key/with/slash": "value2"}`), not all properties entries will be written.
 *
 * With all encoding styles, the empty array properties entries are ignored, because they cannot be decoded to a valid
 * properties array entry.
 *
 * Properties type entries are encoded as follows:
 * - CELIX_PROPERTIES_TYPE_STRING: The value is encoded as a JSON string.
 * - CELIX_PROPERTIES_TYPE_LONG: The value is encoded as a JSON number.
 * - CELIX_PROPERTIES_TYPE_DOUBLE: The value is encoded as a JSON number.
 * - CELIX_PROPERTIES_TYPE_BOOL: The value is encoded as a JSON boolean.
 * - CELIX_PROPERTIES_TYPE_ARRAY: The value is encoded as a JSON array, with each element encoded according to its type.
 * - CELIX_PROPERTIES_TYPE_VERSION: The value is encoded as a JSON string with a "version<" prefix and a ">" suffix
 * (e.g. "version<1.2.3>").
 *
 * For a overview of the possible encode flags, see the CELIX_PROPERTIES_ENCODE_* flags documentation.
 * The default encoding style is a compact and flat JSON representation.
 *
 * @param properties The properties object to encode.
 * @param stream The stream to write the JSON representation of the properties object to.
 * @param encodeFlags The flags to use when encoding the input properties.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ILLEGAL_ARGUMENT if the provided properties cannot be
 * encoded to a JSON representation, ENOMEM if there was not enough memory and CELIX_FILE_IO_EXCEPTION if the stream
 * could not be written to.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_saveToStream(const celix_properties_t* properties,
                                                                FILE* stream,
                                                                int encodeFlags);

/**
 * @brief Save (encode) properties as a JSON representation to a file.
 *
 * For more information how a properties object is encoded to JSON, see the celix_properties_loadFromStream
 *
 * For a overview of the possible encode flags, see the CELIX_PROPERTIES_ENCODE_* flags documentation.
 * The default encoding style is a compact and flat JSON representation.
 *
 * @param[in] properties The properties object to encode.
 * @param[in] filename The file to write the JSON representation of the properties object to.
 * @param[in] encodeFlags The flags to use when encoding the input properties.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ILLEGAL_ARGUMENT if the provided properties cannot be
 * encoded to a JSON representation and ENOMEM if there was not enough memory. CELIX_FILE_IO_EXCEPTION if the file
 * could not be opened or written to.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_save(const celix_properties_t* properties,
                                                        const char* filename,
                                                        int encodeFlags);

/**
 * @brief Save (encode) properties as a JSON representation to a string.
 *
 * For more information how a properties object is encoded to JSON, see the celix_properties_loadFromStream
 *
 * For a overview of the possible encode flags, see the CELIX_PROPERTIES_ENCODE_* flags documentation.
 * The default encoding style is a compact and flat JSON representation.
 *
 * @param[in] properties The properties object to encode.
 * @param[in] encodeFlags The flags to use when encoding the input properties.
 * @param[out] out The JSON string representation of the properties object. The caller is responsible for freeing the
 * returned string using free.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ILLEGAL_ARGUMENT if the provided properties cannot be
 * encoded to a JSON representation and ENOMEM if there was not enough memory.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_saveToString(const celix_properties_t* properties,
                                                                int encodeFlags,
                                                                char** out);

/**
 * @brief Flag to indicate that the decoding should fail if the input contains duplicate JSON keys.
 *
 * E.g. `{"key": "value", "key": "value2"}` is a duplicate key.
 *
 * If this flag is set, the decoding will fail if the input contains a duplicate key and if this flag is not set, the
 * decoding will not fail and the last entry will be used.
 */
#define CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES 0x01

/**
 * @brief Flag to indicate that the decoding should fail if the input contains entry that collide on property keys.
 *
 * E.g. `{"obj/key": "value", "obj": {"key": "value2"}}` is a collision.
 *
 * If this flag is set, the decoding will fail if the input contains a collision and if this flag is not set, the
 * decoding will not fail and the last entry will be used.
 */
#define CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS 0x02

/**
 * @brief Flag to indicate that the decoding should fail if the input contains null values.
 *
 * E.g. `{"key": null}` is a null value.
 *
 * Note arrays with null values are handled by the CELIX_PROPERTIES_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS flag.
 *
 * If this flag is set, the decoding will fail if the input contains a null value and if this flag is not set, the
 * decoding will not fail and the JSON null entry will be ignored.
 */
#define CELIX_PROPERTIES_DECODE_ERROR_ON_NULL_VALUES 0x04

/**
 * @brief Flag to indicate that the decoding should fail if the input contains empty arrays.
 *
 *
 * E.g. `{"key": []}` is an empty array.
 *
 * Note that empty arrays are valid in JSON, but not cannot be decoded to a valid properties array entry.
 *
 * If this flag is set, the decoding will fail if the input contains an empty array and if this flag is not set, the
 * decoding will not fail and the JSON empty array entry will be ignored.
 */
#define CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS 0x08

/**
 * @brief Flag to indicate that the decoding should fail if the input contains unsupported arrays.
 *
 * Unsupported arrays are arrays that contain JSON objects, multiple arrays, arrays with null values and
 * mixed arrays.
 * E.g.
 * - `{"key": [{"nested": "value"}]}` (array with JSON object)
 * - `{"key": [[1,2],[3,4]]}` (array with array)
 * - `{"key": [null,null]}` (array with null values)
 * - `{"key": ["value", 1]}` (mixed array)
 *
 * If this flag is set, the decoding will fail if the input contains an unsupported array and if this flag is not set,
 * the decoding will not fail and the unsupported array entries will be ignored.
 */
#define CELIX_PROPERTIES_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS 0x10

/**
 * @brief Flag to indicate that the decoding should fail if the input contains empty keys.
 *
 * E.g. `{"": "value"}` is an empty key.
 *
 * Note that empty keys are valid in JSON and valid in properties, but not always desired.
 *
 * If this flag is set, the decoding will fail if the input contains an empty key.
 */
#define CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_KEYS 0x20

/**
 * @brief Flag to indicate that the decoding should fail if the input contains any of the decode error flags.
 *
 * This flag is a combination of all decode error flags.
 */
#define CELIX_PROPERTIES_DECODE_STRICT                                                                                 \
    (CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES | CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS |                       \
     CELIX_PROPERTIES_DECODE_ERROR_ON_NULL_VALUES | CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS |                    \
     CELIX_PROPERTIES_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS | CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_KEYS)

/**
 * @brief Load properties from a stream.
 *
 * The stream is expected to be a valid readable stream and is not reset or closed by this function.
 * The content of the stream is expected to be in the format of a JSON object.
 *
 * For decoding a single JSON object is decoded to a properties object.
 *
 * The keys of the JSON object are used as
 * properties keys and the values of the JSON object are used as properties values. If there are nested
 * JSON objects, the keys are concatenated with a '/' separator (e.g. `{"key": {"nested": "value"}}` will be
 * decoded to a properties object with a single entry with key `key/nested` and (string) value `value`).
 *
 * Because properties keys are created by concatenating the JSON keys, there there could be collisions
 * (e.g. `{"obj/key": "value", "obj": {"key": "value2"}}`, two entries with the key `obj/key`. In this case
 * the last decoded JSON entry will be used.
 *
 * Properties entry types are determined by the JSON value type:
 * - JSON string values are decoded as string properties entries.
 * - JSON number values are decoded as long or double properties entries, depending on the value.
 * - JSON boolean values are decoded as boolean properties entries.
 * - jSON string values with a "version<" prefix and a ">" suffix are decoded as version properties entries (e.g.
 * "version<1.2.3>").
 * - JSON array values are decoded as array properties entries. The array can contain any of the above types, but mixed
 * arrays are not supported.
 * - JSON null values are ignored.
 *
 * For a overview of the possible decode flags, see the CELIX_PROPERTIES_DECODE_* flags documentation.
 *
 * @param[in] stream The input stream to parse.
 * @param[in] decodeFlags The flags to use when decoding the input string.
 * @param[out] out The properties object that will be created from the input string. The caller is responsible for
 * freeing the returned properties object using celix_properties_destroy.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ILLEGAL_ARGUMENT if the provided input cannot be
 * decoded to a properties object and ENOMEM if there was not enough memory. CELIX_FILE_IO_EXCEPTION if the file
 * could not be read.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_loadFromStream(FILE* stream,
                                                                  int decodeFlags,
                                                                  celix_properties_t** out);

/**
 * @brief Load properties from a file.
 *
 * @warning The name is temporary and will be renamed to celix_properties_load in the future (when
 * the current celix_properties_load is removed).
 *
 * The content of the filename file is expected to be in the format of a JSON object.
 * For what can and cannot be parsed, see celix_properties_loadFromStream documentation.
 *
 * For a overview of the possible decode flags, see the CELIX_PROPERTIES_DECODE_* flags documentation.
 *
 * If an error occurs, the error status is returned and a message is logged to celix_err.
 *
 * @param[in] filename The file to load the properties from.
 * @param[in] decodeFlags The flags to use when decoding the input string.
 * @param[out] out The properties object that will be created from the input string. The caller is responsible for
 * freeing the returned properties object using celix_properties_destroy.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ILLEGAL_ARGUMENT if the provided input cannot be
 * decoded to a properties object and ENOMEM if there was not enough memory. CELIX_FILE_IO_EXCEPTION if the file
 * could not be opened.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_load(const char* filename,
                                                         int decodeFlags,
                                                         celix_properties_t** out);

/**
 * @brief Load properties from a string.
 *
 * @warning The name is temporary and will be renamed to celix_properties_loadFromString in the future (when
 * the current celix_properties_loadFromString is removed).
 *
 * The input string is expected to be in the format of a JSON object.
 * For what can and cannot be parsed, see celix_properties_loadFromStream documentation.
 *
 * For a overview of the possible decode flags, see the CELIX_PROPERTIES_DECODE_* flags documentation.
 *
 * If an error occurs, the error status is returned and a message is logged to celix_err.
 *
 * @param[in] input The input string to parse.
 * @param[in] decodeFlags The flags to use when decoding the input string.
 * @param[out] out The properties object that will be created from the input string. The caller is responsible for
 * freeing the returned properties object using celix_properties_destroy.
 * @return CELIX_SUCCESS if the operation was successful, CELIX_ILLEGAL_ARGUMENT if the provided input cannot be
 * decoded to a properties object and ENOMEM if there was not enough memory.
 */
CELIX_UTILS_EXPORT celix_status_t celix_properties_loadFromString(const char* input,
                                                                   int decodeFlags,
                                                                   celix_properties_t** out);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_PROPERTIES_H_ */
