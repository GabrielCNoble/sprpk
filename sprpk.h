#ifndef SPRPK_H
#define SPRPK_H

#include <stdint.h>

struct input_image_t
{
    uint32_t width;
    uint32_t height;
    char *file_name;
};

struct frame_t
{
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t width;
    uint32_t height;
};

struct region_t
{
    struct frame_t frame;
    struct input_image_t *image;
};

static char sprpk_header_tag[] = "sprpk_header";

struct sprpk_header_t
{
    char tag[(sizeof(sprpk_header_tag) + 3) & (~3)];
    uint32_t entry_count;
    uint32_t frame_count;
    uint32_t data_offset;
};

struct entry_t
{
    char name[64];
    uint32_t frame_count;
    struct frame_t frames[1];
};

struct sprite_sheet_t
{
    uint32_t width;
    uint32_t height;
    uint32_t free_region_count;
    struct region_t *free_regions;
    uint32_t used_region_count;
    struct region_t *used_regions;
    void *header;
    uint32_t header_size;
};

void fit_image(struct input_image_t *image, struct sprite_sheet_t *sprite_sheet);

void fit_images(struct input_image_t *images, uint32_t image_count, struct sprite_sheet_t *sprite_sheet);

void build_entries(struct sprite_sheet_t *sprite_sheet);

void write_sprite_sheet(char *output_name, struct sprite_sheet_t *sprite_sheet, uint32_t color_free_regions);


#endif // SPRPK_H
