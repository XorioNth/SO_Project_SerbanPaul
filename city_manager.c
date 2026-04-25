#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
// Codes for available commands
typedef enum {
    CMD_NONE,
    CMD_ADD,
    CMD_LIST,
    CMD_VIEW,
    CMD_REMOVE,
    CMD_UPDATE,
    CMD_FILTER
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
//Info about the current execution process, from the command line
typedef struct current_context {
    int report_id;
    int threshold;
    char user[50];
    char role[50];
    CommandType cmd_type;
    char cmd_string_conversion[50];
    char district[100];
    char conditions[20][100]; // 20 conditions max
    int condition_count;
}current_context;

void read_report_data(report* new_report, const char* declared_user) {
    new_report->timestamp = time(NULL);
    strncpy(new_report->inspector_name, declared_user, 50);
    new_report->inspector_name[49] = '\0';
    // Loop, so that we enforce the right data
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
    con->report_id = -1;
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
        else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            con->cmd_type = CMD_VIEW;
            strncpy(con->district, argv[++i], 100);
            con->district[99] = '\0';
            con->report_id = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            con->cmd_type = CMD_REMOVE;
            strncpy(con->district, argv[++i], 100);
            con->district[99] = '\0';
            con->report_id = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            con->cmd_type = CMD_UPDATE;
            strncpy(con->district, argv[++i], 100);
            con->district[99] = '\0';
            con->threshold = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            con->cmd_type = CMD_FILTER;
            strncpy(con->district, argv[++i], 100);
            con->district[99] = '\0';
            con->condition_count = 0;
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                strncpy(con->conditions[con->condition_count], argv[i + 1], 99);
                con->conditions[con->condition_count][99] = '\0';
                con->condition_count++;
                i++;
                // We only process the max number of conditions we can handle
                if (con->condition_count >= 20)
                    break;
            }
        }
    }
}

