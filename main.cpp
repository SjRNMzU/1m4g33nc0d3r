#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <assert.h>

#include <ImageMagick-6/Magick++.h>
#include <ImageMagick-6/magick/version.h>


using namespace std;

inline string hexPrint(const char *data, const size_t len);
inline string hexPrint(const unsigned char *data, const size_t len);

//Bit difference is a 1, same is a 0
unsigned char encode(unsigned char start, unsigned char data)
{
    return start ^ data;
}

//return differenec between 2 chars
unsigned char decode(unsigned char start, unsigned char next)
{
    return start ^ next;
}

//data is encoded 2 bytes per pixel as teh difference between the R->G and G->B values
void embedImageData(Magick::PixelPacket *pixels, size_t width, size_t height, const unsigned char *data, size_t data_len, size_t imgColDepth)
{
    string quantumColDepth = MagickQuantumDepth;
    size_t QDepth;
    if(quantumColDepth.compare("Q8") == 0){
        QDepth = 8;
    }else if(quantumColDepth.compare("Q16") == 0){
        QDepth = 16;
    }else if(quantumColDepth.compare("Q32") == 0){
        QDepth = 32;
    }else{
        cerr << "Error cannot determine compiled ImageMagick++ QuantumDefined Length" << endl;
        exit(EXIT_FAILURE);
    }

    uint32_t pow_diff = QDepth - imgColDepth;

    //prefix data with length
    uint32_t len;
    len = data_len + 4;
    unsigned char buff[len];
    memcpy(buff, &len, 4);
    memcpy(&buff[4], data, data_len);

    //encode last nibble of color with data nibble
    auto encodeQuantum = [&] (Magick::Quantum a, unsigned char data) {
        return Magick::Quantum((a & 0xFFFFFFF0) | encode(a & 0x0F, 0x0F & data));
    };
#ifdef DEBUG
    cout << MagickQuantumDepth << endl;
    cout << "imgColDepth: " << imgColDepth << endl;
    cout << "Pow Diff: " << pow_diff << endl;
    cout << "Data len: " << data_len << endl;
    cout << "Total len: " << len << endl;
    cout << "Length as bytes: " << hexPrint(buff, 4) << endl;
    cout << "data: " << hexPrint(buff, len) << endl;
#endif

    uint32_t bytes = 0;
    for(size_t i=0; i<height; ++i){
        for(size_t j=0; j<width; ++j){
            if(bytes >= len)
                return;

#ifdef DEBUG
            cout << endl << "Pixel: (" << j << "," << i << ")" << endl;
#endif
            Magick::PixelPacket *pix_packt = pixels+((width*i) + j);

            pix_packt->red = pix_packt->red >> pow_diff;
            pix_packt->green = pix_packt->green >> pow_diff;
            pix_packt->blue = pix_packt->blue >> pow_diff;

            Magick::Quantum green, blue, red;
            red = pix_packt->red;
            green = pix_packt->green;
            blue = pix_packt->blue;

            //set green quatum -> green quatum last nibble is encoded
            //encoded nibble is produced by encoding first nibble of data and last nibble or red quantum
            green = (green & 0xFFFFF0) | encodeQuantum( red & 0x0F , ( buff[bytes] >> 4 ) & 0x0F );
            blue = (blue & 0xFFFFF0) | encodeQuantum( green & 0x0F , ( buff[bytes] & 0x0F) );

#ifdef DEBUG
            printf("Red: 0x%.4X, Green: 0x%.4X, Blue: 0x%.4X\n", pix_packt->red, pix_packt->green, pix_packt->blue);
            printf("\tInput: 0x%.2X, Data: 0x%.2X, Out: 0x%.02X\n", red & 0x0F, (buff[bytes] & 0xF0) >> 4 , green & 0xFF);
            printf("\tInput: 0x%.2X, Data: 0x%.2X, Out: 0x%.02X\n", green & 0x0F, buff[bytes] & 0x0F, blue & 0xFF);
            printf("Red: 0x%.02X, Green: 0x%.02X, Blue: 0x%.02X\n", red, green, blue);
#endif


            //Set pixel color
            //wired ImageMacick thing where it expands Quantam to (ABAB)
            Magick::Color pixel_color;
            pixel_color.redQuantum((red << pow_diff) | red);
            pixel_color.greenQuantum((green << pow_diff) | green);
            pixel_color.blueQuantum((blue << pow_diff) | blue);
            *pix_packt = pixel_color;
            bytes++;
        }
    }
    return;
}

