CC=gcc
CFLAGS=-O2 -Wall -Wextra
LDFLAGS=-lwiringPi -lm

BUILD_DIR=build
SRC_DIR=src

all: $(BUILD_DIR)/server $(BUILD_DIR)/client $(BUILD_DIR)/ppg

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/server: $(SRC_DIR)/server.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/client: $(SRC_DIR)/client.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/ppg: $(SRC_DIR)/ppg.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