int create_file_structure(char* dir_name) {
    if (mkdir(dir_name, 0750) == -1) {
        if (errno != EEXIST) {
            printf("Critical error when creating the folder\n");
            return 1;
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
    else
        return 1;
    snprintf(path, sizeof(path), "%s/district.cfg", dir_name);
    f = open(path, O_WRONLY | O_CREAT, 0664);
    if (f != -1) {
        chmod(path, 0640);
        close(f);
    }
    else
        return 1;
    snprintf(path, sizeof(path), "%s/logged_district", dir_name);
    f = open(path, O_WRONLY | O_CREAT, 0664);
    if (f != -1) {
        chmod(path, 0644);
        close(f);
    }
    else
        return 1;
    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", dir_name);
    char target[256];
    snprintf(target, sizeof(target), "%s/reports.dat", dir_name);

    unlink(link_name);

    if (symlink(target, link_name) == -1) {
        perror("Error creating symlink");
        return 1;
    }
    return 0;
}
int write_report_data(report* new_report, const char* district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int f = open(path, O_RDWR | O_APPEND);
    if (f == -1) {
        perror("Error opening reports.dat file");
        return 1;
    }
    lseek(f, 0, SEEK_SET);
    report temp;
    // ID allocation method: find the current max id in the file, and add 1 to it
    int max_id = 0;
    while (read(f, &temp, sizeof(report)) == sizeof(report)) {
        if (temp.report_id > max_id)
            max_id = temp.report_id;
    }
    new_report->report_id = max_id + 1;

    if (write(f, new_report, sizeof(report)) != sizeof(report)) {
        perror("Error writing report");
        return 1;
    }
    else {
        printf("Succesfully saved report\n");
    }
    close(f);
    return 0;
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
// Function with file related sanity checks
int verify_system_integrity(const char* district, const char* role, const char* file_name, char action) {
    struct stat st_link, st_file;
    char link_path[256], data_path[256];
    snprintf(link_path, sizeof(link_path), "active_reports-%s", district);
    snprintf(data_path, sizeof(data_path), "%s/%s", district, file_name);
    if (strcmp(file_name, "reports.dat") == 0) {
        if (lstat(link_path, &st_link) == 0) {
            if (S_ISLNK(st_link.st_mode)) {
                if (stat(link_path, &st_file) == -1) {
                    printf("Dangling link detected for %s\n", link_path);
                    return 0;
                }
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
            printf("Manager lacks permission %c to perform this operation on file %s\n", action, file_name);
            return 0;
        }
    }
    else if (strcmp(role, "inspector") == 0) {
        mode_t mask = (action == 'w') ? S_IWGRP : S_IRGRP;
        if (!(st_file.st_mode & mask)) {
            printf("Inspector lacks permission %c to perform this operation on file %s\n", action, file_name);
            return 0;
        }
    }
    return 1;
}
int list_reports(const char* district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("Error getting file metadata");
        return 1;
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
        return 1;
    }
    report temp;
    while (read(f, &temp, sizeof(report)) == sizeof(report)) {
        // ctime was used to display an actual date, rather than a huge number
        printf("ID: %d\nInspector: %s\nCategory: %s\nCoordinates:(%.2f, %.2f)\nSeverity: %d\nTimestamp:%sDescription: %s\n",
            temp.report_id, temp.inspector_name, temp.category, temp.X, temp.Y, temp.severity, ctime(&temp.timestamp), temp.description);
        printf("-------------------------\n");
    }
    close(f);
    return 0;
}
int view_report(const char* district, int report_id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    if (report_id < 0) {
        printf("Report_id has invalid value\n");
        return 1;
    }
    int f = open(path, O_RDONLY);
    if (f == -1) {
        perror("Error opening file");
        return 1;
    }
    report temp;
    int found = 0;
    while (read(f, &temp, sizeof(report)) == sizeof(report)) {
        if (temp.report_id == report_id) {
            found = 1;
            printf("ID: %d\nInspector: %s\nCategory: %s\nCoordinates:(%.2f, %.2f)\nSeverity: %d\nTimestamp:%sDescription: %s\n",
                temp.report_id, temp.inspector_name, temp.category, temp.X, temp.Y, temp.severity, ctime(&temp.timestamp), temp.description);
            break;
        }
    }
    if (!found) {
        printf("Report with given ID not found\n");
        close(f);
        return 1;
    }
    close(f);
    return 0;
}
void write_to_log(current_context* con) {
    if (!verify_system_integrity(con->district, con->role, "logged_district", 'w')) {
        return;
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", con->district);
    int f = open(path, O_RDWR | O_APPEND);
    if (f == -1) {
        perror("Error opening logged_district file");
        exit(1);
    }
    char log_entry[256];
    int result = snprintf(log_entry, sizeof(log_entry), "%ld\t%s\t%s\t%s\n", time(NULL), con->user, con->role, con->cmd_string_conversion);
    if (write(f, log_entry, result) != result) {
        perror("Error writing report");
        exit(1);
    }
    else {
        printf("Succesfully registered operation in log journal\n");
    }
    close(f);
}
int remove_report(const char* district, int report_id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    if (report_id < 0) {
        printf("Report_id has invalid value\n");
        return 1;
    }
    int f = open(path, O_RDWR);
    if (f == -1) {
        perror("Error opening file");
        return 1;
    }
    report temp;
    int found = 0;
    off_t delete_pos = 0;
    while (read(f, &temp, sizeof(report)) == sizeof(report)) {
        if (temp.report_id == report_id) {
            found = 1;
            delete_pos = lseek(f, 0, SEEK_CUR) - sizeof(report);
            break;
        }
    }
    if (!found) {
        printf("Report with given ID not found\n");
        close(f);
        return 1;
    }
    report next;
    // Shift everything one position back, then truncate the file
    while (read(f, &next, sizeof(report)) == sizeof(report)) {
        lseek(f, delete_pos, SEEK_SET);
        write(f, &next, sizeof(report));
        delete_pos += sizeof(report);
        lseek(f, delete_pos + sizeof(report), SEEK_SET);
    }
    printf("Succesfully removed report\n");
    ftruncate(f, delete_pos);
    close(f);
    return 0;
}
int write_threshold_value(const char* district, int value) {
    char path[256];
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    int f = open(path, O_WRONLY | O_TRUNC);
    if (f == -1) {
        perror("Error opening reports.dat file");
        return 1;
    }
    char threshold_field[16];
    int len = snprintf(threshold_field, sizeof(threshold_field), "threshold=%d\n", value);
    if (write(f, threshold_field, len) != len) {
        perror("Error writing threshold");
        close(f);
        return 1;
    }
    printf("Threshold updated\n");
    close(f);
    return 0;
}
int parse_condition(const char* input, char* field, char* op, char* value) {
    if (!input || !field || !op || !value)
        return -1;

    const char* first_colon = strchr(input, ':');
    if (!first_colon)
        return -1;

    const char* second_colon = strchr(first_colon + 1, ':');
    if (!second_colon)
        return -1;

    /* Reject a trailing third colon in the value portion */
    if (strchr(second_colon + 1, ':'))
        return -1;

    /* field: from start up to the first colon */
    size_t field_len = first_colon - input;
    if (field_len == 0) return -1;
    memcpy(field, input, field_len);
    field[field_len] = '\0';

    /* op: between the two colons */
    size_t op_len = second_colon - (first_colon + 1);
    if (op_len == 0) return -1;
    memcpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    /* value: everything after the second colon */
    size_t value_len = strlen(second_colon + 1);
    if (value_len == 0) return -1;
    memcpy(value, second_colon + 1, value_len);
    value[value_len] = '\0';

    return 0;
}
int match_condition(report* r, const char* field, const char* op, const char* value) {
    if (!r || !field || !op || !value) return 0;

    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<") == 0) return r->severity < val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0) return r->severity > val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    }
    else if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        if (strcmp(op, "<") == 0) return cmp < 0;
        if (strcmp(op, "<=") == 0) return cmp <= 0;
        if (strcmp(op, ">") == 0) return cmp > 0;
        if (strcmp(op, ">=") == 0) return cmp >= 0;
    }
    else if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector_name, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
        if (strcmp(op, "<") == 0) return cmp < 0;
        if (strcmp(op, "<=") == 0) return cmp <= 0;
        if (strcmp(op, ">") == 0) return cmp > 0;
        if (strcmp(op, ">=") == 0) return cmp >= 0;
    }
    else if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == val;
        if (strcmp(op, "!=") == 0) return r->timestamp != val;
        if (strcmp(op, "<") == 0) return r->timestamp < val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= val;
        if (strcmp(op, ">") == 0) return r->timestamp > val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= val;
    }

    return 0;
}
int filter_reports(const char* district, char conditions[][100], int count) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int f = open(path, O_RDONLY);
    if (f == -1) {
        perror("Error opening file");
        return 1;
    }
    report temp;
    while (read(f, &temp, sizeof(report)) == sizeof(report)) {
        char field[50], op[10], value[50];
        int match = 1;
        for (int i = 0; i < count; i++) {
            if (parse_condition(conditions[i], field, op, value) == 0) {
                if (match_condition(&temp, field, op, value) == 0) {
                    match = 0;
                    break;
                }
            }
        }
        if (match) {
            printf("ID: %d\nInspector: %s\nCategory: %s\nCoordinates:(%.2f, %.2f)\nSeverity: %d\nTimestamp:%sDescription: %s\n-------------------------\n",
                temp.report_id, temp.inspector_name, temp.category, temp.X, temp.Y, temp.severity, ctime(&temp.timestamp), temp.description);
        }
    }
    close(f);
    return 0;
}
int main(int argc, char* argv[]) {
    current_context con = { 0 };
    parse_arguments(argc, argv, &con);
    // If the execution was typed incorrectly
    if (con.cmd_type == CMD_NONE || strlen(con.user) == 0 || strlen(con.role) == 0) {
        printf("Incorrect usage of commands\n");
        return 1;
    }
    switch (con.cmd_type) {
    case CMD_ADD: {
        if (create_file_structure(con.district) == 1)
            printf("Failed to initialize files. Operation cancelled\n");
        else if (verify_system_integrity(con.district, con.role, "reports.dat", 'w')) {
            strcpy(con.cmd_string_conversion, "add");
            report new_report;
            memset(&new_report, 0, sizeof(report));
            read_report_data(&new_report, con.user);
            // We write to log only if the operation was successful
            if (write_report_data(&new_report, con.district) == 0)
                write_to_log(&con);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    case CMD_LIST: {
        if (verify_system_integrity(con.district, con.role, "reports.dat", 'r')) {
            strcpy(con.cmd_string_conversion, "list");
            if (list_reports(con.district) == 0)
                write_to_log(&con);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    case CMD_VIEW: {
        if (verify_system_integrity(con.district, con.role, "reports.dat", 'r')) {
            strcpy(con.cmd_string_conversion, "view");
            if (view_report(con.district, con.report_id) == 0)
                write_to_log(&con);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    case CMD_REMOVE: {
        if (strcmp(con.role, "manager") != 0) {
            printf("Error: remove_report is restricted to manager only \n");
            break;
        }
        if (verify_system_integrity(con.district, con.role, "reports.dat", 'w')) {
            strcpy(con.cmd_string_conversion, "remove_report");
            if (remove_report(con.district, con.report_id) == 0)
                write_to_log(&con);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    case CMD_UPDATE: {
        if (con.threshold < 1 || con.threshold > 3) {
            printf("Invalid threshold value, valid ones are 1, 2 or 3\n");
            break;
        }
        if (strcmp(con.role, "manager") != 0) {
            printf("Error: update_threshold is restricted to manager only \n");
            break;
        }
        struct stat st;
        char cfg_path[256];
        snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", con.district);
        if (stat(cfg_path, &st) == -1) {
            perror("cannot stat dsitrict.cfg");
            break;
        }
        if ((st.st_mode & 0777) != 0640) {
            printf("dsitrict.cfg permissions changed, expected 0640, cancelling operation\n");
            break;
        }
        if (verify_system_integrity(con.district, con.role, "district.cfg", 'w')) {
            strcpy(con.cmd_string_conversion, "update_threshold");
            if (write_threshold_value(con.district, con.threshold) == 0)
                write_to_log(&con);
        }
        else {
            printf("Operation cancelled\n");
        }
        break;
    }
    case CMD_FILTER: {
        if (verify_system_integrity(con.district, con.role, "reports.dat", 'r')) {
            strcpy(con.cmd_string_conversion, "filter");
            if (filter_reports(con.district, con.conditions, con.condition_count) == 0)
                write_to_log(&con);
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
