#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <sstream>
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

    cout << MagickQuantumDepth << endl;
    cout << "imgColDepth: " << imgColDepth << endl;
    uint32_t pow_diff = QDepth - imgColDepth;
    cout << "Pow Diff: " << pow_diff << endl;

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

    cout << "Data len: " << data_len << endl;
    cout << "Total len: " << len << endl;
    cout << "Length as bytes: " << hexPrint(buff, 4) << endl;
    cout << "data: " << hexPrint(buff, len) << endl;

    uint32_t bytes = 0;
    for(size_t i=0; i<height; ++i){
        for(size_t j=0; j<width; ++j){
            if(bytes >= len)
                break;

            cout << endl << "Pixel: (" << j << "," << i << ")" << endl;
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
            // pix_packt->green = ((pix_packt->green & 0xFFFFFFF0) | (encodeQuantum(pix_packt->red, (buff[bytes] & 0xF0) >> 4) & 0x0F));
            // pix_packt->blue = ((pix_packt->blue & 0xFFFFFFF0) | (encodeQuantum(pix_packt->green, (buff[bytes] & 0x0F)) & 0x0F));

            green =     (green & 0xFFFFF0) | encodeQuantum( red & 0x0F , ( buff[bytes] >> 4 ) & 0x0F );
            blue =       (blue & 0xFFFFF0) | encodeQuantum( green & 0x0F , ( buff[bytes] & 0x0F) );

            // printf("Red: 0x%.02X, Green: 0x%.02X, Blue: 0x%.02X\n", red & 0xFF, green & 0xFF, blue & 0xFF);
            printf("Red: 0x%.4X, Green: 0x%.4X, Blue: 0x%.4X\n", pix_packt->red, pix_packt->green, pix_packt->blue);
            printf("\tInput: 0x%.2X, Data: 0x%.2X, Out: 0x%.02X\n", red & 0x0F, (buff[bytes] & 0xF0) >> 4 , green & 0xFF);
            printf("\tInput: 0x%.2X, Data: 0x%.2X, Out: 0x%.02X\n", green & 0x0F, buff[bytes] & 0x0F, blue & 0xFF);
            printf("Red: 0x%.02X, Green: 0x%.02X, Blue: 0x%.02X\n", red, green, blue);


            //Set pixel color
            //wired ImageMacick thing where it expands Quantam to (ABAB)
            Magick::Color pixel_color;
            pixel_color.redQuantum((red << pow_diff) | red);
            pixel_color.greenQuantum((green << pow_diff) | green);
            pixel_color.blueQuantum((blue << pow_diff) | blue);
            *pix_packt = pixel_color;
            bytes++;
        }
        if(bytes >= len)
            break;
    }
    return;
}


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

    cout << MagickQuantumDepth << endl;
    cout << "imgColDepth: " << imgColDepth << endl;
    uint32_t pow_diff = QDepth - imgColDepth;
    cout << "Pow Diff: " << pow_diff << endl;

    unsigned char *data = (unsigned char *)malloc(width*height);
    memset(data, 0, width*height);
    size_t it = 0;

    auto decodeQuantum = [] (Magick::Quantum a, Magick::Quantum b) -> unsigned char {
        return decode((a & 0x0000000F) , (b & 0x0000000F));
    };

    cout << "Recovered data:\n";
    for(size_t i=0; i<height; ++i){
        for(size_t j=0; j<width; ++j){
            Magick::PixelPacket *pix_packt = pixels+((width*i) + j);
            unsigned char rg = 0x0F & decodeQuantum(pix_packt->red >> pow_diff, pix_packt->green >> pow_diff);
            unsigned char gb = 0x0F & decodeQuantum(pix_packt->green >> pow_diff, pix_packt->blue >> pow_diff);
            data[it] = (rg << 4) | gb;
            // cout << hexPrint(&rg, 1) << " ";
            // cout << hexPrint(&gb, 1) << " ";
            // cout << hexPrint(&data[it], 1) << "\n";
            it++;

            // if(it > 10)
            //     break;
        }
        // if(it > 10)
        //     break;
    }
    //Decoded PixelArray
    return data;
}

