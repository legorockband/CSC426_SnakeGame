// Keefe Johnson
// Instant OTTER Programmer
/* Version 0.4 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;

volatile sig_atomic_t ctrlc = 0;

int serial_port;
uint32_t data_cksum;

#define APPLY_CMD_CKSUM(x) ((x) << 8) ^ ((x) & 0xFF) ^ (((x) & 0xFF00) >> 8) ^ (((x) & 0xFF0000) >> 16)

#define TIMEOUT_MSEC 5000

#define CMD_RESET_ON  0x0FF000
#define CMD_RESET_OFF 0x0FF001
#define CMD_WRITE_MEM 0x0FF002

void exit_handler(void) {
    fprintf(stderr, "Restoring serial port settings... ");
    tcsetattr(serial_port, TCSANOW, &saved_attributes);
    fprintf(stderr, "closing port... ");
    close(serial_port);
    fprintf(stderr, "closed\n");
}

void open_serial(char *path) {
    struct termios tattr;

    fprintf(stderr, "Opening serial port... ");

    if ((serial_port = open(path, O_RDWR)) == -1) {
        fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Make sure the port is a terminal. */
    if (!isatty(serial_port)) {
        fprintf(stderr, "Not a terminal.\n");
        exit(EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    fprintf(stderr, "reading old settings... ");
    tcgetattr(serial_port, &saved_attributes);
    atexit(exit_handler);

    /* Set the funny terminal modes. */
    tcgetattr(serial_port, &tattr);
    tattr.c_oflag &= ~OPOST;  // raw output
    tattr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | ECHONL | IEXTEN);  // raw input
    tattr.c_cflag &= ~(CSIZE | PARENB | CSTOPB);  // 8N1 ...
    tattr.c_cflag |= (CS8 | CLOCAL | CREAD);      // ... and enable without ownership
    tattr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);  // more raw input, and no software flow control
    tattr.c_cc[VMIN] = 4;
    tattr.c_cc[VTIME] = 10;  // allow up to 1.0 secs between bytes received
    cfsetospeed(&tattr, B115200);
    fprintf(stderr, "flushing transmit buffer and setting raw mode... ");
    tcsetattr(serial_port, TCSAFLUSH, &tattr);

    fprintf(stderr, "ready to communicate\n");
}

int open_file(char *path, off_t *num_words) {
    int file;
    struct stat s;

    if ((file = open(path, O_RDONLY)) == -1) {
        fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fstat(file, &s) == -1) {
        perror("fstat(file)");
        exit(EXIT_FAILURE);
    }

    *num_words = (s.st_size + 3) / 4;  // round up to nearest 4 bytes
    return file;
}

uint32_t file_read_word(int file) {
    uint32_t w;
    ssize_t br;
    w = 0;
    br = read(file, &w, 4);
    if (ctrlc) exit(EXIT_FAILURE);
    if (br == -1) {
        perror("read(file)");
        exit(EXIT_FAILURE);
    }
    if (br == 0) {
        fprintf(stderr, "File size changed\n");
        exit(EXIT_FAILURE);
    }
    return w;
}

void send_word(uint32_t w) {
    ssize_t bw;
    w = htonl(w);
    bw = write(serial_port, &w, 4);
    if (ctrlc) exit(EXIT_FAILURE);
    if (bw == -1) {
        perror("write(serial)");
        exit(EXIT_FAILURE);
    }
    if (bw != 4) {
        fprintf(stderr, "Wrote only %ld of 4 bytes\n", bw);
        exit(EXIT_FAILURE);
    }
}

uint32_t recv_word(void) {
    uint32_t w;
    ssize_t br;
    w = 0;
    br = read(serial_port, &w, 4);
    if (ctrlc) exit(EXIT_FAILURE);
    if (br == -1) {
        perror("read(serial)");
        exit(EXIT_FAILURE);
    }
    if (br != 4) {
        fprintf(stderr, "Read only %ld of 4 bytes: 0x%08X\n", br, ntohl(w));
        exit(EXIT_FAILURE);
    }
    return ntohl(w);
}

