#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define READ_MODE "read"
#define WRITE_MODE "write"
#define REMOVE_MODE "remove"
#define MAX_MESSAGE_LENGTH 100

int main(int argc, char *argv[]) {
    char* mode = argv[1];
    char* file_path = argv[2];
    char* message = argv[3];
    char* message_next_part = argv[4];
    const char anchor_item = '*';
    const char* anchor = "*****";

    const bool is_read_mode = strcmp(mode, READ_MODE) == 0;
    const bool is_write_mode = strcmp(mode, WRITE_MODE) == 0;
    const bool is_remove_mode = strcmp(mode, REMOVE_MODE) == 0;

    if (!is_read_mode && !is_write_mode && !is_remove_mode) {
        printf("You should pass %s or %s or %s mode as a first agrument\n", READ_MODE, WRITE_MODE, REMOVE_MODE);
        exit(1);
    }

    if (!file_path) {
        puts("You should pass file path as a second agrument");
        exit(1);
    }

    if (is_write_mode && !message) {
        puts("You should pass message as a third agrument");
        exit(1);
    }

    if (is_write_mode && message_next_part) {
        puts("You should wrap the message in the double quotes like that: \"hello moto\"");
        exit(1);
    }

    if (is_write_mode && strlen(message) >= MAX_MESSAGE_LENGTH) {
        printf("Message length should be less than %d symbols\n", MAX_MESSAGE_LENGTH);
        exit(1);
    }

    if (is_write_mode && strstr(message, anchor) != NULL) {
        printf("Message should not contains %s\n", anchor);
        exit(1);
    }

    FILE* file;
    const size_t anchor_length = strlen(anchor);
    const size_t shielded_message_max_length = anchor_length * 2 + MAX_MESSAGE_LENGTH;

    file = fopen(file_path, "r+b");

    if (!file) {
        perror("You should pass correct file path as a second agrument");
        exit(1);
    }

    const int file_type_buffer_size = 3;
    unsigned char file_type_buffer[file_type_buffer_size];

    if (fread(file_type_buffer, sizeof(unsigned char), file_type_buffer_size, file) != file_type_buffer_size) {
        perror("Error reading file");
        exit(1);
    }

    const bool is_jpeg = file_type_buffer[0] == 0xff && file_type_buffer[1] == 0xd8 && file_type_buffer[2] == 0xff;

    if (!is_jpeg) {
        puts("You should pass jpeg file as a second agrument");
        exit(1);
    }

    if (is_write_mode) {
        char write_buffer[anchor_length + strlen(message) + anchor_length];

        sprintf(write_buffer, "%s%s%s",anchor, message, anchor);

        size_t write_buffer_size = sizeof(write_buffer);
        char read_buffer[sizeof(write_buffer) / sizeof(write_buffer[0])];
        char check_buffer[anchor_length];

        if (fseek(file, -anchor_length, SEEK_END) != 0) {
            perror("Error writing message");
            exit(1);
        }

        if (fread(&check_buffer, sizeof(char), anchor_length, file) != anchor_length) {
            perror("Error writing message");
            exit(1);
        }

        check_buffer[anchor_length] = '\0';

        if (strcmp(anchor, check_buffer) == 0) {
            printf("File already has a message, remove it before\n");
        } else {
            if (!fputs(write_buffer, file)) {
                perror("Error writing message");
                exit(1);
            }
            printf("Wrote message is: %s\n", message);
        }
    }

    if (is_read_mode) {
        char check_buffer[anchor_length];

        if (fseek(file, -anchor_length, SEEK_END) != 0) {
            perror("Error reading message");
            exit(1);
        }

        if (fread(&check_buffer, sizeof(char), anchor_length, file) != anchor_length) {
            perror("Error reading message");
            exit(1);
        }

        check_buffer[anchor_length] = '\0';

        if (strcmp(anchor, check_buffer) == 0) {
            int draf_message_length = shielded_message_max_length - anchor_length;
            char read_buffer[shielded_message_max_length];
            char draft_buffer[shielded_message_max_length];

            if (fseek(file, -shielded_message_max_length, SEEK_END) != 0) {
                perror("Error reading message");
                exit(1);
            }

            if (fread(&read_buffer, sizeof(char), shielded_message_max_length, file) != shielded_message_max_length) {
                perror("Error reading message");
                exit(1);
            }

            for (size_t i = 0; i < draf_message_length; i++) {
                draft_buffer[i] = read_buffer[i];
            }

            draft_buffer[draf_message_length] = '\0';

            char* last_anchor = strrchr(draft_buffer, anchor_item);
            char* read_message = last_anchor + 1;

            printf("Read message is: %s\n", read_message);
        } else {
            puts("File has no message to read");
        }
    }

    if (is_remove_mode) {
        char check_buffer[anchor_length];

        if (fseek(file, -anchor_length, SEEK_END) != 0) {
            perror("Error removing message");
            exit(1);
        }

        if (fread(&check_buffer, sizeof(char), anchor_length, file) != anchor_length) {
            perror("Error removing message");
            exit(1);
        }

        check_buffer[anchor_length] = '\0';

        if (strcmp(anchor, check_buffer) == 0) {
            int draf_message_length = shielded_message_max_length - anchor_length;
            char read_buffer[shielded_message_max_length];
            char draft_buffer[shielded_message_max_length];

            if (fseek(file, -shielded_message_max_length, SEEK_END) != 0) {
                perror("Error removing message");
                exit(1);
            }

            if (fread(&read_buffer, sizeof(char), shielded_message_max_length, file) != shielded_message_max_length) {
                perror("Error removing message");
                exit(1);
            }

            for (size_t i = 0; i < draf_message_length; i++) {
                draft_buffer[i] = read_buffer[i];
            }

            draft_buffer[draf_message_length] = '\0';

            char* last_anchor = strrchr(draft_buffer, anchor_item);
            char* remove_message = last_anchor + 1;

            size_t remove_message_size = anchor_length * 2 + strlen(remove_message);

            if (fseek(file, -remove_message_size, SEEK_END) != 0) {
                perror("Error removing message");
                exit(1);
            }
            if (ftruncate(fileno(file), ftello(file)) != 0) {
                perror("Error restoring file");
                exit(1);
            }

            printf("Removed message is: %s\n", remove_message);
        } else {
            puts("File has no message to remove");
        }
    }

    fclose(file);

    return 0;
}
