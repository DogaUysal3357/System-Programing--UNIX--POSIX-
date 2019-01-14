#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


/*** MACROS ***/
#define READ_FLAG O_RDONLY
#define IMAGE_WIDTH_TAG_ID 256
#define IMAGE_HEIGHT_TAG_ID 257
#define BITSPERSAMPLE_TAG_ID 258
#define COMPRESSION_TAG_ID 259
#define PHOTOINTERPRETATION_TAG_ID 262
#define STRIPOFFSET_TAG_ID 273
#define SAMPLEPERPIXEL_TAG_ID 277
#define STRIPBYTECOUNT_TAG_ID 279
#define ROWPERSTRIP_TAG_ID 278
#define TAG_SIZE 12
#define TRUE 1
#define FALSE 0

/*** Structs ***/
typedef struct {
    char byteOrder[2];
    short int meaningOfLife;
    unsigned int ifdOffset;

} IFH_t;

typedef struct {

    short int tagID;
    short int dataType;
    unsigned int dataCount;
    unsigned int dataOffset;

} TagField_t;

/*** PROTOTYPES ***/
ssize_t r_read(int fd, void *buf, size_t size);
void readIFH(int fileDescriptor, IFH_t *header);
void readAndPrintImage(int fileDescriptor);

/*** MAIN ***/
int main(int argc, char* argv[]) {
    int inFileDestination; // input file destination
    IFH_t header;

    //Control for correct inputs, usage
    if(argc != 2) {
        fprintf(stderr,"Usage : %s String <filename> }n", argv[0]);
        return 1;
    }

    //Open tiff file to read
    while( (inFileDestination = open(argv[1],READ_FLAG)) == -1 && errno == EINTR );
    if(inFileDestination == -1){
        fprintf(stderr, "Failed to open file %s\n", argv[1]);
    }

    //Read ImageFileHeader to header struct
    readIFH(inFileDestination, &header);

    //Check for valid byteOrder for this program.
    if(header.byteOrder[0] == 'M'){
        printf("This program does not support MotorolaByte ordered images.\n\n");
        return 1;
    }

    //Go to ImageFileDirectory offset
    lseek(inFileDestination, header.ifdOffset, SEEK_SET);

    //Read and print Image
    readAndPrintImage(inFileDestination);

    //Close File
    close(inFileDestination);

    return 0;
}

ssize_t r_read(int fd, void *buf, size_t size) {
    ssize_t retval;
    while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
    return retval;
}

void readIFH(int fileDescriptor, IFH_t *header){
    r_read(fileDescriptor, header, sizeof(IFH_t));

    printf("Byte Order -> %.2s \n\n", header->byteOrder);
}

