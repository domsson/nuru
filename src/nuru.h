#ifndef NURU_H
#define NURU_H

#include <stdio.h>      // size_t, fopen()
#include <stdint.h>     // uint8_t, uint16_t
#include <string.h>     // strcmp()
#include <arpa/inet.h>  // ntohs()

#define NURU_NAME "nuru"
#define NURU_URL  "https://github.com/domsson/nuru"

#define NURU_VER_MAJOR 0
#define NURU_VER_MINOR 0
#define NURU_VER_PATCH 1

#ifndef NURU_SCOPE
#	define NURU_SCOPE
#endif

// 
// API
// 

#define NURU_IMG_SIGNATURE "NURUIMG"
#define NURU_PAL_SIGNATURE "NURUPAL"

#define NURU_SPACE ' '

#define NURU_STR_LEN 8
#define NURU_STR_LEN_RAW 7
#define NURU_PAL_SIZE 256

#define NURU_ERR_NONE        0
#define NURU_ERR_OTHER      -1
#define NURU_ERR_MEMORY     -2
#define NURU_ERR_FILE_OPEN  -3
#define NURU_ERR_FILE_READ  -4
#define NURU_ERR_FILE_TYPE  -5
#define NURU_ERR_FILE_MODE  -6

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
	char     signature[NURU_STR_LEN];
	uint8_t  version;
	uint8_t  glyph_mode;
	uint8_t  color_mode;
	uint8_t  mdata_mode;
	uint16_t cols;
	uint16_t rows;
	uint8_t  fg;
	uint8_t  bg;
	char     palette[NURU_STR_LEN];
	char     comment[NURU_STR_LEN];
	uint8_t  reserved;

	nuru_cell_s *cells;
	size_t num_cells;
}
nuru_img_s;

