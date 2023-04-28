#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utilities.h"                // DO NOT REMOVE this line
#include "implementation_reference.h" // DO NOT REMOVE this line

// Hash codes for sensor operations
#define W 177660
#define A 177638
#define S 177656
#define D 177641
#define CW 5862207
#define CCW 193452258
#define MX 5862538
#define MY 5862539

// Declariations
__attribute__((always_inline)) void processMoveUp(int offset);
__attribute__((always_inline)) void processMoveRight(int offset);
__attribute__((always_inline)) void processMoveDown(int offset);
__attribute__((always_inline)) void processMoveLeft(int offse);
__attribute__((always_inline)) void processRotateCW(int rotate_iteration);
__attribute__((always_inline)) void processRotateCCW(int rotate_iteration);
__attribute__((always_inline)) void processMirrorX();
__attribute__((always_inline)) void processMirrorY();
__attribute__((always_inline)) void applyTransformation(int transformation_matrix[3][3]);
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

int identity_matrix[3][3] = {{1, 0, 0},
                             {0, 1, 0},
                             {0, 0, 1}
                            };
int aggregated_transformation[3][3];
int matrix_buffer[3][3];

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image up
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveUp(int offset) {
    int translate_up[3][3] = {{1, 0, -offset},
                              {0, 1,       0},
                              {0, 0,       1}
                             };

    applyTransformation(translate_up);

    return;
}

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image left
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveRight(int offset) {
    int translate_right[3][3] = {{1, 0,      0},
                                 {0, 1, offset},
                                 {0, 0,      1}
                                };

    applyTransformation(translate_right);

    return;
}

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image up
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveDown(int offset) {
    int translate_down[3][3] = {{1, 0, offset},
                                {0, 1,      0},
                                {0, 0,      1}
                               };

    applyTransformation(translate_down);

    return;
}

/***********************************************************************************************************************
 * @param offset - number of pixels to shift the object in bitmap image right
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMoveLeft(int offset) {
    int translate_left[3][3] = {{1, 0,       0},
                                {0, 1, -offset},
                                {0, 0,       1}
                               };

    applyTransformation(translate_left);

    return;
}

/***********************************************************************************************************************
 * @param rotate_iteration - rotate object inside frame buffer clockwise by 90 degrees, <iteration> times
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note: You can assume the frame will always be square and you will be rotating the entire image
 **********************************************************************************************************************/
__attribute__((always_inline)) void processRotateCW(int rotate_iteration) {
    // handle negative offsets
    if (rotate_iteration < 0) rotate_iteration *= -3;

    rotate_iteration %= 4;

    if (rotate_iteration == 0) return;

    int rotate_90[3][3]  = {{ 0, 1,          0},
                            {-1, 0, _width - 1},
                            { 0, 0,          1}
                           };
    int rotate_180[3][3] = {{-1,  0, _width - 1},
                            { 0, -1, _width - 1},
                            { 0,  0,          1}
                           };
    int rotate_270[3][3] = {{0, -1, _width - 1},
                            {1,  0,          0},
                            {0,  0,          1}
                           };

    switch (rotate_iteration) {
        case 1:     applyTransformation(rotate_90);     break;
        case 2:     applyTransformation(rotate_180);    break;
        case 3:     applyTransformation(rotate_270);    break;
    }

    return;
}

/***********************************************************************************************************************
 * @param rotate_iteration - rotate object inside frame buffer counter clockwise by 90 degrees, <iteration> times
 * Note: You can assume the frame will always be square and you will be rotating the entire image
 **********************************************************************************************************************/
__attribute__((always_inline)) void processRotateCCW(int rotate_iteration) {
    if (rotate_iteration == 0) return;

    // rotate -90 CCW == rotate 90 CW
    if (rotate_iteration < 0) processRotateCW(-1 * rotate_iteration);
    // rotate 90 CCW == rotate 270 CW
    else processRotateCW(3 * rotate_iteration);
    
    return;
}

/***********************************************************************************************************************
 * Flip the entire image about the x-axis in the middle
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMirrorX() {
    int reflect_x[3][3] = {{-1, 0, _height - 1},
                           { 0, 1,           0},
                           { 0, 0,           1}
                          };

    applyTransformation(reflect_x);

    return;
}

/***********************************************************************************************************************
 * Flip the entire image about the y-axis in the middle
 **********************************************************************************************************************/
