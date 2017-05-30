/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <PL/platform.h>
#include <PL/platform_filesystem.h>

/*  MAD/MTD Format Specification    */
/* The MAD/MTD format is the package format used by
 * Hogs of War to store and index content used by
 * the game.
 *
 * Files within these packages are expected to be in
 * a specific order, as both the game and other assets
 * within the game rely on this order so that they, for
 * example, will know which textures to load in / use.
 *
 * Because of this, any package that's recreated will need
 * to be done so in a way that preserves the original file
 * order.
 *
 * Thanks to solemnwarning for his help on this one!
 */

typedef struct __attribute__((packed)) MADIndex {
    char file[12];

    int32_t padding0;

    uint32_t offset;
    uint32_t length;
} MADIndex;

int main(int argc, char **argv) {
    const char *arg;
    if((arg = plGetCommandLineArgument("extract")) && (arg[0] != '\0')) {
        if(!plFileExists(arg)) {
            printf("Failed to find %s!\n", arg);
            return -1;
        }

        FILE *file = fopen(arg, "rb");
        if(!file) {
            printf("Failed to load %s!\n", arg);
            return -1;
        }

        char package_name[PL_SYSTEM_MAX_PATH] = { 0 };
        plStripExtension(package_name, plGetFileName(arg));
        plLowerCasePath(package_name);

        char package_extension[PL_SYSTEM_MAX_PATH] = { 0 };
        snprintf(package_extension, sizeof(package_extension), "%s", plGetFileExtension(arg));
        plLowerCasePath(package_extension);

        printf("Extracting %s...\n", plGetFileName(arg));

        unsigned int lowest_offset = UINT32_MAX;
        unsigned int cur_index = 0;
        long position;
        do {
            cur_index++;

            MADIndex index;
            if(fread(&index, sizeof(MADIndex), 1, file) != 1) {
                printf("Unexpected index size for index %d, in %s!\n", cur_index, plGetFileName(arg));
                return -1;
            }

            position = ftell(file);
            if(lowest_offset > index.offset) {
                lowest_offset = index.offset;
            }

            const char *ext = plGetFileExtension(index.file);
            if(!ext || ext[0] == '\0') {
                printf("Invalid extension for %s, skipping!\n", index.file);
                continue;
            }

            char file_path[PL_SYSTEM_MAX_PATH];
            snprintf(file_path, sizeof(file_path), "./extract/%s_%s", package_name, package_extension);
            if(!plCreateDirectory(file_path)) {
                printf("Failed to create directory at %s!\n", file_path);
                return -1;
            }
            snprintf(file_path, sizeof(file_path), "./extract/%s_%s/%s", package_name, package_extension, index.file);

            plLowerCasePath(file_path);

            fseek(file, index.offset, SEEK_SET);
            uint8_t *data = calloc(index.length, sizeof(uint8_t));
            if(fread(data, sizeof(uint8_t), index.length, file) == index.length) {
                printf("Writing %s...\n", file_path);

                FILE *out = fopen(file_path, "wb");
                if(!out || fwrite(data, sizeof(uint8_t), index.length, out) != index.length) {
                    printf("Failed to write %s!\n", file_path);
                    return -1;
                }
                fclose(out);
            }
            free(data);

            fseek(file, position, SEEK_SET);
        } while(position < lowest_offset);

        fclose(file);
    }

    return 0;
}