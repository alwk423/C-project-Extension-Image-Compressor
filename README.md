# Image Compressor (PPM → PNG)

A small image reformatter that converts NetPBM images (PBM/PGM/PPM) into PNG files.

What it does
- Reads a PBM/PGM/PPM image.
- Produces a valid PNG file containing the same image data.

Pipeline stages
1. Extraction — parse the input NetPBM file and load pixels.
2. Serialisation — convert pixels into byte-aligned scanlines.
3. Filtering — apply PNG-style line filters (Sub, Up, Average, Paeth).
4. Compression — deflate the filtered data using zlib.
5. Chunking — wrap compressed data into PNG IDAT chunks and build IHDR/IEND.
6. Encoding — write PNG signature and chunks to the output file.

Basic usage
- Build: make -C src
- Run: ./image-compressor/reformat input.ppm output.png

Credits
- Group project by 4 people.
- Contributors: Celestia Liang, Michelle Lee, Audrey Lam, Andy Chung.
