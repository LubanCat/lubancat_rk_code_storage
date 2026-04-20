out_file_name = spi_oled_app

all: spi_oled_app.c oled_code_table.c
	aarch64-linux-gnu-gcc $^ -o $(out_file_name)


.PHONY: clean
clean:
	rm $(out_file_name)

	
