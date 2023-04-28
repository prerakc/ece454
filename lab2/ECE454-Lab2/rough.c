unsigned long hash(unsigned char *str);

// Constants to keep track of clock-wise rotations
#define ROTATED_UP          0
#define ROTATED_RIGHT       1
#define ROTATED_DOWN        2
#define ROTATED_LEFT        3

// Map operations to constants
#define W   177660
#define A   177638
#define S   177656
#define D   177641
#define CW  5862207 
#define CCW 193452258
#define MX  5862538
#define MY  5862539

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}

void combine_transformations(struct kv *sensor_values, int sensorValueIdx) {
    short deltaX = 0;
    short deltaY = 0;
    bool flippedX = false;
    bool flippedY = false;
    signed char rotation = ROTATED_UP;

    for (; sensorValueIdx < sensorValueIdx + 25; sensorValueIdx++) {
        int sensorValue = sensor_values[sensorValueIdx].value;

        if (sensorValue == 0) continue;

        switch(hash(sensor_values[sensorValueIdx].key)) {
            case W:
            case A:
            case S:
            case D:
            case CW:
                rotation = (rotation + sensorValue) % 4;

            case CCW:
                if (sensorValue > 0) {
                    rotation = (rotation + 3*sensorValue) % 4;
                } else {
                    rotation = (rotation + (-1)*sensorValue) % 4;
                }

            case MX:
            case MY:
        }

    }
}

void combine_transformations(struct kv *sensor_values, int sensorValueIdx) {
    int aggregated_matrix[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    int transformation_matrix[3][3];

    for (; sensorValueIdx < sensorValueIdx + 25; sensorValueIdx++) {
        int sensorKey
        int sensorValue = sensor_values[sensorValueIdx].value;

        if (sensorValue == 0) continue;

        switch(hash(sensor_values[sensorValueIdx].key)) {
            case W:
                int current_matrix[][] = {{1, 0, 0}, {0, 1, -1 * sensorValue}, {0, 0, 1}};
                memcpy(transformation_matrix, current_matrix, 4*9);

            case A:
                int current_matrix[][] = {{1, 0, -1 * sensorValue}, {0, 1, 0}, {0, 0, 1}};
                memcpy(transformation_matrix, current_matrix, 4*9);

            case S:
                int current_matrix[][] = {{1, 0, 0}, {0, 1, sensorValue}, {0, 0, 1}};
                memcpy(transformation_matrix, current_matrix, 4*9);

            case D:
                int current_matrix[][] = {{1, 0, sensorValue}, {0, 1, 0}, {0, 0, 1}};
                memcpy(transformation_matrix, current_matrix, 4*9);

            case CW:
                int rotation = sensorValue % 4;

                if (rotation == 0) continue;

                if (rotation < 0) {
                    rotation = 4 + rotation;
                }

                if (rotation == 1) {
                    int current_matrix[][] = {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}};
                    memcpy(transformation_matrix, current_matrix, 4*9);
                } else if (rotation == 2) {
                    int current_matrix[][] = {{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}};
                    memcpy(transformation_matrix, current_matrix, 4*9);
                } else {
                    int current_matrix[][] = {{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}};
                    memcpy(transformation_matrix, current_matrix, 4*9);
                }

            case CCW:
                int rotation = sensorValue % 4;

                if (rotation == 0) continue;

                if (rotation > 0) {
                    rotation = 4 - rotation;
                } else {
                    rotation *= -1;
                }

                if (rotation == 1) {
                    int current_matrix[][] = {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}};
                    memcpy(transformation_matrix, current_matrix, 4*9);
                } else if (rotation == 2) {
                    int current_matrix[][] = {{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}};
                    memcpy(transformation_matrix, current_matrix, 4*9);
                } else {
                    int current_matrix[][] = {{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}};
                    memcpy(transformation_matrix, current_matrix, 4*9);
                }

            case MX:
            case MY:
        }

    }
}