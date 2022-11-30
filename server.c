#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "net.h"
#include "array_list.c"
#define ARGS_CNT 2
#define ARR_SIZE 1024

void list_free(ArrayList *array_list){
   int loop = 0;
   while(loop < array_list -> length){
      free(array_list -> elements[loop]);
      loop++;
   }
   free(array_list -> elements);
   free(array_list);
}

void response_write(char *message, int nfd){
   int val;
   int length = strlen(message);
   val = write(nfd, message, length);
   if(val < 0){
      perror("write failed");
      exit(1);
   }
}

void response_400(int nfd){
   char *message = NULL;
   message = "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 46\r\n\r\nRequest is incorrect and cannot be processed\n";
   response_write(message, nfd);
   exit(1);
}

void response_403(int nfd){
   char *message = NULL;
   message = "HTTP/1.0 403 Permission Denied\r\nContent-Type: text/html\r\nContent-Length: 18\r\n\r\nPermission denied\n";
   response_write(message, nfd);
   exit(1);
}

void response_404(int nfd){
   char *message = NULL;
   message = "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\nRequest cannot be found\n";
   response_write(message, nfd);
   exit(1);
}

void response_500(int nfd){
   char *message = NULL;
   message = "HTTP/1.0 500 Internal Error\r\nContent-Type: text/html\r\nContent-Length: 29\r\n\r\nAn unexpected error occured\n";
   response_write(message, nfd);
   exit(1);
}

void response_501(int nfd){
   char *message = NULL;
   message = "HTTP/1.0 501 Not Implemented\r\nContent-Type: text/html\r\nContent-Length: 31\r\n\r\nFunctionality is not supported\n";
   response_write(message, nfd);
   exit(1);
}

pid_t spawn(int nfd){
   pid_t pid;
   if ((pid = fork()) < 0){
      response_500(nfd);
   }
   return pid;
}