#ifdef DEBUG
void dumpPixels(Magick::PixelPacket *pixels, size_t width, size_t height, size_t N)
{
    size_t it = 0;
    for(size_t i=0; i<height; ++i){
        for(size_t j=0; j<width; ++j){
            Magick::PixelPacket *pix_packt = pixels+((width*i) + j);
            printf("0x%.4X\t0x%.4X\t0x%.4X\n", pix_packt->red, pix_packt->green, pix_packt->blue);
            it++;
            if(it > N)
                return;
        }
    }
}
#endif


unsigned char *pixelCacheToBytes(Magick::PixelPacket *pixels, size_t width, size_t height, size_t imgColDepth)
{
    string quantumColDepth = MagickQuantumDepth;
    size_t QDepth;
    if(quantumColDepth.compare("Q8") == 0){
        QDepth = 8;
    }else if(quantumColDepth.compare("Q16") == 0){
        QDepth = 16;
    }else if(quantumColDepth.compare("Q32") == 0){
        QDepth = 32;
    }else{
        cerr << "Error cannot determine compiled ImageMagick++ QuantumDefined Length" << endl;
        exit(EXIT_FAILURE);
    }

    uint32_t pow_diff = QDepth - imgColDepth;
#ifdef DEBUG
    cout << MagickQuantumDepth << endl;
    cout << "imgColDepth: " << imgColDepth << endl;
    cout << "Pow Diff: " << pow_diff << endl;
#endif

    unsigned char *data = new unsigned char [width*height];
    memset(data, 0, width*height);
    size_t it = 0;

    auto decodeQuantum = [] (Magick::Quantum a, Magick::Quantum b) -> unsigned char {
        return decode((a & 0x0000000F) , (b & 0x0000000F));
    };

    // cout << "Recovered data:\n";
    for(size_t i=0; i<height; ++i){
        for(size_t j=0; j<width; ++j){
            Magick::PixelPacket *pix_packt = pixels+((width*i) + j);
            unsigned char rg = 0x0F & decodeQuantum(pix_packt->red >> pow_diff, pix_packt->green >> pow_diff);
            unsigned char gb = 0x0F & decodeQuantum(pix_packt->green >> pow_diff, pix_packt->blue >> pow_diff);
            data[it] = (rg << 4) | gb;
            it++;
        }
    }
    //Decoded PixelArray
    return data;
}

unsigned char *uncoverImageData(Magick::PixelPacket *pixels, uint32_t width, uint32_t height, size_t imgColDepth, size_t &len)
{
    unsigned char *data = pixelCacheToBytes(pixels, width, height, imgColDepth);
#ifdef DEBUG
    cout << "First 4 bytes of data: " << hexPrint(data, 4) << endl;
#endif

    //get len
    size_t data_len = 0;
    memcpy(&data_len, &data[0], 4);

    len = data_len - 4;
    cout << "Encoded data size: " << len << endl;
    assert(data_len < width * height);

    //-4 for size of int32_t + 1 for null pointer added on by snprintf
    unsigned char *buff = new unsigned char[len];
    // snprintf(buff, len - 4 + 1, "%s", &data[4]);
    memcpy(buff, &data[4], len);
    free(data);
    return buff;
}

inline string hexPrint(const char *data, const size_t len)
{
    char *buff = new char[(len * 5) + 1]; // '0xFF '\0
    for(size_t i = 0; i < len; ++i){
        snprintf(buff, strlen(buff) +5+1, "%s0x%.2X ", buff, data[i]);
    }
    return string(buff);
}

inline string hexPrint(const unsigned char *data, const size_t len)
{
    char *buff = new char[(len * 5) + 1]; // '0xFF '\0
    for(size_t i = 0; i < len; ++i){
        snprintf(buff, strlen(buff) +5+1, "%s0x%.2X ", buff, data[i]);
    }
    return string(buff);
}


