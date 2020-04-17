#ifndef SPRPK_H
#define SPRPK_H

#include <stdint.h>

struct input_image_t
{
    uint32_t width;
    uint32_t height;
    char *file_name;
};

struct region_t
{
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t width;
    uint32_t height;
    struct input_image_t *image;
};

struct sprite_sheet_t
{
    uint32_t width;
    uint32_t height;
    uint32_t free_region_count;
    struct region_t *free_regions;
    uint32_t used_region_count;
    struct region_t *used_regions;
};

void fit_image(struct input_image_t *image, struct sprite_sheet_t *sprite_sheet);

void fit_images(struct input_image_t *images, uint32_t image_count, struct sprite_sheet_t *sprite_sheet);

void write_sprite_sheet(char *output_name, struct sprite_sheet_t *sprite_sheet);


#endif // SPRPK_H
