#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

typedef enum {
    CMD_NONE,
    CMD_ADD,
    CMD_LIST,
    CMD_REMOVE,
    CMD_FILTER,
    CMD_UPDATE
} CommandType;

typedef struct report {
    int report_id;
    char inspector_name[50];
    double X, Y;
    char category[50];
    int severity;
    time_t timestamp;
    char description[256];
}report;

typedef struct current_context {
    char user[50];
    char role[50];
    CommandType cmd_type;
    char district[100];
}current_context;

void read_report_data(report* new_report, const char* declared_user) {
    new_report->timestamp = time(NULL);
    strncpy(new_report->inspector_name, declared_user, 50);
    new_report->inspector_name[49] = '\0';

    while (1) {
        printf("Insert X Coordinate: ");
        if (scanf("%lf", &new_report->X) == 1) break;
        printf("Invalid data\n");
        while (getchar() != '\n');
    }
    while (1) {
        printf("Insert Y Coordinate: ");
        if (scanf("%lf", &new_report->Y) == 1) break;
        printf("Invalid data\n");
        while (getchar() != '\n');
    }

    printf("Insert category (road/lighting/flooding/other): ");
    scanf("%30s", new_report->category);
    while (getchar() != '\n');

    while (1) {
        printf("Insert Severity Level (1/2/3): ");
        if (scanf("%d", &new_report->severity) == 1 && new_report->severity >= 1 && new_report->severity <= 3) break;
        printf("Invalid data\n");
        while (getchar() != '\n');
    }
    while (getchar() != '\n');

    printf("Insert description: ");
    if (fgets(new_report->description, 256, stdin) != NULL) {
        new_report->description[strcspn(new_report->description, "\n")] = 0;
    }
}

void parse_arguments(int argc, char* argv[], current_context* con) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            strncpy(con->role, argv[++i], 50);
            con->role[49] = '\0';
        }
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            strncpy(con->user, argv[++i], 50);
            con->user[49] = '\0';
        }
        else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            con->cmd_type = CMD_ADD;
            strncpy(con->district, argv[++i], 100);
            con->district[99] = '\0';
        }
        else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            con->cmd_type = CMD_LIST;
            strncpy(con->district, argv[++i], 100);
            con->district[99] = '\0';
        }
    }
}

void create_file_structure(char* dir_name) {
    if (mkdir(dir_name, 0750) == -1) {
        if (errno != EEXIST) {
            printf("Critical error when creating the folder\n");
            exit(1);
        }
        else {
            chmod(dir_name, 0750);
        }
    }
    char path[256];
    int f;

    snprintf(path, sizeof(path), "%s/reports.dat", dir_name);
    f = open(path, O_WRONLY | O_CREAT, 0664);
    if (f != -1) {
        chmod(path, 0664);
        close(f);
    }
    snprintf(path, sizeof(path), "%s/district.cfg", dir_name);
    f = open(path, O_WRONLY | O_CREAT, 0664);
    if (f != -1) {
        chmod(path, 0640);
        close(f);
    }
    snprintf(path, sizeof(path), "%s/logged_district", dir_name);
    f = open(path, O_WRONLY | O_CREAT, 0664);
    if (f != -1) {
        chmod(path, 0644);
        close(f);
    }
    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", dir_name);
    char target[256];
    snprintf(target, sizeof(target), "%s/reports.dat", dir_name);

    unlink(link_name);

    if (symlink(target, link_name) == -1) {
        perror("Error creating symlink");
    }

}
void write_report_data(report* new_report, const char* district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int f = open(path, O_RDWR | O_APPEND);
    if (f == -1) {
        perror("Error opening reports.dat file");
        exit(1);
    }
    off_t size = lseek(f, 0, SEEK_END);
    if (size == -1) {
        perror("Error seeking in file");
        close(f);
        exit(1);
    }
    new_report->report_id = (size / sizeof(report)) + 1;
    if (write(f, new_report, sizeof(report)) != sizeof(report)) {
        perror("Error writing report");
        exit(1);
    }
    else {
        printf("Succesfully saved report\n");
    }
    close(f);
}
void permission_string_converter(mode_t mode, char* str) {
    str[0] = (S_ISDIR(mode)) ? 'd' : '-';
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}
int verify_system_integrity(const char* district, const char* role, char action) {
    struct stat st_link, st_file;
    char link_path[256], data_path[256];
    snprintf(link_path, sizeof(link_path), "active_reports-%s", district);
    snprintf(data_path, sizeof(data_path), "%s/reports.dat", district);

    if (lstat(link_path, &st_link) == 0) {
        if (S_ISLNK(st_link.st_mode)) {
            if (stat(link_path, &st_file) == -1) {
                printf("Dangling link detected for %s\n", link_path);
                return 0;
            }
        }
    }
    if (stat(data_path, &st_file) == -1) {
        perror("File is missing");
        return 0;
    }
    if (strcmp(role, "manager") == 0) {
        mode_t mask = (action == 'w') ? S_IWUSR : S_IRUSR;
        if (!(st_file.st_mode & mask)) {
            printf("Manager lacks permission %c to perform this operation\n", action);
            return 0;
        }
    }
    else if (strcmp(role, "inspector") == 0) {
        mode_t mask = (action == 'w') ? S_IWGRP : S_IRGRP;
        if (!(st_file.st_mode & mask)) {
            printf("Inspector lacks permission %c to perform this operation\n", action);
            return 0;
        }
    }
    return 1;
}
void list_reports(const char* district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("Error getting file metadata");
        return;
    }
    char perms[11];
    permission_string_converter(st.st_mode, perms);
    printf("File Info for %s\n", path);
    printf("Permissions: %s | Size: %ld bytes\n", perms, (long)st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));
    printf("-------------------------\n");
    int f = open(path, O_RDONLY);
    if (f == -1) {
        perror("Error opening file");
        return;
    }
    report temp;
    while (read(f, &temp, sizeof(report)) == sizeof(report)) {
        printf("ID: %d\nInspector: %s\nCategory: %s\nCoordinates:(%.2f, %.2f)\nSeverity: %d\nTimestamp:%sDescription: %s\n",
            temp.report_id, temp.inspector_name, temp.category, temp.X, temp.Y, temp.severity, ctime(&temp.timestamp), temp.description);
        printf("-------------------------\n");
    }
    close(f);
}
int main(int argc, char* argv[]) {
    current_context con = { 0 };
    parse_arguments(argc, argv, &con);
    if (con.cmd_type == CMD_NONE || strlen(con.user) == 0 || strlen(con.role) == 0) {
        printf("Incorrect usage of commands\n");
        return 1;
    }
    switch (con.cmd_type) {
    case CMD_ADD: {
        create_file_structure(con.district);
        if (verify_system_integrity(con.district, con.role, 'w')) {
            report new_report;
            memset(&new_report, 0, sizeof(report));
            read_report_data(&new_report, con.user);
            write_report_data(&new_report, con.district);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    case CMD_LIST: {
        if (verify_system_integrity(con.district, con.role, 'r')) {
            list_reports(con.district);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    default: {
        printf("Command not recognized\n");
        break;
    }
    }
    return 0;
}
