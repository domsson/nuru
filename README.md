# nuru

nuru is a file format specification for terminal drawings using ASCII or 
Unicode characters and, optionally, colors via ANSI escape sequences or RGB. 
It can therefore handle a multitude of ANSI and ASCII art. 

nuru uses two types of files, image files and optional palette files.
Palette files can contain either glyphs (characters) or colors. This enables
the emulation of code pages and allows for color palette swapping.

Images can further contain meta data for each cell. This makes the format
suitable for use in a variety of projects, including games.

This repository contains the file format descriptions and a header-only
C file providing data structures and functions for handling nuru files.

A work-in-progress nuru image web editor is also available:
[nuru-web](https://github.com/domsson/nuru-web/)

## Status

This is still a work in progress. Once the formats are final for their 
initial version (1), I'll update this space to let you know.

## Files

Two binary file formats are being introduced:

- `NUI` - nuru image file
- `NUP` - nuru palette file

All length values given are in bytes. Numerical fields larger than 1 byte 
are encoded using big endian byte order and can therefore be translated 
to the host's byte order by using functions like `ntohs()`.

## NUI - nuru image file

Binary file consisting of a 32 byte header and payload of varying length. 

The payload is a list of image cells, in row-major order, that describes 
the character, foreground and background color for that cell, plus optional 
meta data.

### NUI header (32 bytes)

| offset | field       | length | type       | description                     |
|--------|-------------|--------|------------|---------------------------------|
| `00`   | signature   | 7      | `char`     | File signature, `4E 55 52 55 49 4D 47` (`NURUIMG`) |
| `07`   | version     | 1      | `uint8_t`  | File format version |
| `08`   | glyph\_mode | 1      | `uint8_t`  | Glyph mode | 
| `09`   | color\_mode | 1      | `uint8_t`  | Color mode |
| `10`   | mdata\_mode | 1      | `uint8_t`  | Meta data mode |
| `11`   | width       | 2      | `uint16_t` | Image width (number of columns) |
| `13`   | height      | 2      | `uint16_t` | Image height (number of rows) | 
| `15`   | fg\_key     | 1      | `uint8_t`  | Default/key foreground color |
| `16`   | bg\_key     | 1      | `uint8_t`  | Default/key background color |
| `17`   | glyph\_pal  | 7      | `char`     | Name of the glyph palette to be used for this image |
| `24`   | color\_pal  | 7      | `char`     | Name of the color palette to be used for this image |
| `31`   | reserved    | 1      |  n/a       | Currently not in use, reserved for future extensions |

The `fg_key` and `bg_key` fields allow an application to make use of a 
terminal's default foreground and/ or background color when displaying a cell. 
This is being done by sacrificying two colors, one for each channel, to signify 
"no color". This allows for transparency when displaying or combining images. 

In 8-bit color mode, colors `0` (`#000`, black) and `15` (`#fff`, white) are 
recommended, as they both exist twice (`16` and `231`).

The `glyph_mode`, `color_mode` and `mdata_mode` values were designed in a way 
that they can be directly used as buffer size for the buffers that are used to 
store the respective values from the cell data.

The `palette` field is case insensitive, so applications should either convert 
to lower or upper case before comparing it against available palette files.

### NUI payload (cell data)

The payload consists of `width` * `height` entries. The size and data fields 
of each entry depend on `glyph_mode`, `color_mode` and `mdata_mode`. 

	[[glyph] [color] [mdata]] [...]

#### Cell data: glyph

Defines the glyph to be used for a cell. The length and interpretation of the 
data depends on the `glyph_mode`:

| glyph\_mode | length | type       | description                              |
|-------------|--------|------------|------------------------------------------|
| 0           | 0      | n/a        | No glpyhs, the image only uses the space character (requires a `color_mode` other than `0`) |
| 1           | 1      | `uint8_t`  | ASCII char code *or* index into glyph palette |
| 2           | 2      | `uint16_t` | Unicode code point into the Basic Multilingual Plane (Plane 0) |

If `glyph_mode` is set to `1` and `glyph_pal` is set, then the value is to be 
interpreted as an index into the glyph palette. However, if `glpyh_pal` is 
empty, then the value is simply an ASCII char code (Unicode extended ASCII).

The correlation of `glyph_mode` and the `length` is intentional.

#### Cell data: color

Defines the foreground and background color data for a cell. The length and 
interpretation of the data depends on the `color_mode`:

| color\_mode | length | type         | description                            |
|-------------|--------|--------------|----------------------------------------|
| 0           | 0      | n/a          | No colors, the image is monochrome (requires a `glyph_mode` other than `0`) |
| 1           | 1      | `uint8_t`    | 4 bit ANSI colors; high nibble is foreground, low nibble is background color |
| 2           | 2      | 2× `uint8_t` | 8 bit ANSI colors; high byte is foreground, low byte is background color |

 - For `1`, see [4-bit ANSI color](https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit) (0 => 30, 1 => 31, ... 15 => 97)
 - For `2`, see [8-bit ANSI color](https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit)

If `color_mode` is set to `2` and `color_pal` is set, then the values are to be 
interpreted as indices into the color palette. However, if `color_pal` is empty, 
then the values are to be interpreted as 8 bit ANSI color codes. 

The correlation of `color_mode` and the `length` is intentional.

#### Cell data: mdata

Defines the meta data for a cell. The length of the data depends on the 
`mdata_mode` field, the interpretation is outside the scope of nuru:

| mdata\_mode | length | type       | description                              |
|-------------|--------|------------|------------------------------------------|
| 0           | 0      | n/a        | No meta data                             |
| 1           | 1      | n/a        | 1 byte of meta data                      |
| 2           | 2      | n/a        | 2 bytes of meta data                     |

The correlation of `mdata_mode` and the `length` is intentional.

## NUP - nuru palette file

Describes a glyph or color palette, i.e. the characters or colors to be used 
in a nuru image. 16 byte header and payload of fixed length (256 entries). 

### NUP header (16 bytes)

| offset | field     | length | type      | description                        |
|--------|-----------|--------|-----------|------------------------------------|
| `00`   | signature | 7      | `char`    | File format signature `4E 55 52 55 50 41 4C` (`NURUPAL`) |
| `07`   | version   | 1      | `uint8_t` | File format version                |
| `08`   | type      | 1      | `uint8_t` | Palette type, see below            |
| `09`   | default1  | 1      | `uint8_t` | Index of default glyph (usually space) or foreground color |
| `10`   | default2  | 1      | `uint8_t` | Index of default background color  | 
| `11`   | context   | 4      | n/a       | Free to use                        |
| `12`   | reserved  | 1      | n/a       | Currently not in use, reserved for future extensions |

### NUP payload (256 or 512 or 768 bytes)

	entry0 ... entry255

Size and interpretation of each entry depends on the palette `type`, see below. 
There are always 256 entries, therefore the total payload size is `256 * type`.

#### Palette type and entry data

The palette type determines not only whether the palette is a glyph or color 
palette, but also the size and interpretation of each entry within the palette:

| type | palette | entry size | entry interpretation                           |
|------|---------|------------|------------------------------------------------|
| 1    | colors  | 1          | 8 bit ANSI color                               |
| 2    | glyphs  | 2          | Unicode code point into the Basic Multilingual Plane (Plane 0) |
| 3    | colors  | 3          | RGB color (R, G and B channels in that order)  |

The correlation of `type` and the entry size is intentional.

## Related projects / file formats

 - [pxltrm](https://github.com/dylanaraps/pxltrm)
 - [Netpbm file format](https://en.wikipedia.org/wiki/Netpbm#File_formats)

