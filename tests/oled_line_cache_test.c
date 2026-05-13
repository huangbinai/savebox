#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define OLED_LINE_COUNT 8U
#define OLED_LINE_CHAR_CAPACITY 16U

static bool oled_line_changed(char cache[OLED_LINE_COUNT][OLED_LINE_CHAR_CAPACITY + 1U],
                              unsigned int line,
                              const char *text,
                              char padded[OLED_LINE_CHAR_CAPACITY + 1U])
{
    size_t src_len = 0U;

    memset(padded, ' ', OLED_LINE_CHAR_CAPACITY);
    padded[OLED_LINE_CHAR_CAPACITY] = '\0';

    if (text != NULL) {
        src_len = strlen(text);
        if (src_len > OLED_LINE_CHAR_CAPACITY) {
            src_len = OLED_LINE_CHAR_CAPACITY;
        }
        memcpy(padded, text, src_len);
    }

    if (strncmp(cache[line], padded, OLED_LINE_CHAR_CAPACITY) == 0) {
        return false;
    }

    memcpy(cache[line], padded, OLED_LINE_CHAR_CAPACITY + 1U);
    return true;
}

int main(void)
{
    char cache[OLED_LINE_COUNT][OLED_LINE_CHAR_CAPACITY + 1U] = {{0}};
    char padded[OLED_LINE_CHAR_CAPACITY + 1U] = {0};

    assert(oled_line_changed(cache, 0U, "LOCK=LOCKED", padded));
    assert(strcmp(cache[0], "LOCK=LOCKED     ") == 0);

    assert(!oled_line_changed(cache, 0U, "LOCK=LOCKED", padded));

    assert(oled_line_changed(cache, 0U, "LOCK=NA", padded));
    assert(strcmp(cache[0], "LOCK=NA         ") == 0);

    assert(oled_line_changed(cache, 1U, NULL, padded));
    assert(strcmp(cache[1], "                ") == 0);

    puts("oled_line_cache_test passed");
    return 0;
}
