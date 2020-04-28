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

#define MAX(a, b) (a>b?a:b)
#define MIN(a, b) (a<b?a:b)
#define CLAMP(min, value, max) MIN(max, MAX(value, min))
#define RATIO(a, b) (a>=b?(float)b/(float)a:(float)a/(float)b)

int compare_image_names(const void *a, const void *b)
{
    struct region_t *region_a = (struct region_t *)a;
    struct region_t *region_b = (struct region_t *)b;
    static char image_a_filename[PATH_MAX];
    static char image_b_filename[PATH_MAX];
    int32_t image_a_index;
    int32_t image_b_index;
    int32_t compare;

    image_a_index = get_index_from_path(region_a->image->file_name);
    image_b_index = get_index_from_path(region_b->image->file_name);

    strcpy(image_a_filename, strip_decorations_from_path(region_a->image->file_name));
    strcpy(image_b_filename, strip_decorations_from_path(region_b->image->file_name));

    compare = strcmp(image_a_filename, image_b_filename);
    if(!compare && image_a_index >= 0 && image_b_index >= 0)
    {
        return image_b_index - image_a_index;
    }

    return compare;
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

            if(region->frame.width >= image->width && region->frame.height >= image->height)
            {
                new_width = region->frame.offset_x + image->width;
                new_width = MAX(new_width, sprite_sheet->width);
                new_height = region->frame.offset_y + image->height;
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

                if(region->frame.width >= image->width && region->frame.height >= image->height)
                {
                    /* new size of the sprite sheet if this region were to
                    be used */
                    new_width = region->frame.offset_x + image->width;
                    new_width = MAX(new_width, sprite_sheet->width);
                    new_height = region->frame.offset_y + image->height;
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

    new_region->frame.offset_x = used_region->frame.offset_x + image->width;
    new_region->frame.offset_y = used_region->frame.offset_y;
    new_region->frame.width = used_region->frame.width - image->width;
    new_region->frame.height = image->height;
    new_region->image = NULL;
    if(!(new_region->frame.width && new_region->frame.height))
    {
        /* if this region has 0 area, don't create it */
        if(best_fit + 1 < sprite_sheet->free_region_count)
        {
            memcpy(new_region, sprite_sheet->free_regions + sprite_sheet->free_region_count - 1, sizeof(struct region_t));
        }
        sprite_sheet->free_region_count--;
    }

    new_region = sprite_sheet->free_regions + sprite_sheet->free_region_count;
    new_region->frame.offset_x = used_region->frame.offset_x;
    new_region->frame.offset_y = used_region->frame.offset_y + image->height;
    new_region->frame.width = used_region->frame.width;
    new_region->frame.height = used_region->frame.height - image->height;
    new_region->image = NULL;
    if(new_region->frame.width && new_region->frame.height)
    {
        /* if this region has area greater than , create it */
        sprite_sheet->free_region_count++;
    }

    used_region->image = image;
    used_region->frame.width = image->width;
    used_region->frame.height = image->height;

    new_width = used_region->frame.width + used_region->frame.offset_x;
    new_height = used_region->frame.height + used_region->frame.offset_y;

    if(new_width > sprite_sheet->width)
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

    sprite_sheet->free_regions->frame.width = 0xffffffff;
    sprite_sheet->free_regions->frame.height = 0xffffffff;

    for(uint32_t image_index = 0; image_index < image_count; image_index++)
    {
       fit_image(images + image_index, sprite_sheet);
    }
}

void build_entries(struct sprite_sheet_t *sprite_sheet)
{
    char *entries_memory;
    uint32_t entries_memory_size;
    struct sprpk_header_t *header;
    struct entry_t *entry;
//    struct frame_t *frame;
    struct region_t *region;
    int32_t frame_index;
    char *entry_name;
    qsort(sprite_sheet->used_regions, sprite_sheet->used_region_count, sizeof(struct region_t), compare_image_names);
    entries_memory_size = sizeof(struct sprpk_header_t) +
                    (sizeof(struct entry_t) + sizeof(struct frame_t)) * sprite_sheet->used_region_count;
    entries_memory = calloc(1, entries_memory_size);
    header = (struct sprpk_header_t *)entries_memory;
    entries_memory += sizeof(struct sprpk_header_t);
    strcpy(header->tag, sprpk_header_tag);

    entry_name = strip_decorations_from_path(strip_path_from_file_name(sprite_sheet->used_regions[0].image->file_name));
    for(uint32_t region_index = 0; region_index < sprite_sheet->used_region_count;)
    {
        region = sprite_sheet->used_regions + region_index;

        entry = (struct entry_t *)entries_memory;
        entries_memory += sizeof(struct entry_t);
        header->entry_count++;

        strcpy(entry->name, entry_name);
        frame_index = get_index_from_path(region->image->file_name);
        if(frame_index < 0)
        {
            /* this image has no index in its name,
            so it'll be alone in this entry */
            entry->frames[0] = region->frame;
            header->frame_count++;
            if(region_index + 1 < sprite_sheet->used_region_count)
            {
                entry_name = strip_decorations_from_path(strip_path_from_file_name(region[1].image->file_name));
            }
            continue;
        }
        else
        {
            entry->frame_count = frame_index + 1;
            header->frame_count += entry->frame_count;
            while(region_index < sprite_sheet->used_region_count)
            {
                region = sprite_sheet->used_regions + region_index;
                frame_index = get_index_from_path(region->image->file_name);
                entry_name = strip_decorations_from_path(strip_path_from_file_name(region->image->file_name));
                if(strcmp(entry->name, entry_name))
                {
                    break;
                }
                entry->frames[frame_index] = region->frame;
                region_index++;
            }

            /* struct entry_t already contains a struct frame_t itself, so we subtract
            one away here to not waste space */
            entries_memory += sizeof(struct frame_t) * (entry->frame_count - 1);
        }
    }

    header->data_offset = entries_memory - (char *)header;
    sprite_sheet->header = header;
    sprite_sheet->header_size = header->data_offset;
}

void write_sprite_sheet_pixels(void *context, void *data, int size)
{
    FILE *file = (FILE *)context;
    fwrite(data, 1, size, file);
}

void write_sprite_sheet(char *output_name, struct sprite_sheet_t *sprite_sheet, uint32_t color_free_regions)
{
    uint32_t output_row_pitch;
    struct region_t *region;
    uint32_t *output_pixels;
    FILE *file;

    output_row_pitch = sprite_sheet->width * STBI_rgb_alpha;
    output_pixels = calloc(sprite_sheet->height, output_row_pitch);

    for(uint32_t region_index = 0; region_index < sprite_sheet->used_region_count; region_index++)
    {
        region = sprite_sheet->used_regions + region_index;

        int width;
        int height;
        int comps;
        unsigned char *pixels = stbi_load(region->image->file_name, &width, &height, &comps, STBI_rgb_alpha);
        uint32_t row_pitch = width * STBI_rgb_alpha;

        for(uint32_t y = 0; y < region->frame.height; y++)
        {
            uint32_t output_pixel_offset = (region->frame.offset_y + y) * sprite_sheet->width + region->frame.offset_x;
            uint32_t pixel_offset = y * width * STBI_rgb_alpha;
            memcpy(output_pixels + output_pixel_offset, pixels + pixel_offset, row_pitch);
        }

        free(pixels);
    }

    if(color_free_regions)
    {
        for(uint32_t region_index = 0; region_index < sprite_sheet->free_region_count; region_index++)
        {
            region = sprite_sheet->free_regions + region_index;

            if(region->frame.offset_y + region->frame.height > sprite_sheet->height)
            {
                region->frame.height = sprite_sheet->height - region->frame.offset_y;
            }

            if(region->frame.offset_x + region->frame.width > sprite_sheet->width)
            {
                region->frame.width = sprite_sheet->width - region->frame.offset_x;
            }

            uint32_t pixel_color = ((0xffffffff * rand()) + region_index) | 0xff000000;
            for(uint32_t y = 0; y < region->frame.height; y++)
            {
                uint32_t output_pixel_offset = (region->frame.offset_y + y) * sprite_sheet->width + region->frame.offset_x;
//                uint32_t row_pitch = region->frame.width;

                for(uint32_t x = 0; x < region->frame.width; x++)
                {
                    *(output_pixels + output_pixel_offset + x) = pixel_color;
                }
            }
        }
    }

    file = fopen(output_name, "wb");
    fwrite(sprite_sheet->header, 1, sprite_sheet->header_size, file);
    stbi_write_png_to_func(write_sprite_sheet_pixels, file, sprite_sheet->width, sprite_sheet->height, 4, output_pixels, output_row_pitch);
    fclose(file);
}
