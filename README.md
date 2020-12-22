# nuru

nuru is a file format specification for terminal drawings using Unicode 
characters and, optionally, colors via ANSI escape sequences or RGB. 

nuru uses two files, an image file and an (optional) glyph palette file.

This repository contains the file format descriptions and a header-only 
C file providing data structures and functions for handling nuru files. 

A work-in-progress nuru image web editor is also available: 
[nuru-web](https://github.com/domsson/nuru-web/)

## Status

This is still a work in progress. Once the formats are final for their 
initial version (1), I'll update this space to let you know. Most likely, 
the NUI format is final, but I'm still undecided on the NUP header format.

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
| `17`   | palette     | 7      | `char`     | Name of the palette intended to be used for this image |
| `24`   | comment     | 7      | `char`     | Free to use (for example, author signature) |
| `31`   | reserved    | 1      |  n/a       | Currently not in used, should be left empty |

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
| 1           | 1      | `uint8_t`  | Index into a glyph palette, unless palette name is `default` |
| 2           | 2      | `uint16_t` | Unicode code point into the Basic Multilingual Plane (Plane 0) |
| 3           | 3      | `uint32_t` | Unicode code point into any Plane (not recommended) |

If the `palette` field is set to `default` or `ASCII` and `glyph_mode` is `1`, 
then the palette is assumed to be the first 256 Unicode code points (extended 
ASCII). This allows for the distribution of simple ASCII art images without 
the need to ship a glyph palette at all.

#### Cell data: color

Defines the foreground and background color data for a cell. The length and 
interpretation of the data depends on the `color_mode`:

| color\_mode | length | type         | description                           |
|-------------|--------|--------------|---------------------------------------|
| 0           | 0      | n/a          | No colors, the image is monochrome (requires a `glyph_mode` other than `0`) |
| 1           | 1      | `uint8_t`    | 4 bit ANSI colors; high nibble is foreground, low nibble is background color |
| 2           | 2      | 2× `uint8_t` | 8 bit ANSI colors; high byte is foreground, low byte is background color |
| 3           | 3      | 2× `uint8_t` | RGB colors; R, G and B channels in that order |

 - For `1`, see [4-bit ANSI color](https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit) (30 = 0, 31 = 1, ... 97 = 15)
 - For `2`, see [8-bit ANSI color](https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit)

A possible future expansion would be to allow for the use of color palettes, 
similar to glyph palettes. This would allow using RGB colors, but limit the 
selection of colors to a limited set. The only issue is that this would break 
the current field design, where the mode fields can directly be used as the 
buffer size for the respective data. If this was to be implemented, the NUP 
format would need to use the alternatively suggested 32 bit header (see below).

#### Cell data: mdata

Defines the meta data for a cell. The length of the data depends on the 
`mdata_mode` field, the interpretation is outside the scope of nuru:

| mdata\_mode | length | type       | description                              |
|-------------|--------|------------|------------------------------------------|
| 0           | 0      | n/a        | No meta data                             |
| 1           | 1      | n/a        | 1 byte of meta data                      |
| 2           | 2      | n/a        | 2 byte of meta data                      |
| 3           | 3      | n/a        | 3 byte of meta data                      |

## NUP - nuru palette file

Describes a glyph palette, i.e. the characters to be used in a nuru image.
16 byte header and payload of fixed length. 

The payload is simply a list of 2 byte Unicode code points that can be 
used as the value of a `wchar_t` and therefore easily be printed.

### NUP header (current implementation, 16 bytes)

| offset | field       | length | type       | description                       |
|--------|-------------|--------|------------|-----------------------------------|
| `00`   | signature   | 7      | `char`     | File format signature `4E 55 52 55 50 41 4C` (`NURUPAL`) |
| `07`   | version     | 1      | `uint8_t`  | File format version |
| `08`   | default     | 1      | `uint8_t`  | Default element (usually points to the space character) |
| `09`   | name        | 7      | `char`     | Name of the palette |

### NUP payload (characters, 256 bytes)

	codepoint0 ... codepoint255

Each entry is a Unicode code point from the basic multilingual plane.

### NUP header (alternative proposal, 32 bytes)

This alternative version has the benefit of being the same size as the NUI 
header, and the fields that have the same meaning are aligned at the same 
offsets, which makes for easier reading and writing implementations. More 
importantly, however, this header allows for palettes to be used for other 
purposes than just glyphs, namely color palettes. It all comes at the cost 
of 16 additional bytes per palette file. Worth it?

| offset | field       | length | type       | description                       |
|--------|-------------|--------|------------|-----------------------------------|
| `00`   | signature   | 7      | `char`     | File format signature `4E 55 52 55 50 41 4C` (`NURUPAL`) |
| `07`   | version     | 1      | `uint8_t`  | File format version |
| `08`   | type        | 1      | `uint8_t`  | Palette type (glyphs, colors) | 
| `09`   | mode        | 1      | `uint8_t`  | Palette mode (usually size of each entry) |
| `10`   | reserved    | 1      |  n/a       | Currently not in use, should be left empty |
| `11`   | length      | 2      | `uint16_t` | Number of entries in the payload |
| `13`   | depth       | 2      | `uint16_t` | Always `1` so that `length*depth` is the number of entries in the payload |
| `15`   | default1    | 1      | `uint8_t`  | Primary default entry (e.g. index of space character) |
| `16`   | default2    | 1      | `uint8_t`  | Secondary default entry (e.g. recommended background color) |
| `17`   | name        | 7      | `char`     | Name of the palette |
| `24`   | comment     | 7      | `char`     | Free to use (for example, author signature) |
| `31`   | reserved    | 1      |  n/a       | Currently not in use, should be left empty |

## Related projects / file formats

 - [pxltrm](https://github.com/dylanaraps/pxltrm)
 - [Netpbm file format](https://en.wikipedia.org/wiki/Netpbm#File_formats)

