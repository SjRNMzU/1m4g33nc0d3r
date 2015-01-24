# 1m4g33nc0d3r
Stenographically {en,de}code arbitrary data in images.

## How many images formats can you embed data into?
Ans: "over 100 major file formats"
1m4g33nc0d3r is built on ImageMagick. For a full list check http://www.imagemagick.org/script/formats.php


### Running

Works wel with BMP, PNG and PDFs. Highly compressed JPG dont seem to work due to lossy compression.

####TODO 
-optimization for encoding data in JPG search and replace all colors
-confirm data can be recovered on encoding

### Example

'''
`$ ./1m4g33nc0d3r input.bmp e SAMPLE_FILE.txt output.bmp`

>>>>    1m4g33nc0d3r    <<<<

Input Image: input.bmp
        Image size: 480x360
        Image depth: 8

Image file size: 518538B
Data Size: 11297B
Compression ratio: 0.0217863
Saving to: output.bmp
`$ ./1m4g33nc0d3r output.bmp d OUTPUT_FILE.txt`

>>>>    1m4g33nc0d3r    <<<<

Input Image: output.bmp
        Image size: 480x360
        Image depth: 8

Encoded data size: 11297
Recoverd file of 11297B, saving to `OUTPUT_FILE.txt`
'''
