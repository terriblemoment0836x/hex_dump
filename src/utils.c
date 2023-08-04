#include "utils.h"

uint64_t get_file_size(char * file_path) {
    HANDLE hfile = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if ( hfile == INVALID_HANDLE_VALUE) {
        perror("Error calculating the size of the file.");
        exit(1);
    }
    
    LARGE_INTEGER size;
    GetFileSizeEx(hfile, &size);

    CloseHandle(hfile);

    return size.QuadPart;
}
uint8_t dump_bin(FILE *fd, uint32_t column_size, uint32_t column_count,
                 bool show_address, bool show_ascii, bool enable_colors,
                 uint64_t file_size, enum num_types number_type)
{
    assert(fd != NULL);
    assert(column_count != 0 && column_size != 0);

    const uint32_t line_size = column_count*column_size;
    uint64_t buffer_size = file_size + line_size;
    buffer_size -= buffer_size % line_size;
    
    uint32_t bytes_read;
    uint32_t buffer_index;
    uint32_t line_end;
    uint8_t input_buffer[buffer_size];
    uint8_t t = 0;
    const uint8_t ascii_distance = 8;
    uint32_t file_position = 0;

    do {
        bytes_read = fread(input_buffer, sizeof(char), buffer_size, fd);
        buffer_index = 0;

        while ( buffer_index < bytes_read ) {
            line_end = min(buffer_index + line_size, bytes_read);

            if (show_address == true) {
                t = printf("0x%#08x:\t", file_position + buffer_index);
            }
            print_line_dump(input_buffer, buffer_index, line_end, column_size, number_type);

            if (show_ascii == true) {
                printf("\r");
                printf("%c[%dC", 0x1B, number_type * line_size + column_count + ascii_distance + t);

                print_line_ascii(input_buffer, buffer_index, line_end);
            }
            printf("\n");

            buffer_index += line_size;
        }
        file_position += buffer_index;

    } while ( bytes_read == buffer_size );
    if (show_address == true) printf("0x%#08x:\t", file_position);

    assert(!ferror(fd));

    return 0;
    
}
uint8_t print_line_dump(uint8_t * buff, uint32_t start, uint32_t end, uint32_t column_size, enum num_types number_type) {
    static bool color_enabled = false;
    static uint8_t color_code = 33;

    char printf_template[20];
    switch(number_type) {
        case D_HEXADECIMAL:
            // strcpy(printf_template, "%c");
            strcpy(printf_template, "%02x");
            break;
        case D_OCTAL:
            strcpy(printf_template, "%03o");
            break;
        case D_BINARY:
            strcpy(printf_template, "%c%c%c%c%c%c%c%c");
            break;
        default:
            return 2;
    }
        for (unsigned int i = start; i < end; i++) {
            if (configure_color(buff[i], color_code, i == end - 1) == true ) {
                color_code = (color_code == 37) ? 33 : color_code + 1;
            }
            if ( number_type == D_BINARY)
                printf(printf_template, PRINTF_BIN_ARG(buff[i]));
            else
                printf(printf_template, buff[i]);
            
            if ( (i+1) % column_size == 0) printf(" ");
        }
        disable_color();
        return 0;
}

void print_line_ascii(uint8_t * buff, uint32_t start, uint32_t end) {
    static uint8_t color_code = 33;
    char temp = buff[end];
    buff[end] = '\0';
    printf("[");
    while ( start < end ) {
            if (configure_color(buff[start], color_code, start == end - 1) == true ) {
                color_code = (color_code == 37) ? 33 : color_code + 1;
            }
            if (buff[start] >= 36 && buff[start] <= 126) {
                printf("%c", buff[start]);
            }
            else {
                printf(".");
            }
            ++start;
    }
    disable_color();
    printf("]");
    buff[end] = temp;
}

void disable_color() {
    printf("%c[0m", 0x1b);
}

bool configure_color(uint8_t c, uint8_t color_code, uint8_t reset) {
    static bool color_enabled = false;
            if (c >= 36 && c <= 126) {
                if (color_enabled != true) {
                    color_enabled = true;
                    printf("%c[%dm", 0x1b, color_code);
                }
                color_enabled = ! reset;
                return false;
            }

            color_enabled = false;
            disable_color();
            return true;
}