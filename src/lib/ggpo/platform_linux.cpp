/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "platform_linux.h"

struct timespec start = { 0 };


// GetEnvironmentVariable equivalent for Unix
static size_t GetEnvironmentVariable(const char *name, char *buffer, size_t bufferSize) {
    if (name == NULL || buffer == NULL || bufferSize == 0) {
        return 0; // Invalid parameters
    }

    const char *value = getenv(name);
    if (value == NULL) {
        return 0; // Environment variable not found
    }

    size_t requiredSize = strlen(value) + 1; // +1 for null terminator

    if (requiredSize > bufferSize) {
        // Buffer too small, return required size
        return requiredSize;
    }

    strncpy(buffer, value, bufferSize);
    buffer[bufferSize - 1] = '\0'; // Ensure null termination

    return requiredSize - 1; // Return length of the string (excluding null terminator)
}


uint Platform::GetCurrentTimeMS() {
    if (start.tv_sec == 0 && start.tv_nsec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        return 0;
    }
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    return ((current.tv_sec - start.tv_sec) * 1000) +
           ((current.tv_nsec  - start.tv_nsec ) / 1000000);
}

int
Platform::GetConfigInt(const char* name)
{
   char buf[1024];
   if (GetEnvironmentVariable(name, buf, ARRAY_SIZE(buf)) == 0) {
      return 0;
   }
   return atoi(buf);
}

bool Platform::GetConfigBool(const char* name)
{
   char buf[1024];
   if (GetEnvironmentVariable(name, buf, ARRAY_SIZE(buf)) == 0) {
      return false;
   }
   return atoi(buf) != 0 || strcasecmp(buf, "true") == 0;
}
