#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <dirent.h>
#include "sprpk.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "dstuff/file/path.h"

int compare_images(const void *a, const void *b)
{
    struct input_image_t *image_a = (struct input_image_t *)a;
    struct input_image_t *image_b = (struct input_image_t *)b;
    uint32_t a_pixels = image_a->width * image_a->height;
    uint32_t b_pixels = image_b->width * image_b->height;

    return b_pixels - a_pixels;
}

int main(int argc, char *argv[])
{
    uint32_t arg_index = 1;
    struct input_image_t *images = NULL;
    struct input_image_t *image;
    struct sprite_sheet_t sprite_sheet;
    uint32_t image_count = 0;
    char *output_name = "output.sprpk";
    int width;
    int height;
    int comp;
//    char file_name[PATH_MAX];
    uint32_t color_free_regions = 0;
//    char *file_ext;
//    char path[512];
    char *path;
    char *input_path;
    DIR *dir;
//    DIR *probe;
    struct dirent *entry;
    if(argc > 1)
    {
        while(arg_index < argc)
        {
            if(!strcmp(argv[arg_index], "-f"))
            {
                arg_index++;

                if(arg_index == argc)
                {
                    printf("error: expecting file name after '-f'\n");
                    return 0;
                }

                images = realloc(images, sizeof(struct input_image_t) * (image_count + 1));
                image = images + image_count;
                image->file_name = strdup(argv[arg_index]);

                if(!stbi_info(image->file_name, &width, &height, &comp))
                {
                    printf("error: couldn't open file '%s'\n", image->file_name);
                    return 0;
                }
                image->width = (uint32_t)width;
                image->height = (uint32_t)height;
                arg_index++;
                image_count++;
            }
            else if(!strcmp(argv[arg_index], "-d"))
            {
                arg_index++;
                input_path = argv[arg_index];
                arg_index++;
                dir = opendir(input_path);

                while((entry = readdir(dir)))
                {
                    path = append_path(format_path(input_path), entry->d_name);
                    if(!is_dir(path))
                    {
                        images = realloc(images, sizeof(struct input_image_t) * (image_count + 1));
                        image = images + image_count;
                        image->file_name = strdup(path);
                        if(!stbi_info(image->file_name, &width, &height, &comp))
                        {
                            printf("error: couldn't open file '%s'\n", image->file_name);
                            return 0;
                        }
                        image->width = (uint32_t)width;
                        image->height = (uint32_t)height;
                        image_count++;
                    }
                }
            }
            else if(!strcmp(argv[arg_index], "-o"))
            {
                arg_index++;
                if(arg_index == argc)
                {
                    printf("error: expecting output name after '-o'\n");
                    return 0;
                }
                output_name = argv[arg_index];
                arg_index++;
            }
            else if(!strcmp(argv[arg_index], "-h"))
            {
                return 0;
            }
            else if(!strcmp(argv[arg_index], "-cf"))
            {
                color_free_regions = 1;
            }
            else
            {
                printf("error: unknown option '%s'\n", argv[arg_index]);
                return 0;
            }
        }

        if(images)
        {
            printf("sorting...\n");
            qsort(images, image_count, sizeof(struct input_image_t), compare_images);
            printf("fitting...\n");
            fit_images(images, image_count, &sprite_sheet);
            printf("building entries...\n");
            build_entries(&sprite_sheet);
            printf("outputting...\n");
            write_sprite_sheet(output_name, &sprite_sheet, color_free_regions);
            printf("done");
            return 0;
        }
    }

    printf("error: no input given\n");
    return 0;
}
