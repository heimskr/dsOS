#define VIDEO_START 0xb8000
#define VGA_LIGHT_GRAY 7

static void print_string(const char *str) {
	unsigned char *video = ((unsigned char *)VIDEO_START);
	while(*str != '\0') {
		*(video++) = *str++;
		*(video++) = VGA_LIGHT_GRAY;
	}
}

void kernel_main(void) {
	print_string("Hello World!");
	while(1);
}
