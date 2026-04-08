#include "../include/dhcp.h"

#define LOG_FILE "logs/dhcp.log"
#define MAX_LOG_SIZE 100000   // ~100 KB

// -------- CHECK FILE SIZE --------
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return 0;
}

// -------- ROTATE LOG --------
void rotate_logs() {
    long size = get_file_size(LOG_FILE);

    if (size >= MAX_LOG_SIZE) {
        // Rename old log → dhcp.log.1
        rename("logs/dhcp.log", "logs/dhcp.log.1");
    }
}

// -------- LOG FUNCTION --------
void log_event(const char *level, const char *msg) {

    rotate_logs();  // check before writing

    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;

    // Time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char time_buf[64];
    strftime(time_buf, sizeof(time_buf),
             "%Y-%m-%d %H:%M:%S", t);

    // Write log
    fprintf(fp, "[%s] %s: %s\n", time_buf, level, msg);

    fclose(fp);

    // Optional: also print to console
    printf("[%s] %s: %s\n", time_buf, level, msg);
}
