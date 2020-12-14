#ifndef NURU_H
#define NURU_H

#include <stdio.h>      // size_t, fopen()
#include <stdint.h>     // uint8_t, uint16_t
#include <string.h>     // strcmp()

#ifndef NURU_SCOPE
#  define NURU_SCOPE
#endif

// 
// API
// 

#define NURU_IMG_SIGNATURE "NURUIMG"
#define NURU_PAL_SIGNATURE "NURUPAL"

typedef enum nuru_glyph_mode
{
	NURU_GLYPH_MODE_NONE    = 0,  // spaces only (needs a color mode)
	NURU_GLYPH_MODE_PALETTE = 1,  // using a 255 char palette file
	NURU_GLYPH_MODE_UNICODE = 2   // directly using 16 bit code points
}
nuru_glyph_mode_e;

typedef enum nuru_color_mode
{
	NURU_COLOR_MODE_NONE = 0,     // no colors, monochrome
	NURU_COLOR_MODE_4BIT = 1,     // 4-bit ANSI colors
	NURU_COLOR_MODE_8BIT = 2      // 8-bit ANSI colors
}
nuru_color_mode_e;

typedef enum nuru_mdata_mode
{
	NURU_MDATA_MODE_NONE  = 0,    // no meta data
	NURU_MDATA_MODE_1BYTE = 1,    // 1 byte of meta data per cell
	NURU_MDATA_MODE_2BYTE = 2     // 2 byte of meta data per cell
}
nuru_mdata_mode_e;

typedef struct nuru_cell
{
	uint16_t ch;
	uint8_t  fg;
	uint8_t  bg;
	uint16_t md;
}
nuru_cell_s;

typedef struct nuru_img
{
	char     signature[8];
	uint8_t  version;
	uint8_t  glyph_mode;
	uint8_t  color_mode;
	uint8_t  mdata_mode;
	uint16_t cols;
	uint16_t rows;
	uint8_t  fg;
	uint8_t  bg;
	char     palette[8];
	char     comment[8];

	nuru_cell_s *cells;
	size_t num_cells;
}
nuru_img_s;

typedef struct nuru_pal
{
	char    signature[8];
	uint8_t version;
	uint8_t space;
	char    name[8];

	uint16_t codepoints[256];
}
nuru_pal_s;

NURU_SCOPE int nuru_img_load(nuru_img_s *img, const char *file);
NURU_SCOPE int nuru_img_free(nuru_img_s *img);
NURU_SCOPE int nuru_pal_load(nuru_pal_s *pal, const char *file);

NURU_SCOPE nuru_cell_s* nuru_get_cell(nuru_img_s *img, uint16_t col, uint16_t row);

// 
// IMPLEMENTATION
// 

#ifdef NURU_IMPLEMENTATION

NURU_SCOPE int
nuru_img_load(nuru_img_s *img, const char *file)
{
	FILE *fp;
	fp = fopen(file, "rb");
	if (fp == NULL)
	{
		return -1;
	}

	// header: signature
	fread(&img->signature, 7, 1, fp);
	img->signature[7] = 0;
	if (strcmp(img->signature, NURU_IMG_SIGNATURE) != 0)
	{
		fclose(fp);
		return -2;
	}

	// header: version
	fread(&img->version, 1, 1, fp);

	// header: modes
	fread(&img->color_mode, 1, 1, fp);
	fread(&img->glyph_mode, 1, 1, fp);
	fread(&img->mdata_mode,  1, 1, fp);

	// header: image size (TODO this assumes the system is little endian)
	uint16_t buf = 0;
	fread(&buf, 2, 1, fp);
	img->cols = (0xFF00 & (buf << 8)) | (0x00FF & buf >> 8);
	fread(&buf, 2, 1, fp);
	img->rows = (0xFF00 & (buf << 8)) | (0x00FF & buf >> 8);

	// header: default colors / key colors
	fread(&img->fg, 1, 1, fp);
	fread(&img->bg, 1, 1, fp);
	
	// header: palette name
	fread(&img->palette, 7, 1, fp);
	fread(&img->comment, 7, 1, fp);
	img->palette[7] = 0;
	img->comment[7] = 0;

	// header: reserved byte
	fread(&buf, 1, 1, fp);

	// payload
	img->num_cells = img->cols * img->rows;
	img->cells = malloc(sizeof(nuru_cell_s) * img->num_cells);
	if (img->cells == NULL)
	{
		fclose(fp);
		return -3;
	}

	// TODO take different modes into account
	for (size_t c = 0; c < img->num_cells; ++c)
	{
		uint8_t ch = 0;
		fread(&ch, 1, 1, fp);
		img->cells[c].ch |= 0x00FF & ch;
		fread(&img->cells[c].fg, 1, 1, fp);
		fread(&img->cells[c].bg, 1, 1, fp);
	}

	fclose(fp);
	return img->num_cells;
}

NURU_SCOPE nuru_cell_s*
nuru_get_cell(nuru_img_s *img, uint16_t col, uint16_t row)
{
	size_t idx = (row * img->cols) + col;
	if (idx >= img->num_cells)
	{
		return NULL;
	}
	return &img->cells[idx];
}

NURU_SCOPE int
nuru_img_free(nuru_img_s *img)
{
	if (!img)
	{
		return -1;
	}
	if (!img->cells)
	{
		return -1;
	}

	free(img->cells);
	img->cells = NULL;
	return 0;
}

NURU_SCOPE int
nuru_pal_load(nuru_pal_s *pal, const char *file)
{
	FILE *fp;
	fp = fopen(file, "rb");
	if (fp == NULL)
	{
		return -1;
	}

	// header: signature
	fread(&pal->signature, 7, 1, fp);
	pal->signature[7] = 0;
	if (strcmp(pal->signature, NURU_PAL_SIGNATURE) != 0)
	{
		return -2;
	}

	// header: version
	fread(&pal->version, 1, 1, fp);
	
	// header: default fill char (usually space)
	fread(&pal->space, 1, 1, fp);

	// header: palette name
	fread(&pal->name, 7, 1, fp);
	pal->name[7] = 0;

	for (int i = 0; i < 256; ++i)
	{
		uint16_t buf = 0;
		fread(&buf, 2, 1, fp);
		pal->codepoints[i] = (0xFF00 & (buf << 8)) | (0x00FF & buf >> 8);
	}

	return 0;
}

#endif /* NURU_IMPLEMENTATION */
#endif /* NURU_H */
