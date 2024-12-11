#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

typedef char i8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef unsigned long u64;

#define PRINT_ERROR(cstring) write(STDERR_FILENO, cstring, sizeof(cstring) - 1)

#pragma pack(1)
struct bmp_header
{
	// Note: header
	i8  signature[2]; // should equal to "BM"
	u32 file_size;
	u32 unused_0;
	u32 data_offset;

	// Note: info header
	u32 info_header_size;
	u32 width; // in px
	u32 height; // in px
	u16 number_of_planes; // should be 1
	u16 bit_per_pixel; // 1, 4, 8, 16, 24 or 32
	u32 compression_type; // should be 0
	u32 compressed_image_size; // should be 0
	// Note: there are more stuff there but it is not important here
};

struct file_content
{
	i8*   data;
	u32   size;
};

struct file_content   read_entire_file(char* filename)
{
	char* file_data = 0;
	unsigned long	file_size = 0;
	int input_file_fd = open(filename, O_RDONLY);
	if (input_file_fd >= 0)
	{
		struct stat input_file_stat = {0};
		stat(filename, &input_file_stat);
		file_size = input_file_stat.st_size;
		file_data = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_file_fd, 0);
		close(input_file_fd);
	}
	return (struct file_content){file_data, file_size};
}

// returns offset
int find_header(struct bmp_header* header, struct file_content *content, int header_color[3], int start)
{
	unsigned int i = start;
	unsigned int limit = (header->width * header->height) * 4 < header->file_size ? (header->width * header->height) * 4 : header->file_size;
	for (; i < limit; i += 4)
	{
		int z = 0;
		for (; z < 3; z++)
		{
			if (header_color[z] < 0)
				continue;
			if ((unsigned char)content->data[i + z] != header_color[z])
				break;
		}
		if (z == 3)
			return i;
	}
	return -1;
}

void decoder(struct bmp_header* header, struct file_content *content)
{
    if (!(header->signature[0] == 'B' && header->signature[1] == 'M'))
    {
        PRINT_ERROR("Signature not BM.");
        return;
    }

    int header_color_start[3] = {127, 188, 217};
    int header_offset = find_header(header, content, header_color_start, header->data_offset);
    if (header_offset == -1)
    {
        PRINT_ERROR("Didn't find header start");
        return;
    }

    int header_end = header_offset + (header->width * 28) + 28;
    int content_len = (unsigned char)content->data[header_end] + (unsigned char)content->data[header_end + 2];

    int i = header_end - 20 - (8 * header->width);
    int written = 0;
    char buffer[4096];
    int buffer_pos = 0;

    while (i > 0 && written < content_len)
    {
        for (int p = 0; p < 6 * 4 && written < content_len; p++)
        {
            unsigned char byte = content->data[i + p];
            if (byte != 0)
            {
                buffer[buffer_pos++] = byte;
                written++;
                if (buffer_pos == sizeof(buffer))
                {
                    write(1, buffer, buffer_pos);
                    buffer_pos = 0;
                }
            }
        }
        i -= header->width * 4;
    }

    if (buffer_pos > 0)
        write(1, buffer, buffer_pos);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		PRINT_ERROR("Usage: decode <input_filename>\n");
		return 1;
	}
	struct file_content file_content = read_entire_file(argv[1]);
	if (file_content.data == NULL)
	{
		PRINT_ERROR("Failed to read file\n");
		return 1;
	}
	struct bmp_header* header = (struct bmp_header*) file_content.data;
	// printf("signature: %.2s\nfile_size: %u\ndata_offset: %u\ninfo_header_size: %u\nwidth: %u\nheight: %u\nplanes: %i\nbit_per_px: %i\ncompression_type: %u\ncompression_size: %u\n", header->signature, header->file_size, header->data_offset, header->info_header_size, header->width, header->height, header->number_of_planes, header->bit_per_pixel, header->compression_type, header->compressed_image_size);
	decoder(header, &file_content);
	return 0;
}