void readAndPrintImage(int fileDescriptor){

    short unsigned int numOfEntries;
    TagField_t tag,
            stripOffSetTag,
            stripByteCountTag;
    int i, k, l,
            width,
            height ;
    unsigned long int rowPerStrip = 0;
    unsigned char* buffer;
    unsigned long *offSetsArr;
    unsigned long *stripByteCountArr;

    unsigned short int whiteIsZero = TRUE;
    unsigned short bitsPerSample;
    unsigned short samplePerPixel;
    unsigned short bufferSize;
    int readByte = 0;


    r_read(fileDescriptor, &numOfEntries, sizeof(short));

    for(i=0; i< numOfEntries; ++i ){
        r_read(fileDescriptor,&tag, sizeof(TagField_t));

        //Read Image Width
        if(tag.tagID == IMAGE_WIDTH_TAG_ID){
            width = tag.dataOffset;
        }
        //Read Image Height ( Length )
        if(tag.tagID == IMAGE_HEIGHT_TAG_ID){
            height = tag.dataOffset;
        }
        //Read PhotoInterpretation to check is "white is zero"
        if(tag.tagID == PHOTOINTERPRETATION_TAG_ID){
            if(tag.dataOffset == 1){
                whiteIsZero = FALSE;
            }else if (tag.dataOffset == 0){
            }
        }
        //Read Strip Offset Tag to stripOffSetTag struct.
        if(tag.tagID == STRIPOFFSET_TAG_ID){
            stripOffSetTag = tag;
        }
        //Read rowPerStrip
        if(tag.tagID == ROWPERSTRIP_TAG_ID){
            rowPerStrip = tag.dataOffset;
        }
        //Read StripByteCount
        if(tag.tagID == STRIPBYTECOUNT_TAG_ID){
            stripByteCountTag = tag;
        }
        // Read bits per sample
        if(tag.tagID == BITSPERSAMPLE_TAG_ID){
            bitsPerSample = (short)tag.dataOffset;
        }
        //Read sample per pixel
        if(tag.tagID == SAMPLEPERPIXEL_TAG_ID){
            samplePerPixel = (short)tag.dataOffset;
        }
    }

    printf("Image Height -> %d\n", height);
    printf("Image Width -> %d\n\n", width);

    //Allocate enough memory for the buffer.
    bufferSize =  (bitsPerSample/8 ) * samplePerPixel;
    buffer = (unsigned char*) malloc(sizeof(unsigned char) * bufferSize);
    if(bitsPerSample%8 != 0){
        printf("This program does not support files with less than 1 byte per pixel\n\n");
        exit(1);
    }

    //If there is more than 1 stripe, read each strips byte count
    if(stripByteCountTag.dataCount != 1){
        stripByteCountArr = (unsigned long*) malloc(stripByteCountTag.dataCount);
        lseek(fileDescriptor, stripByteCountTag.dataOffset, SEEK_SET);
        for (i = 0; i < stripByteCountTag.dataCount; ++i) {
            r_read(fileDescriptor, &stripByteCountArr[i], sizeof(unsigned long));
        }
    }
    printf("\n");


    //Read & Print image.


    lseek(fileDescriptor,stripOffSetTag.dataOffset,SEEK_SET);
    //If there's only one stripe
    if(stripOffSetTag.dataCount == 1){

        printf("***********\n\n");
        for(i=0 ; i<height ;++i){
            for(k=0; k<width ;++k){
                r_read(fileDescriptor,buffer,bufferSize);
                if(buffer[0] == 255){
                    if(whiteIsZero){
                        printf("0 ");
                    }else{
                        printf("1 ");
                    }
                }else{
                    if(whiteIsZero){
                        printf("1 ");
                    }else{
                        printf("0 ");
                    }
                }
            }
            printf("\n");
        }
    }else {

        //If there's more than one Stripe
        offSetsArr = (unsigned long *) malloc(stripOffSetTag.dataCount);
        for (i = 0; i < stripOffSetTag.dataCount; ++i) {
            r_read(fileDescriptor, &offSetsArr[i], sizeof(unsigned long));
        }

        printf("***********\n\n");

        //Strip Cycle
        for(i=0; i<stripOffSetTag.dataCount; ++i){
            lseek(fileDescriptor,offSetsArr[i],SEEK_SET);

            while(readByte<stripByteCountArr[i]){
                for(k=0; k<height && readByte<stripByteCountArr[i]; ++k ){
                    if(l == width)
                        printf("\n");
                    for(l=0; l<width && readByte<stripByteCountArr[i]; ++l){
                        readByte += r_read(fileDescriptor,buffer,bufferSize);
                        if(buffer[0] == 255){
                            if(whiteIsZero){
                                printf("1 ");
                            }else{
                                printf("0 ");
                            }
                        }else{
                            if(whiteIsZero){
                                printf("0 ");
                            }else{
                                printf("1 ");
                            }
                        }
                    }
                }
            }
            readByte = 0;
        }
    }


    if(stripOffSetTag.dataCount != 1){
        free(offSetsArr);
    }
    if(stripByteCountTag.dataCount != 1){
        free(stripByteCountArr);
    }
    free(buffer);
}


















































