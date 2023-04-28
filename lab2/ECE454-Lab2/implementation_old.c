#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utilities.h"  // DO NOT REMOVE this line
#include "implementation_reference.h"   // DO NOT REMOVE this line

// Hash codes for sensor operations
#define W   177660
#define A   177638
#define S   177656
#define D   177641
#define CW  5862207 
#define CCW 193452258
#define MX  5862538
#define MY  5862539

// Declariations
__attribute__((always_inline)) void processMoveUp(int offset);
__attribute__((always_inline)) void processMoveRight(int offset);
__attribute__((always_inline)) void processMoveDown(int offset);
__attribute__((always_inline)) void processMoveLeft(int offse);
__attribute__((always_inline)) void processRotateCW(int rotate_iteration);
__attribute__((always_inline)) void processRotateCCW(int rotate_iteration);
__attribute__((always_inline)) void processMirrorX();
__attribute__((always_inline)) void processMirrorY();
__attribute__((always_inline)) unsigned char *renderFrame(unsigned char *buffer_frame);
__attribute__((always_inline)) unsigned long hash(unsigned char *str);

typedef struct {
    __uint8_t r, g, b;
    short row, col;
} Pixel;

Pixel *objectPixels;
int numObjectPixels;

unsigned int _width;
unsigned int _height;

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image up
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveUp(int offset) {
    // handle negative offsets
    if (offset < 0){
        offset *= -1;

        int i;

        for (i = 0; i < numObjectPixels; i++) {
            objectPixels[i].row += offset;
        }

        return;
    }

    int i;

    for (i = 0; i < numObjectPixels; i++) {
        objectPixels[i].row -= offset;
    }

    return;
}

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image left
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveRight(int offset) {
    // handle negative offsets
    if (offset < 0){
        offset *= -1;

        int i;

        for (i = 0; i < numObjectPixels; i++) {
            objectPixels[i].col -= offset;
        }

        return;
    }

    int i;

    for (i = 0; i < numObjectPixels; i++) {
        objectPixels[i].col += offset;
    }

    return;
}

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image up
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveDown(int offset) {
    // handle negative offsets
    if (offset < 0){
        offset *= -1;

        int i;

        for (i = 0; i < numObjectPixels; i++) {
            objectPixels[i].row -= offset;
        }

        return;
    }

    int i;

    for (i = 0; i < numObjectPixels; i++) {
        objectPixels[i].row += offset;
    }

    return;
}

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image right
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveLeft(int offset) {
    // handle negative offsets
    if (offset < 0){
        offset *= -1;

        int i;

        for (i = 0; i < numObjectPixels; i++) {
            objectPixels[i].col += offset;
        }

        return;
    }

    int i;

    for (i = 0; i < numObjectPixels; i++) {
        objectPixels[i].col -= offset;
    }

    return;
}

/***********************************************************************************************************************
 * @param rotate_iteration - rotate object inside frame buffer clockwise by 90 degrees, <iteration> times
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note: You can assume the frame will always be square and you will be rotating the entire image
 **********************************************************************************************************************/
__attribute__((always_inline)) void processRotateCW(int rotate_iteration) {
    // handle negative offsets
    if (rotate_iteration < 0){
        rotate_iteration *= -3;
    }

    rotate_iteration = rotate_iteration % 4;

    int iteration, i, render_column, render_row, pixel_column, pixel_row;

    for (iteration = 0; iteration < rotate_iteration; iteration++) {
        for (i = 0; i < numObjectPixels; i++) {
            render_column = _width - 1;
			render_row = 0;

            pixel_column = objectPixels[i].col;
            pixel_row = objectPixels[i].row;

			objectPixels[i].col = render_column - pixel_row;
			objectPixels[i].row = render_row + pixel_column;
        }
    }

    return;
}

/***********************************************************************************************************************
 * @param rotate_iteration - rotate object inside frame buffer counter clockwise by 90 degrees, <iteration> times
 * Note: You can assume the frame will always be square and you will be rotating the entire image
 **********************************************************************************************************************/
__attribute__((always_inline)) void processRotateCCW(int rotate_iteration) {
    if (rotate_iteration < 0){ // rotate -90 CCW == rotate 90 CW
        processRotateCW(-1 * rotate_iteration);
    } else { // rotate 90 CCW == rotate 270 CW
        processRotateCW(3 * rotate_iteration);
    }

    return;
}

