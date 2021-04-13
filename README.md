# get-exe-icon

This library can be used to extract the primary icon resource of a Windows PE
executable (.exe or .dll) to an .ico file. The primary icon is the icon shown
by Explorer for .exe files. The output .ico is exactly as the original
application developer made it, including all available image sizes and
optionally including high-res PNGs.

## Usage

Copy `get-exe-icon.c` and `get-exe-icon.h` into your project. You may wish to
only copy the functions you need or modify them as appropriate for your project.

The primary function is `get_exe_icon_from_file_utf16()`, and almost every
other function declared in the header is a wrapper around that. Usage for
every function is documented in the header.

## Testing

`tests.c`, along with the data in `testdata` contains a suite of tests. Use
your compiler of choice to compile `tests.c` and `get-exe-icon.c` and run the
test program.

## License

[MIT](https://mit-license.org/)

## Contributors

* Aury Snow
* Serge Zarembsky
* Frank Chiarulli