string uncoverImageData(Magick::PixelPacket *pixels, uint32_t width, uint32_t height, size_t imgColDepth)
{
    unsigned char *data = pixelCacheToBytes(pixels, width, height, imgColDepth);
    cout << "First 4 bytes of data: " << hexPrint(data, 4) << endl;

    uint32_t len;
    //get len
    memcpy(&len, &data[0], 4);

    cout << "Encoded data size: " << len << endl;
    assert(len < 100);

    //-4 for size of int32_t + 1 for null pointer added on by snprintf
    char buff[len - 4 + 1];
    snprintf(buff, len - 4 + 1, "%s", &data[4]);
    return string(buff);
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

int main(int argc, char *argv[])
{
    char blob[8] = {"abcdefg"};
    char data[6] = {"Hello"};
    char encoded_blob[8];
    char decoded[8]; 

    //copy blob into encoded blob, set decoded buff to 0
    memcpy(encoded_blob, blob, 8);
    memset(decoded, 0, 8);
    
    incise(encoded_blob, 8, data, 6);
    deincise(encoded_blob, 8, decoded, 6);

    cout << "blob: " << hexPrint(blob, 8) << endl;
    cout << "data: " << hexPrint(data, 6) << endl;
    cout << "encoded_blob: "<< hexPrint(encoded_blob, 8) << endl;
    cout << "data: " << data << endl;
    cout << "decoded: " << decoded << endl;

    cout << endl << ">>>>\t1m4g33nc0d3r\t<<<<" << endl << endl;

    string data_str("Hello");
    cout << "Data to encode: " << data_str << endl;
    cout << "Data to encode: " << hexPrint(data_str.c_str(), data_str.size()) << endl;

    Magick::InitializeMagick(*argv);

    string imagePath = argv[1];
    Magick::Image *img = new Magick::Image(imagePath);
    img->modifyImage(); //make sure only 1 reference to image

    size_t width = img->columns();
    size_t height = img->rows();
    size_t bpp = 2;
    size_t modDepth = img->modulusDepth();
    size_t depth = img->depth();

    cout << "Bits Per Pixel: " << bpp << endl;
    cout << "Image mod depth: " << modDepth << endl;
    cout << "Image depth: " << depth << endl;
    cout << "Image size: " << width << "x" << height << endl;


    // Magick::Pixels pixel_cache(*img);
    // Magick::PixelPacket *pixels;
    // pixels = pixel_cache.get(0, 0, width, height);

    Magick::PixelPacket *pixels = img->getPixels(0, 0, width, height);

    // uint32_t bytes = 0;
    if((data_str.size() + 1) > (width*height) ){
        cerr << "Error not enough space to encode data in image" << endl;
        exit(EXIT_FAILURE);
    }

    unsigned char data_buff[data_str.size()];
    memcpy(data_buff, data_str.c_str(), data_str.size());

    // return 0;

    char op = *argv[2];
    switch(op){
        case 'e':{
            dumpPixels(pixels, width, height, 12);
            embedImageData(pixels, width, height, data_buff, data_str.size(), depth);
            img->syncPixels();
            img->write(argv[3]);
            dumpPixels(pixels, width, height, 12);
            // uint32_t kbytes = bytes / 1024;
            // bytes = bytes % 1024;
            // cout << "Total Bytes of space used: " << kbytes << "KB " << bytes << "B" << endl;
            break;
            }
        case 'd':{
            dumpPixels(pixels, width, height, 12);
            string decodedData = uncoverImageData(pixels, width, height, depth);
            cout << "Decoded data: " << decodedData.c_str() << endl;
            break;
            }
        default:
            cerr << "Error, no operation specified" << endl;
            exit(EXIT_FAILURE);
    }

    delete img;
    return 0;
}

