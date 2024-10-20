# Extract JPEG from Canon CR3 


## Issue

Common tools like `exiftool`, `exiv2`, `imagemagick`, `libraw` can only get the preview images from the Canon CR3 raw file using exif info, with no thumbnail. And the preview images are just too compressed to be used (one is 160x120, the other is 1620x1080).


Test result for getting preview image:

```bash
$  exiv2 --print p test.CR3
Preview 1: image/jpeg, 160x120 pixels, 10220 bytes
Preview 2: image/jpeg, 1620x1080 pixels, 183454 bytes
```

The result for getting thumbnail:

```bash
$ exiv2 --verbose --extract t test.CR3
File 1/1: test.CR3
test.CR3: Image does not contain an Exif thumbnail
```


## Solution

So this tool is modified from [Geeqie](https://github.com/BestImageViewer/geeqie/) source code. It provides a magical way to get the JPEG thumbnail that has the same size as the raw file, by matching the start token and end token of a JPEG file against the raw CR3 file. 


## Usage

- Compile the code `$ g++ -o cr3-to-jpeg cr3-to-jpeg.cpp`
- Run the program `$ ./cr3-to-jpeg /path/to/your/directory`
- (optional) Move the executable where you want it



## Note

- All the CR3 tests are done with my Canon M6 Mark2 Camera. 
- Limited tested
