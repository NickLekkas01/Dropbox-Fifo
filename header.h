#define SizeofStrings 100

void my_write(int bytes, int fd, char *string);
void my_read(int bytes, int fd, char *string);
int explain_wait_status(int status, int pid);
void write_to_fifo(int fd, char *path, int b, int fd3);
void read_from_fifo(int fd, char *path, int b, int fd3);
void delete_mirror(char *path);
void signal_arrived(int signal);
void handler (void);
void uninstall_handler (void);