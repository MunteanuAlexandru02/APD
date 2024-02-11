#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100
#define MAX_CLIENTS 10
#define ACK_MESSAGE "ok"
#define REQUIRE_FILE 0
#define REFRESH_FILE_INFO 1
#define FINISHED_ONE_FILE 2
#define FINISHED_ALL_FILES 3
#define FINISHED_EVERYTHING 4
#define REQUIRE_HASH 5
#define UPDATE_HASHES_DB 6
#define IS_FINISHED 69
#define DEBUG_MESSAGE "VACA\n"
#define S "\nSUSANU\n"

struct client_files {
    int files_count;                              //number of files already stored by client
    char files_info[MAX_FILES][MAX_CHUNKS];       //every file and the stored hashed

    int req_files_count;                          //number of required files
    char* req_files[MAX_FILES];                   //the name of the required files
};

struct file_swarm {                               //file info, like its name and the clients
    char name[MAX_FILENAME];
    char hash[MAX_CHUNKS][HASH_SIZE + 2];
    //int chunk_size;
    int clients_no;
    int clients[MAX_CLIENTS];                     //the rank of the clients
    int chunk_size[MAX_CLIENTS];
};

struct file_info {                                //info used by the client
    char name[MAX_FILENAME];
    int chunk_no;
    int current_chunk;
    char hash[MAX_CHUNKS][HASH_SIZE + 2];
};

struct tracker_info {                             //info held by the tracker
    struct file_swarm f[MAX_FILES];
    int size;

    int finished_clients[MAX_CLIENTS + 1];            // the clients that finished downloading every file
};

struct needed_files {
    int rank;
    int req_files_count;
    char files[MAX_FILES][MAX_FILENAME];
    struct file_info *p_owned;
    int owned_files_count;
};

struct to_send_files {
    int file_count;
    int rank;
    struct file_info owned[MAX_FILENAME];
};