#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#define maxLength 30

typedef struct report {
     int report_id;
     char inspector_name[maxLength];
     double X, Y;
     char category[maxLength];
     int severity;
     time_t timestamp;
     char description[maxLength];
}report;

void read_report_data(){
    report new_report;
    printf("Insert X Coordinate: ");
    if(scanf("%lf", &new_report.X) != 1){
        printf("Date Introduse Invalid!\n");
        return;
    }
    printf("Insert Y Coordinate: ");
    if(scanf("%lf", &new_report.Y) != 1){
        printf("Date Introduse Invalid!\n");
        return;
    }
    printf("Insert category (road/lighting/flooding/other): ");
    if(scanf("%30s", new_report.category) != 1){
        printf("Date introduse invalid!\n");
        return;
    }
    printf("Insert Severity Level (1/2/3): ");
    if(scanf("%d", &new_report.severity) != 1){
        printf("Date Introduse Invalid!\n");
        return;
    }
    printf("Insert description: ");
    if(scanf("%30s", new_report.description) != 1){
        printf("Date Introduse Invalid!\n");
        return;
    }
}

int create_files(char *dir_name){
    int dir_created = mkdir(dir_name, 0750);
    if(!dir_created){
        int file1, file2, file3;
        char dir_path[10] = "";
        strcat(dir_path, dir_name);
        strcat(dir_path, "/");
        char file1_path[50] = "", file2_path[50] = "", file3_path[50] = "";
        strcpy(file1_path, dir_path), strcpy(file2_path, dir_path), strcpy(file3_path, dir_path);
        strcat(file1_path, "reports.dat"),
        strcat(file2_path, "district.cfg");
        strcat(file3_path, "logged_district");
        file1 = open(file1_path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
        file2 = open(file2_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        file3 = open(file3_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]){
    int found_user = 0, found_role = 0;
    for(int i = 0; i < argc; i++){
      char role[maxLength] = "", user[maxLength] = "";
      if(strcmp(argv[i], "--role") == 0)
          strcpy(argv[i + 1], role), found_role = 1;
      else if(strcmp(argv[i], "--user") == 0)
          strcpy(argv[i + 1], user), found_user = 1;
      else if(strcmp(argv[i], "--add") == 0){
          if(create_files(argv[i + 1]) == 1)
              read_report_data();
          else
              printf("Eroare La Crearea Directorului \n");
      }
    }
    if(found_role == 0 || found_user == 0){
      printf("Eroare la structura comenzii\n");
      exit(1);
    }
    return 0;
}