int wait_readable(int msec) {
    int r;
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(serial_port, &set);
    timeout.tv_sec = msec / 1000;
    timeout.tv_usec = (msec % 1000) * 1000;
    r = select(serial_port + 1, &set, NULL, NULL, &timeout);
    if (ctrlc) exit(EXIT_FAILURE);
    if (r == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }
    return FD_ISSET(serial_port, &set);
}

void expect_word(uint32_t expect) {
    uint32_t r;
    if (wait_readable(TIMEOUT_MSEC)) {
        if ((r = recv_word()) != expect) {
            fprintf(stderr, "Expected 0x%08X but received 0x%08X\n", expect, r);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Expected 0x%08X but received nothing\n", expect);
        exit(EXIT_FAILURE);
    }
}

void expect_timeout(void) {
    if (wait_readable(TIMEOUT_MSEC)) {
        fprintf(stderr, "Expected timeout but received 0x%08X\n", recv_word());
        exit(EXIT_FAILURE);
    }
}

void enter_reset_state(void) {
    uint32_t cmd;
    cmd = APPLY_CMD_CKSUM(CMD_RESET_ON);  // rst <= 1
    fprintf(stderr, "Putting MCU into reset state... ");
    send_word(cmd);
    expect_word(cmd);  // expect cmd echo
    fprintf(stderr, "success\n");
}

void exit_reset_state(void) {
    uint32_t cmd;
    cmd = APPLY_CMD_CKSUM(CMD_RESET_OFF);  // rst <= 0
    fprintf(stderr, "Taking MCU out of reset state... ");
    send_word(cmd);
    expect_word(cmd);  // expect cmd echo
    fprintf(stderr, "success\n");
}

void start_mem_write(uint32_t start_addr, uint32_t num_words) {
    uint32_t cmd;
    cmd = APPLY_CMD_CKSUM(CMD_WRITE_MEM);
    fprintf(stderr, "Starting mem write... ");
    send_word(cmd);
    expect_word(cmd);  // expect cmd echo
    fprintf(stderr, "sending address and length... ");
    data_cksum = 0;
    send_word(start_addr);
    data_cksum ^= start_addr;
    send_word(num_words);
    data_cksum ^= num_words;
    fprintf(stderr, "sending data... ");
}

void mem_write_word(uint32_t word) {
    send_word(word);
    data_cksum ^= word;
}

void verify_mem_write(void) {
    fsync(serial_port);
    fprintf(stderr, "verifying checksum... ");
    expect_word(data_cksum);
    fprintf(stderr, "success\n");
}

void flush_and_show_progress(long long complete, long long total) {
    static long long old_progress;
    long long progress;

    if (complete == -1) {
        fprintf(stderr, "             ");
    } else {
        progress = complete * 100 / total;
        if (progress != old_progress) {
            fsync(serial_port);
            fprintf(stderr, "\b\b\b\b\b\b\b\b\b\b\b\b\b%3lld%% done... ", progress);
            old_progress = progress;
        }
    }
}

void ctrlc_handler(int s) {
    ctrlc = 1;
}

void register_ctrlc_handler(void) {
    struct sigaction sa = {0};
    sa.sa_handler = ctrlc_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    off_t num_words;
    int file;
    off_t i;
    uint32_t word;

    register_ctrlc_handler();

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mem.bin> <serial>\n", argv[0]); 
        exit(EXIT_FAILURE);
    }

    file = open_file(argv[1], &num_words);
    fprintf(stderr, "File length is %ld words\n", num_words);
    open_serial(argv[2]);

    enter_reset_state();
    start_mem_write(0, num_words);
    flush_and_show_progress(-1, -1);
    for (i = 0; i < num_words; i++) {
        word = file_read_word(file);
        mem_write_word(word);
        flush_and_show_progress(i + 1, num_words);
    }
    verify_mem_write();
    exit_reset_state();

    fprintf(stderr, "Successfully programmed!\n");
    close(file);
    // serial port will be closed by exit handler
}