#ifdef DEBUG
void incise(char *surface, size_t surface_len, char *ink, size_t ink_len)
{
    assert(surface_len > ink_len + 1);
    for(size_t i = 0; i < ink_len; ++i){
        surface[i+1] = encode(surface[i], ink[i]);
    }
    return;
} 

void deincise(char *surface, size_t surface_len, char *ink, size_t ink_len)
{
    assert(surface_len > ink_len + 1);
    for(size_t i = 0; i < ink_len; ++i){
        ink[i] = decode(surface[i], surface[i+1]);
    }
    return;
} 
#endif

//Copy file into buffer of unsigned chars
unsigned char *fileToChar(string filename, size_t &len)
{
    fstream file;
    stringstream ss;
    file.open(filename ,ios::in | ios::binary);
    if(file.is_open())
        ss << file.rdbuf();
    else{
        throw new runtime_error("Error opening: " + filename);
        file.close();
    }
    file.close();
    len = ss.str().size();
    unsigned char *buff = new unsigned char [len];
    memcpy(buff, ss.str().c_str(), len);
    return buff;
}

//Copy unsigned char bufferr into file
void charToFile(const unsigned char *buff, size_t &len, string filename)
{
    fstream file;
    file.open(filename ,ios::out | ios::binary | ios::trunc);
    std::filebuf *fBuf = file.rdbuf();
    if(file.is_open())
        fBuf->sputn(reinterpret_cast<const char *>(buff), len);
    else{
        throw new runtime_error("Error opening: " + filename);
        file.close();
    }
    file.close();
    return;
}

int main(int argc, char *argv[])
{
    if(argc < 4){
        cerr << "Useage: 1m4g33nc0d3r image.ext e/d embed.file output" << endl;
        // cerr << "Useage: 1m4g33nc0d3r -i image.ext e/d -o output" << endl;
        exit(EXIT_FAILURE);
    }

    Magick::InitializeMagick(*argv);

    //print init screen
    cout << endl << ">>>>\t1m4g33nc0d3r\t<<<<" << endl << endl;


    string imagePath = argv[1];
    Magick::Image *img = new Magick::Image(imagePath);
    img->modifyImage(); //make sure only 1 reference to image

    size_t width = img->columns();
    size_t height = img->rows();
    size_t depth = img->depth();

    cout << "Input Image: " << argv[1] << endl;
    cout << "\tImage size: " << width << "x" << height << endl;
    cout << "\tImage depth: " << depth << endl;
    cout << endl;

    Magick::PixelPacket *pixels = img->getPixels(0, 0, width, height);

    char op = *argv[2];
    switch(op){
        case 'e':{
            size_t buff_len = 0;
            unsigned char *buff = fileToChar(argv[3], buff_len);

            cout << "Image file size: " << img->fileSize() << "B" << endl;
            cout << "Data Size: " << buff_len << "B" << endl;
            cout << "Compression ratio: " << static_cast<float>(buff_len)/static_cast<float>(img->fileSize()) << endl;

            // cout << "Data: " << data << endl;
            // cout << "hex(Data): " << hexPrint(data.c_str(), data.size()) << endl;

            if((buff_len + 1) > (width*height) ){
                cerr << "Error not enough space to encode data in image" << endl;
                exit(EXIT_FAILURE);
            }

            #ifdef DEBUG
            dumpPixels(pixels, width, height, buff_len + 4);
            #endif
            embedImageData(pixels, width, height, buff, buff_len, depth);
            img->syncPixels();
            cout << "Saving to: " << argv[4] << endl;
            img->write(argv[4]);
            #ifdef DEBUG
            dumpPixels(pixels, width, height, buff_len + 4);
            #endif
            break;
            }
        case 'd':{
            #ifdef DEBUG
            dumpPixels(pixels, width, height, buff_len + 4);
            #endif
            size_t data_len = 0;
            unsigned char *data = uncoverImageData(pixels, width, height, depth, data_len);
            cout << "Recoverd file of " << data_len << "B, saving to `" << argv[3] << "`" << endl;
            charToFile(data, data_len, argv[3]);
            break;
            }
        default:
            cerr << "Error, no operation specified" << endl;
            exit(EXIT_FAILURE);
    }

    delete img;
    return 0;
}

