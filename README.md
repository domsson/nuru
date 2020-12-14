# nuru

nuru is a file format specification for terminal drawings using Unicode 
characters and, optionally, 4 or 8 bit colors via ANSI escape sequences.

Additionally, a header-only C file provides some basic data structures 
and functions for handling nuru files. 

## Status

Rather early work-in-progress. Do not use yet. Especially the code. 

## Files

Two file formats are being introduced;

- `.nui`: nuru image file
- `.nup`: nuru palette file

## nuru image file (NUI)

Binary file consisting of a 32 byte header and payload of varying length. 

The payload is a list of image cells, in row-major order, that describes 
the character, foreground and background color for that cell, plus optional 
meta data.

Fields larger than 1 byte are encoded using little endian byte order. 

### NUI header (32 bytes)

 - `00` - `06`: File signature `68 61 6b 6f` (`NURUIMG`) (7 bytes, char)
 - `07`: File version (1 byte, uint8\_t)
 - `08`: Glyph mode (1 byte, uint8\_t)
 - `09`: Color mode (1 byte, uint8\_t)
 - `10`: Meta data mode (1 byte, uint8\_t)
 - `11` - `12`: Width (in character cells) (2 bytes, uint16\_t)
 - `13` - `14`: Height (in character cells) (2 bytes, uint16\_t)
 - `15`: default foreground color (1 byte, uint8\_t) 
 - `16`: default background color (1 byte, uint8\_t)
 - `17` - `23`: Name of the palette originally used to create the image (7 bytes, char) 
 - `24` - `30`: Free to use (author signature) (7 bytes, char)
 - `31`: Reserved (1 byte)

### NUI payload (cell data)

    [[glyph] [color] [mdata]] [...]

Where color is:

    foreground-color background-color

### Modes

Glyph mode:

 - `0`: 0 bits, print spaces only (can be used with background color)
 - `1`: 8 bits, 256 characters as indices into a character palette\*
 - `2`: 16 bits, directly using Unicode code points (only 2 byte code points, obviously)

\*) If the palette is `default`, then the first 256 Unicode code points shall be used instead. 
    This allows creation and distribution of files without the need for a palette file.

Color mode:

 - `0`: 0 bits (monochrome)
 - `1`: 8 bits: 4 bits for foreground color, 4 bits for background color, see [4-bit ANSI color](https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit) (30 = 0, 31 = 1, ... 97 = 15)
 - `2`: 16 bits: 8 bits for foreground color, 8 bits for background color, see [8-bit ANSI color](https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit)

Meta data mode:

 - `0`: 0 bits (no meta data)
 - `1`: 8 bits
 - `2`: 16 bits

### Default colors

The default colors allow an application to make use of a terminal's default 
foreground and/ or background color when displaying a cell. This is being done 
by sacrificying two colors, one for each channel, to signify "no color". 

In 8-bit color mode, colors `0` (`#000`, black) and `15` (`#fff`, white) are 
recommended, as they both exist twice (`16` and `231`).

 - `0`: default foreground color
 - `1`: default background color 

## nuru palette file (NUP)

Binary file consisting of a 16 byte header and payload of fixed length. 

The payload is simply a list of 2 byte Unicode code points that can be 
used as the value of a `wchar_t` and therefore easily be printed.

Fields larger than 1 byte are encoded using little endian byte order. 

### NUP header (16 bytes)

 - `00` - `06`: `NURUPAL` (7 bytes, char)
 - `07`: File format version as integer (1 byte, uint8\_t)
 - `08`: Index of default fill character (usually space) (1 byte, uint8\_t)
 - `09` - `15`: Name of the palette (7 bytes, char)

### NUP payload (characters, 256 bytes)

	codepoint0 ... codepoint255

Each entry is a Unicode code point.

## Related projects / file formats

 - https://github.com/dylanaraps/pxltrm
 - https://en.wikipedia.org/wiki/Netpbm#File_formats