__attribute__((always_inline)) void processMirrorY() {
    int reflect_y[3][3] = {{1,  0,          0},
                           {0, -1, _width - 1},
                           {0,  0,          1}
                          };

    applyTransformation(reflect_y);

    return;
}

/***********************************************************************************************************************
 * Combine the given transformation matrix with the aggregated transformations matrix
 * @param transformation_matrix - transformation matrix to combine with
 * Note:
 * this function assumes that all transformation matrices are 3x3 because the
 * image pixels are 2D vectors projected into 3D space using homogeneous coordinates
 **********************************************************************************************************************/
__attribute__((always_inline)) void applyTransformation(int transformation_matrix[3][3]) {
    memset(matrix_buffer, 0, 9 * sizeof(int));

    int i, j, k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 3; k++) matrix_buffer[i][j] += transformation_matrix[i][k] * aggregated_transformation[k][j];
        }
    }

    memcpy(aggregated_transformation, matrix_buffer, 9 * sizeof(int));

    return;
}

/***********************************************************************************************************************
 * Apply the aggregated transformation matrix onto each non-white pixel and render a new frame
 * @param frame_buffer - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @return - pointer pointing to a buffer storing a modified 24-bit bitmap image
 **********************************************************************************************************************/
__attribute__((always_inline)) unsigned char *renderFrame(unsigned char *buffer_frame) {
    int objectIdx, newRow, newCol;

    for (objectIdx = 0; objectIdx < numObjectPixels; objectIdx++) {
        newRow = aggregated_transformation[0][0] * objectPixels[objectIdx].row + aggregated_transformation[0][1] * objectPixels[objectIdx].col + aggregated_transformation[0][2];
        newCol = aggregated_transformation[1][0] * objectPixels[objectIdx].row + aggregated_transformation[1][1] * objectPixels[objectIdx].col + aggregated_transformation[1][2];

        objectPixels[objectIdx].row = newRow;
        objectPixels[objectIdx].col = newCol;
    }

    memset(buffer_frame, 255, _width * _height * 3);

    int i;
    int bufferFrameIdx;

    for (i = 0; i < numObjectPixels; i++) {
        bufferFrameIdx = 3 * (objectPixels[i].row * _width + objectPixels[i].col);
        memcpy(buffer_frame + bufferFrameIdx, objectPixels + i, 3);
    }

    return buffer_frame;
}


/***********************************************************************************************************************
 * Hash a given string
 * @param str - an array of characters containing the string
 * @return - the string's hash code
 **********************************************************************************************************************/
__attribute__((always_inline)) unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++) hash = ((hash << 5) + hash) + c;

    return hash;
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info()
{
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
                           unsigned int width, unsigned int height, bool grading_mode)
{
    objectPixels = (Pixel *)malloc(sizeof(Pixel) * width * height);
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
                memcpy(objectPixels + numObjectPixels, frame_buffer + pixelLocation, 3);

                numObjectPixels += 1;
            }

            pixelLocation += 3;
        }
    }

    int processed_frames = 0;
    int sensorValueIdx, operationIdx;

    for (sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx += 25) {
        if (sensorValueIdx + 25 > sensor_values_count) break;

        memcpy(aggregated_transformation, identity_matrix, 9 * sizeof(int));

        for (operationIdx = 0; operationIdx < 25; operationIdx++) {
            switch (hash(sensor_values[sensorValueIdx + operationIdx].key)) {
                case W:     processMoveUp(sensor_values[sensorValueIdx + operationIdx].value);      break;
                case A:     processMoveLeft(sensor_values[sensorValueIdx + operationIdx].value);    break;
                case S:     processMoveDown(sensor_values[sensorValueIdx + operationIdx].value);    break;
                case D:     processMoveRight(sensor_values[sensorValueIdx + operationIdx].value);   break;
                case CW:    processRotateCW(sensor_values[sensorValueIdx + operationIdx].value);    break;
                case CCW:   processRotateCCW(sensor_values[sensorValueIdx + operationIdx].value);   break;
                case MX:    processMirrorX();                                                       break;
                case MY:    processMirrorY();                                                       break;
            }
        }

        frame_buffer = renderFrame(frame_buffer);
        verifyFrame(frame_buffer, width, height, grading_mode);
    }

    free(objectPixels);

    return;
}