/***********************************************************************************************************************
 * Flip entire image about x-axis in the middle
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMirrorX() {    
    int i;
    
    for (i = 0; i < numObjectPixels; i++) {
	    objectPixels[i].row = (_height - objectPixels[i].row - 1);
    }

    return;
}

/***********************************************************************************************************************
 * Flip entire image about y-axis in the middle
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMirrorY() {    
    int i;
    
    for (i = 0; i < numObjectPixels; i++) {
	    objectPixels[i].col = (_width - objectPixels[i].col - 1);
	}

    return;
}

__attribute__((always_inline)) unsigned char *renderFrame(unsigned char *buffer_frame) {
    memset(buffer_frame, 255, _width*_height*3);

    int i;
    int bufferFrameIdx;

    for (i = 0; i < numObjectPixels; i++) {        
        bufferFrameIdx = 3*(objectPixels[i].row * _width + objectPixels[i].col);
        memcpy(buffer_frame + bufferFrameIdx, objectPixels + i, 3);
    }

    return buffer_frame;
}

__attribute__((always_inline)) unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info(){
    // Please modify this field with something interesting
    char team_name[] = "malloc-enjoyer";

    // Please fill in your information
    char student_first_name[] = "prerak";
    char student_last_name[] = "chaudhari";
    char student_student_number[] = "1005114760";

    // Printing out team information
    printf("*******************************************************************************************************\n");
    printf("Team Information:\n");
    printf("\tteam_name: %s\n", team_name);
    printf("\tstudent_first_name: %s\n", student_first_name);
    printf("\tstudent_last_name: %s\n", student_last_name);
    printf("\tstudent_student_number: %s\n", student_student_number);
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          You can modify anything else in this file
 ***********************************************************************************************************************
 * @param sensor_values - structure stores parsed key value pairs of program instructions
 * @param sensor_values_count - number of valid sensor values parsed from sensor log file or commandline console
 * @param frame_buffer - pointer pointing to a buffer storing the imported  24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param grading_mode - turns off verification and turn on instrumentation
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void implementation_driver(struct kv *sensor_values, int sensor_values_count, unsigned char *frame_buffer,
                           unsigned int width, unsigned int height, bool grading_mode) {

    objectPixels = (Pixel *)malloc(sizeof(Pixel)*width*height);
    numObjectPixels = 0;

    _width = width;
    _height = height;

    int pixelLocation = 0;

    short row, col;
    unsigned char r, g, b;

    for (row = 0; row < width; row++) {
        for (col = 0; col < height; col++) {
            r = frame_buffer[pixelLocation];
            g = frame_buffer[pixelLocation + 1];
            b = frame_buffer[pixelLocation + 2];            
            
            if (r + g + b < 765) {
                objectPixels[numObjectPixels].row = row;
                objectPixels[numObjectPixels].col = col;
                // memcpy(objectPixels + numObjectPixels, frame_buffer + pixelLocation, 3);
                objectPixels[numObjectPixels].r = r;
                objectPixels[numObjectPixels].g = g;
                objectPixels[numObjectPixels].b = b;

                numObjectPixels += 1;
            }

            pixelLocation += 3;
        }
    }
    
    int processed_frames = 0;

    for (int sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx++) {
        switch(hash(sensor_values[sensorValueIdx].key)) {
            case W:     processMoveUp(sensor_values[sensorValueIdx].value);     break;
            case A:     processMoveLeft(sensor_values[sensorValueIdx].value);   break;
            case S:     processMoveDown(sensor_values[sensorValueIdx].value);   break;
            case D:     processMoveRight(sensor_values[sensorValueIdx].value);  break;
            case CW:    processRotateCW(sensor_values[sensorValueIdx].value);   break;
            case CCW:   processRotateCCW(sensor_values[sensorValueIdx].value);  break;
            case MX:    processMirrorX();                                       break;
            case MY:    processMirrorY();                                       break;
        }

        processed_frames += 1;

        if (processed_frames % 25 == 0) {
            frame_buffer = renderFrame(frame_buffer);
            verifyFrame(frame_buffer, width, height, grading_mode);

            if (sensorValueIdx + 25 >= sensor_values_count) break;
        }
    }

    free(objectPixels);

    return;
}