char *head_code(char *result, int nfd){
   char buffer[ARR_SIZE];
   int number;
   struct stat buf_stat;
   int size;
   lstat(result, &buf_stat);
   size = buf_stat.st_size;
   snprintf(buffer, ARR_SIZE, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", size);
   number = write(nfd, buffer, strlen(buffer));
   if(number < 1){
      response_500(nfd);
   }
   return result;
}

char *head_request(char *contents, int nfd, char *request){
   char *period = ".";
   char *result = NULL;
   result = malloc(1 + strlen(period) + strlen(contents));
   strcpy(result, period);
   strcat(result, contents);
   result = head_code(result, nfd);
   return result;
}

void get_request(int nfd, char *result){
   int count;
   int fd;
   int number;
   char buffer[ARR_SIZE];
   if(strcmp(result, "") == 0){
      response_400(nfd);
   }
   if((fd = open(result, O_RDONLY)) == -1){
      response_404(nfd);
   }
   while((count = read(fd, buffer, ARR_SIZE))){
      number = write(nfd, buffer, count);
      if(number < 0){
         response_500(nfd);
      }
   }
   close(fd);
}

void execute_request(pid_t file_pid, char *prog_name, char **args_array, int nfd){
   char pid_num[ARR_SIZE];
   pid_t pid;
   int fd;
   int status;
   snprintf(pid_num, ARR_SIZE, "file_pid:%d.txt", file_pid);
   if((pid = spawn(nfd)) == 0){
      if((fd = open(pid_num, O_WRONLY | O_CREAT, S_IRUSR | S_IXUSR | S_IWUSR)) < 0){
         response_403(nfd);
      }
      dup2(fd, 1);
      close(fd);
      if(strcmp(prog_name, "") == 0){
         response_400(nfd);
      }
      execv(prog_name, args_array);
      response_404(nfd);
   }
   else{
      wait(&status);
      head_code(pid_num, nfd);
      get_request(nfd, pid_num);
      remove(pid_num);
   }
}

void cgi_no_args(char *contents, char *request, int nfd){
   char *args_array[2];
   char *prog_name;
   pid_t file_pid = getpid();
   prog_name = strsep(&contents, "/");
   if(prog_name == NULL){
      response_400(nfd);
   }
   args_array[0] = prog_name;
   args_array[1] = '\0';
   execute_request(file_pid, prog_name, args_array, nfd);
}

void cgi_args(char *contents, char *request, int nfd){
   char *arguments = NULL;
   char *separate = NULL;
   pid_t file_pid = getpid();
   ArrayList *array_list = array_list_new();
   strsep(&contents, "/");
   strsep(&contents, "/");
   separate = strsep(&contents, "?");
   array_list_add_to_end(array_list, separate);
   while((arguments = strsep(&contents, "&"))){
      array_list_add_to_end(array_list, arguments);
   }
   execute_request(file_pid, separate, array_list -> elements, nfd);
}

void cgi_support(char *contents, char *request, int nfd){
   char *duplicate;
   char *separate;
   int value;
   duplicate = strdup(contents);
   strsep(&duplicate, "/");
   separate = strsep(&duplicate, "/");
   value = chdir(separate);
   if(value != 0){
      response_404(nfd);
   }
   if((strchr(contents, '?')) != NULL){
      cgi_args(contents, request, nfd);
   }
   else{
      cgi_no_args(duplicate, request, nfd);
   }
}

void manage_args(char *copy, int nfd){
   char *req = NULL;
   char *slash_check = NULL;
   char *dup;
   char *result;
   int length;
   char *request = strsep(&copy, " ");
   char *contents = strsep(&copy, " ");
   dup = strdup(contents);
   
   if(strcmp(request, "HEAD") == 0){
      result = head_request(dup, nfd, request);
   }
   length = strlen(contents);
   if(length > 8){
      req = strndup(contents, 9);
      if(strcmp(req, "/cgi-like") == 0){
         slash_check = strndup(contents, 10);
         if(strcmp(slash_check, "/cgi-like/") == 0){
            cgi_support(contents, request, nfd);
         }
         else{
            response_501(nfd);
         }
      }
      else if(strcmp(request, "GET") == 0){
         result = head_request(dup, nfd, request);
         get_request(nfd, result);
      }
      else{
         response_400(nfd);
      }
   }
   free(req);
   free(slash_check);
}

void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r");
   char *line = NULL;
   char *copy = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL){
      perror("fdopen");
      close(nfd);
      return;
   }
   while((num = getline(&line, &size, network)) >= 0){
      copy = line;
      manage_args(copy, nfd);
   }

   free(line);
   fclose(network);
}

void wait_signal(){
   int status;
   int pid;
   while((pid = waitpid(0, &status, WNOHANG)) > 0) {
      if(WIFEXITED(status) != 0 && WEXITSTATUS(status) != 0){
         return;
      }
   } 
}

void signal_handle(){
   struct sigaction action;
   if(sigemptyset(&action.sa_mask) == -1){
      perror("Sigempty Error");
      exit(1);
   }
   action.sa_flags = 0;
   action.sa_handler = wait_signal;
   if(sigaction(SIGCHLD, &action, NULL) == -1){
      perror("Sigaction Error");
      exit(1);
   }
}

void run_service(int fd)
{
   while (1){
      int nfd = accept_connection(fd);
      int pid;
      if (nfd != -1){
         printf("Connection established\n");
         if((pid = spawn(nfd)) == 0){
            handle_request(nfd);
            exit(0);
         }
         else{
            signal_handle();
         }
         printf("Connection closed\n");
      }
   }
}

void check_port(char* args_port){
   if(atoi(args_port) < 1024 || atoi(args_port) > 65535){
      perror("port needs to be between 1024 and 65535");
      exit(1);
   }
}

void validate_arguments(int argc, char *argv[]){
    if (argc != ARGS_CNT){
		fprintf(stderr, "usage: %s file\n", argv[0]); 
		exit(1); 
	}
}

int main(int argc, char *argv[])
{
   validate_arguments(argc, argv);
   check_port(argv[1]);
   int port = atoi(argv[1]);
   int fd = create_service(port);

   if (fd != -1){
      printf("listening on port: %d\n", port);

      run_service(fd);

      close(fd);
   }
   return 0;
}

