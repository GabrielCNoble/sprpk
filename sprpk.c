#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <dirent.h>
#include "sprpk.h"
#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"
#include "../dstuff/file/path.h"

#define MAX(a, b) (a>b?a:b)
#define MIN(a, b) (a<b?a:b)
#define CLAMP(min, value, max) MIN(max, MAX(value, min))
#define RATIO(a, b) (a>=b?(float)b/(float)a:(float)a/(float)b)

int compare_images(struct input_image_t *a, struct input_image_t *b)
{
    uint32_t a_pixels = a->width * a->height;
    uint32_t b_pixels = b->width * b->height;

    return b_pixels - a_pixels;
//    if(a_pixels > b_pixels)
//    {
//        return -1;
//    }
//    else if(a_pixels == b_pixels)
//    {
//        if(a->width > b->width)
//        {
//            return -1;
//        }
//        else
//        {
//            return 0;
//        }
//    }
//
//    return 1;
}

void fit_image(struct input_image_t *image, struct sprite_sheet_t *sprite_sheet)
{
    struct region_t *region;
//    struct region_t *best_region;
    struct region_t *used_region;
    struct region_t *new_region;
//    uint32_t width;
//    uint32_t height;
    uint32_t new_width;
    uint32_t new_height;
//    uint32_t new_pixel_count;
    uint32_t cur_pixel_count;
//    struct region_t new_regions[2];
    uint32_t best_fit = 0;
    uint32_t pixel_count_increase;
    uint32_t best_pixel_count_increase = 0xffffffff;
//    int32_t increase;

//    float cur_ratio;
    float new_ratio;
    float best_ratio;

    if(sprite_sheet->used_region_count)
    {
        cur_pixel_count = sprite_sheet->width * sprite_sheet->height;

        for(uint32_t region_index = 0; region_index < sprite_sheet->free_region_count; region_index++)
        {
            /* first, look for a region that doesn't increase the current size of
            the sprite sheet. Those types of region is formed every time an image
            that increases the size of the current sprite sheet gets fit */
            region = sprite_sheet->free_regions + region_index;

            if(region->width >= image->width && region->height >= image->height)
            {
                new_width = region->offset_x + image->width;
                new_width = MAX(new_width, sprite_sheet->width);
                new_height = region->offset_y + image->height;
                new_height = MAX(new_height, sprite_sheet->height);

                pixel_count_increase = new_width * new_height - cur_pixel_count;

                if(pixel_count_increase < best_pixel_count_increase)
                {
                    best_pixel_count_increase = pixel_count_increase;
                    best_fit = region_index;
                }
            }
        }

        if(best_pixel_count_increase > 0)
        {
            /* didn't find a region that doesn't increase the current size
            of the sprite sheet, so now look for one that brings the ratio
            of the sides closer to one, or takes it less away from one. This
            means, look for a region that when fit makes the sprite sheet
            become more squareish. */

            best_ratio = -FLT_MAX;

            for(uint32_t region_index = 0; region_index < sprite_sheet->free_region_count; region_index++)
            {
                region = sprite_sheet->free_regions + region_index;

                if(region->width >= image->width && region->height >= image->height)
                {
                    /* new size of the sprite sheet if this region were to
                    be used */
                    new_width = region->offset_x + image->width;
                    new_width = MAX(new_width, sprite_sheet->width);
                    new_height = region->offset_y + image->height;
                    new_height = MAX(new_height, sprite_sheet->height);


                    new_ratio = RATIO(new_width, new_height);

                    if(new_ratio > best_ratio)
                    {
                        best_fit = region_index;
                        best_ratio = new_ratio;
                    }
                }
            }
        }
    }

    new_region = sprite_sheet->free_regions + best_fit;
    used_region = sprite_sheet->used_regions + sprite_sheet->used_region_count;
    sprite_sheet->used_region_count++;
    memcpy(used_region, new_region, sizeof(struct region_t));

    new_region->offset_x = used_region->offset_x + image->width;
    new_region->offset_y = used_region->offset_y;
    new_region->width = used_region->width - image->width;
    new_region->height = image->height;
    new_region->image = NULL;

    new_region = sprite_sheet->free_regions + sprite_sheet->free_region_count;
    sprite_sheet->free_region_count++;

    new_region->offset_x = used_region->offset_x;
    new_region->offset_y = used_region->offset_y + image->height;
    new_region->width = used_region->width;
    new_region->height = used_region->height - image->height;
    new_region->image = NULL;

    used_region->image = image;
    used_region->width = image->width;
    used_region->height = image->height;

    new_width = used_region->width + used_region->offset_x;
    new_height = used_region->height + used_region->offset_y;

    if(new_width> sprite_sheet->width)
    {
        sprite_sheet->width = new_width;
    }

    if(new_height > sprite_sheet->height)
    {
        sprite_sheet->height = new_height;
    }
}

void fit_images(struct input_image_t *images, uint32_t image_count, struct sprite_sheet_t *sprite_sheet)
{
    sprite_sheet->width = 0;
    sprite_sheet->height = 0;
    sprite_sheet->free_region_count = 1;
    sprite_sheet->free_regions = calloc(sizeof(struct region_t), image_count * 2);
    sprite_sheet->used_region_count = 0;
    sprite_sheet->used_regions = calloc(sizeof(struct region_t), image_count);

    sprite_sheet->free_regions->width = 0xffffffff;
    sprite_sheet->free_regions->height = 0xffffffff;

    for(uint32_t image_index = 0; image_index < image_count; image_index++)
    {
       fit_image(images + image_index, sprite_sheet);
    }
}

void write_sprite_sheet(char *output_name, struct sprite_sheet_t *sprite_sheet)
{
    uint32_t output_row_pitch;
    struct region_t *region;
    char *output_pixels;

    output_row_pitch = sprite_sheet->width * 4;
    output_pixels = calloc(sprite_sheet->height, output_row_pitch);

    for(uint32_t region_index = 0; region_index < sprite_sheet->used_region_count; region_index++)
    {
        region = sprite_sheet->used_regions + region_index;

        uint32_t width;
        uint32_t height;
        uint32_t comps;
        char *pixels = stbi_load(region->image->file_name, &width, &height, &comps, STBI_rgb_alpha);
        uint32_t row_pitch = width * STBI_rgb_alpha;

        for(uint32_t y = 0; y < region->height; y++)
        {
            uint32_t output_pixel_offset = (region->offset_y + y) * output_row_pitch + region->offset_x * STBI_rgb_alpha;
            uint32_t pixel_offset = y * width * STBI_rgb_alpha;
            memcpy(output_pixels + output_pixel_offset, pixels + pixel_offset, row_pitch);
        }

        free(pixels);
    }

    stbi_write_png(output_name, sprite_sheet->width, sprite_sheet->height, 4, output_pixels, output_row_pitch);
}

int main(int argc, char *argv[])
{
    uint32_t arg_index = 1;
    struct input_image_t *images = NULL;
    struct input_image_t *image;
    struct sprite_sheet_t sprite_sheet;
    uint32_t image_count = 0;
    char *output_name = "output.sprpk";
    uint32_t width;
    uint32_t height;
    uint32_t comp;
//    char path[512];
    char *path;
    char *input_path;
    DIR *dir;
    DIR *probe;
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
                image->file_name = argv[arg_index];
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

                while(entry = readdir(dir))
                {
                    path = append_path_segment(input_path, entry->d_name);
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
            printf("outputting...\n");
            write_sprite_sheet(output_name, &sprite_sheet);
            printf("done");
            return 0;
        }
    }

    printf("error: no input given\n");
    return 0;
}