typedef struct nuru_pal
{
	char    signature[NURU_STR_LEN];
	uint8_t version;
	uint8_t space;
	char    name[NURU_STR_LEN];

	uint16_t codepoints[NURU_PAL_SIZE];
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
nuru_read_int(void* buf, uint8_t size, FILE *fp)
{
	uint16_t tmp = 0;
	if (size > 2)
	{
		return NURU_ERR_OTHER;
	}

	if (fread(&tmp, 1, size, fp) != size)
	{
		return NURU_ERR_FILE_READ;
	}

	if (size == 1)
	{
		*(uint8_t*)(buf) = tmp;
		return 0;
	}
	else
	{
		*(uint16_t*)(buf) = ntohs(tmp);
		return 0;
	}
}

NURU_SCOPE int
nuru_read_col(uint8_t* fg, uint8_t* bg, FILE* fp)
{
	uint8_t tmp = 0;
	if (fread(&tmp, 1, 2, fp) != 2)
	{
		return NURU_ERR_FILE_READ;
	}

	*fg = 0xF0 & tmp;
	*bg = 0x0F & tmp;
	return 0;
}

NURU_SCOPE int
nuru_read_str(char* buf, size_t len, FILE* fp)
{
	if (fread(buf, 1, len, fp) != len)
	{
		return NURU_ERR_FILE_READ;
	}

	buf[len] = 0;
	return 0;
}

NURU_SCOPE int
nuru_img_load(nuru_img_s* img, const char* file)
{
	// open file
	FILE* fp = fopen(file, "rb");
	if (fp == NULL)
	{
		return NURU_ERR_FILE_OPEN;
	}

	// read signature
	if (nuru_read_str(img->signature, NURU_STR_LEN_RAW, fp) != 0)
	{
		fclose(fp);
		return NURU_ERR_FILE_READ;
	}

	if (strcmp(img->signature, NURU_IMG_SIGNATURE) != 0)
	{
		fclose(fp);
		return NURU_ERR_FILE_TYPE;
	}

	int success = 0;
	success += nuru_read_int(&img->version, 1, fp);
	success += nuru_read_int(&img->glyph_mode, 1, fp);
	success += nuru_read_int(&img->color_mode, 1, fp);
	success += nuru_read_int(&img->mdata_mode, 1, fp);
	success += nuru_read_int(&img->cols, 2, fp);
	success += nuru_read_int(&img->rows, 2, fp);
	success += nuru_read_int(&img->fg, 1, fp);
	success += nuru_read_int(&img->bg, 1, fp);
	success += nuru_read_str(img->palette, NURU_STR_LEN_RAW, fp);
	success += nuru_read_str(img->comment, NURU_STR_LEN_RAW, fp);
	success += nuru_read_int(&img->reserved, 1, fp);

	if (success != 0)
	{
		fclose(fp);
		return NURU_ERR_FILE_READ;
	}

	// read payload
	img->num_cells = img->cols * img->rows;
	img->cells = malloc(sizeof(nuru_cell_s) * img->num_cells);
	if (img->cells == NULL)
	{
		fclose(fp);
		return NURU_ERR_MEMORY;
	}

	for (size_t c = 0; c < img->num_cells; ++c)
	{
		switch (img->glyph_mode)
		{
			case NURU_GLYPH_MODE_NONE:
				img->cells[c].ch = NURU_SPACE;
				break;
			case NURU_GLYPH_MODE_PALETTE:
				success += nuru_read_int(&img->cells[c].ch, 1, fp);
				break;
			case NURU_GLYPH_MODE_UNICODE:
				success += nuru_read_int(&img->cells[c].ch, 2, fp);
				break;
			default:
				fclose(fp);
				return NURU_ERR_FILE_MODE;
		}

		switch(img->color_mode)
		{
			case NURU_COLOR_MODE_NONE:
				img->cells[c].fg = 0;
				img->cells[c].bg = 0;
				break;
			case NURU_COLOR_MODE_4BIT:
				success += nuru_read_col(&img->cells[c].fg, &img->cells[c].bg, fp);
				break;
			case NURU_COLOR_MODE_8BIT:
				success += nuru_read_int(&img->cells[c].fg, 1, fp);
				success += nuru_read_int(&img->cells[c].bg, 1, fp);
				break;
			default:
				fclose(fp);
				return NURU_ERR_FILE_MODE;
		}

		switch(img->mdata_mode)
		{
			case NURU_MDATA_MODE_NONE:
				img->cells[c].md = 0;
				break;
			case NURU_MDATA_MODE_1BYTE:
				success += nuru_read_int(&img->cells[c].md, 1, fp);
				break;
			case NURU_MDATA_MODE_2BYTE:
				success += nuru_read_int(&img->cells[c].md, 2, fp);
				break;
			default:
				fclose(fp);
				return NURU_ERR_FILE_MODE;
		}

		if (success != 0)
		{
			fclose(fp);
			return NURU_ERR_FILE_READ;
		}
	}

	fclose(fp);
	return img->num_cells;
}

NURU_SCOPE nuru_cell_s*
nuru_get_cell(nuru_img_s* img, uint16_t col, uint16_t row)
{
	size_t idx = (row * img->cols) + col;
	if (idx >= img->num_cells)
	{
		return NULL;
	}
	return &img->cells[idx];
}

NURU_SCOPE int
nuru_img_free(nuru_img_s* img)
{
	if (!img)
	{
		return NURU_ERR_OTHER;
	}
	if (!img->cells)
	{
		return NURU_ERR_OTHER;
	}

	free(img->cells);
	img->cells = NULL;
	return 0;
}

NURU_SCOPE int
nuru_pal_load(nuru_pal_s* pal, const char* file)
{
	// open file
	FILE* fp = fopen(file, "rb");
	if (fp == NULL)
	{
		return NURU_ERR_MEMORY;
	}

	// read signature
	if (nuru_read_str(pal->signature, NURU_STR_LEN_RAW, fp) != 0)
	{
		fclose(fp);
		return NURU_ERR_FILE_READ;
	}
	
	if (strcmp(pal->signature, NURU_PAL_SIGNATURE) != 0)
	{
		fclose(fp);
		return NURU_ERR_FILE_TYPE;
	}

	int success = 0;

	// read rest of header
	success += nuru_read_int(&pal->version, 1, fp);
	success += nuru_read_int(&pal->space, 1, fp);
	success += nuru_read_str(pal->name, NURU_STR_LEN_RAW, fp);

	if (success != 0)
	{
		fclose(fp);
		return NURU_ERR_FILE_READ;
	}

	// read payload
	for (int i = 0; i < NURU_PAL_SIZE; ++i)
	{
		if (nuru_read_int(&pal->codepoints[i], 2, fp) != 0)
		{
			fclose(fp);
			return NURU_ERR_FILE_READ;
		}
	}

	return 0;
}

#endif /* NURU_IMPLEMENTATION */
#endif /* NURU_H */
