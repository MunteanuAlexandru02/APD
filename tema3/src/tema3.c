#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"

//send_file_finish_info(nf.rank, nf.files[i], total_hashes_received);
void send_file_finish_info(int rank, char *filename, int hashes_count) {
    int message_type = FINISHED_ONE_FILE;
    
    //Send message to tracker
    MPI_Send(&rank, 1, MPI_INT, 0, 30, MPI_COMM_WORLD);
    MPI_Send(&message_type, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    MPI_Send(filename, strlen(filename) + 1, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
    MPI_Send(&hashes_count, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);

    // Send message for upload_func
    MPI_Send(&rank, 1, MPI_INT, rank, 5, MPI_COMM_WORLD);
    MPI_Send(&message_type, 1, MPI_INT, rank, 6, MPI_COMM_WORLD);
    MPI_Send(filename, strlen(filename) + 1, MPI_CHAR, rank, 7, MPI_COMM_WORLD);
}

void send_all_files_finish(int rank) {
    int message_type = FINISHED_ALL_FILES;

    MPI_Send(&rank, 1, MPI_INT, 0, 30, MPI_COMM_WORLD);
    MPI_Send(&message_type, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
}

void *download_thread_func(void *arg) {
    struct needed_files nf = *(struct needed_files*) arg;
    int count = nf.req_files_count;
    int tracker_message_type = REQUIRE_FILE;
    int clients[MAX_CLIENTS + 1];
    int chunk_sizes[MAX_CLIENTS + 1];
    char requested_hash[HASH_SIZE + 1];
    MPI_Status status;
    for (int i = 0; i < count; i++) {
        int total_hashes_received = 0;
        int hashes_received = 0;

        while(1) {
            //Ask every 10 hashes
            if (hashes_received % 10 == 0) {
                // Send info to the tracker, so that the client will be added to
                // the file swarm
                if (hashes_received % 5 == 0 && hashes_received != 0) {
                    MPI_Send(&nf.rank, 1, MPI_INT, 0, 30, MPI_COMM_WORLD);
                    int update_tracker = REFRESH_FILE_INFO;
                    MPI_Send(&update_tracker, 1, MPI_INT, 0 , 1, MPI_COMM_WORLD);
                    MPI_Send(nf.files[i], MAX_FILENAME + 1, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
                    MPI_Send(&total_hashes_received, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
                }

                MPI_Send(&nf.rank, 1, MPI_INT, 0, 30, MPI_COMM_WORLD);
                MPI_Send(&tracker_message_type, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
                MPI_Send(nf.files[i], strlen(nf.files[i]) + 1, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
                // Receive the number of clients that hold
                int clients_count;
                MPI_Recv(&clients_count, 1, MPI_INT, 0, 20, MPI_COMM_WORLD, &status);
                
                int message_type = REQUIRE_HASH;
                for (int j = 0; j < clients_count; j++) {
                    MPI_Recv(&clients[j], 1, MPI_INT, 0, 21, MPI_COMM_WORLD, &status);
                    MPI_Recv(&chunk_sizes[j], 1, MPI_INT, 0, 22, MPI_COMM_WORLD, &status);
                }

                while (1) {
                    if (total_hashes_received == chunk_sizes[0]) {
                        MPI_Send(&nf.rank, 1, MPI_INT, nf.rank, 5, MPI_COMM_WORLD);
                        message_type = UPDATE_HASHES_DB;
                        MPI_Send(&message_type, 1, MPI_INT, nf.rank, 6, MPI_COMM_WORLD);
                        MPI_Send(requested_hash, HASH_SIZE + 1, MPI_CHAR, nf.rank, 7, MPI_COMM_WORLD);
                        MPI_Send(&total_hashes_received, 1, MPI_INT, nf.rank, 8, MPI_COMM_WORLD);
                        MPI_Send(nf.files[i], MAX_FILENAME + 1, MPI_CHAR, nf.rank, 9, MPI_COMM_WORLD);
                        break;
                    }
                    
                    // In order to alternate as much as possible, without
                    // doing something smart like considering the connections
                    // of a peer, I'll just select randomly a client from the list
                    int random_client = rand() % clients_count;
                    // the other client needs to have more hashes in order to
                    // download from it

                    if (chunk_sizes[random_client] > total_hashes_received) {
                        //send a request for the chunk
                        int send_rank = clients[random_client];
                        MPI_Send(&nf.rank, 1, MPI_INT, send_rank, 5, MPI_COMM_WORLD);
                        MPI_Send(&message_type, 1, MPI_INT, send_rank, 6, MPI_COMM_WORLD);
                        MPI_Send(&total_hashes_received, 1, MPI_INT, send_rank, 7, MPI_COMM_WORLD);
                        MPI_Send(nf.files[i], strlen(nf.files[i]) + 1, MPI_CHAR, send_rank, 8, MPI_COMM_WORLD);

                        // need to add the hash somewhere
                        
                        MPI_Recv(requested_hash, HASH_SIZE + 1, MPI_CHAR, send_rank, 9, MPI_COMM_WORLD, &status);

                        // I have decided to send a message to the same client that
                        // received the hash because I intend to use the same hash
                        // in the upload function
                        MPI_Send(&nf.rank, 1, MPI_INT, nf.rank, 5, MPI_COMM_WORLD);
                        message_type = UPDATE_HASHES_DB;
                        MPI_Send(&message_type, 1, MPI_INT, nf.rank, 6, MPI_COMM_WORLD);
                        MPI_Send(requested_hash, HASH_SIZE + 1, MPI_CHAR, nf.rank, 7, MPI_COMM_WORLD);
                        MPI_Send(&total_hashes_received, 1, MPI_INT, nf.rank, 8, MPI_COMM_WORLD);
                        MPI_Send(nf.files[i], MAX_FILENAME + 1, MPI_CHAR, nf.rank, 9, MPI_COMM_WORLD);
                        
                        message_type = REQUIRE_HASH;
                        total_hashes_received++;
                        hashes_received++;
                        
                        if (hashes_received != 0 && hashes_received % 10 == 0) {
                            break;
                        }
                    }
                }
            }
            if (total_hashes_received == chunk_sizes[0]) {
                // inca nu inteleg de ce trebuie sa fac asta aici
                if (total_hashes_received % 10 == 0) {
                    MPI_Send(&nf.rank, 1, MPI_INT, nf.rank, 5, MPI_COMM_WORLD);
                    int message_type = UPDATE_HASHES_DB;
                    MPI_Send(&message_type, 1, MPI_INT, nf.rank, 6, MPI_COMM_WORLD);
                    MPI_Send(requested_hash, HASH_SIZE + 1, MPI_CHAR, nf.rank, 7, MPI_COMM_WORLD);
                    MPI_Send(&total_hashes_received, 1, MPI_INT, nf.rank, 8, MPI_COMM_WORLD);
                    MPI_Send(nf.files[i], MAX_FILENAME + 1, MPI_CHAR, nf.rank, 9, MPI_COMM_WORLD);
                }
                break;
            }
        }
        // Received the entire file, need to signal the start of writing info
        // into a file -- send the info to the tracker and also upload_thread_func
        send_file_finish_info(nf.rank, nf.files[i], total_hashes_received);
    }
    send_all_files_finish(nf.rank);
    return NULL;
}

void *upload_thread_func(void *arg) {
    //int rank = *(int*) arg;
    struct to_send_files sf = *(struct to_send_files *) arg;
    int recv_rank, message_type, needed_hash;
    char filename[MAX_FILENAME + 1];
    MPI_Status status;

    while(1) {
        
        MPI_Recv(&recv_rank, 1, MPI_INT, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
        MPI_Recv(&message_type, 1, MPI_INT, recv_rank, 6, MPI_COMM_WORLD, &status);
        if (message_type == REQUIRE_HASH) {
            MPI_Recv(&needed_hash, 1, MPI_INT, recv_rank, 7, MPI_COMM_WORLD, &status);
            MPI_Recv(filename, MAX_FILENAME + 1, MPI_CHAR, recv_rank, 8, MPI_COMM_WORLD, &status);            
            
            // need to find the filename
            for (int i = 0; i < sf.file_count; i++) {
                // found the filename, need to send the needed_hash
                if (strcmp(filename, sf.owned[i].name) == 0) {
                    int length = strlen(sf.owned[i].hash[needed_hash]) + 1;
                    MPI_Send(sf.owned[i].hash[needed_hash], length, MPI_CHAR, recv_rank, 9, MPI_COMM_WORLD);
                    break;
                }
            }
        } else if (message_type == UPDATE_HASHES_DB) {
            char hash[HASH_SIZE + 1];
            int hash_position;
            MPI_Recv(hash, HASH_SIZE + 1, MPI_CHAR, recv_rank, 7, MPI_COMM_WORLD, &status);
            MPI_Recv(&hash_position, 1, MPI_INT, recv_rank, 8, MPI_COMM_WORLD, &status);
            MPI_Recv(filename, MAX_FILENAME + 1, MPI_CHAR, recv_rank, 9, MPI_COMM_WORLD, &status);

            int ok = 1;
            for (int i = 0; i < sf.file_count; i++) {
                // already know the file, just need to update the hashes
                if (strcmp(filename, sf.owned[i].name) == 0) {
                    sf.owned[i].current_chunk++;
                    sf.owned[i].chunk_no++;
                    strcpy(sf.owned[i].hash[hash_position], hash);
                    ok = 0;
                    break;
                }
            }
            
            // need to add the file to the list
            if (ok == 1) {
                int count = sf.file_count;
                sf.owned[count].current_chunk = 0;
                sf.owned[count].chunk_no = 0;
                strcpy(sf.owned[count].name, filename);
                strcpy(sf.owned[count].hash[0], hash);
                sf.file_count ++;
            }

        } else if (message_type == FINISHED_ONE_FILE) {
            // just need to write the hashes inside a file
            MPI_Recv(filename, MAX_FILENAME + 1, MPI_CHAR, recv_rank, 7, MPI_COMM_WORLD, &status);
            char name_of_file[2*MAX_FILENAME];
            sprintf(name_of_file, "client%d_%s", recv_rank, filename);
        
            FILE *file = fopen(name_of_file, "w");

            for (int i = 0; i < sf.file_count; i++) {
                if (strcmp(sf.owned[i].name, filename) == 0) {
                    for(int j = 0; j < sf.owned[i].chunk_no; j++) {
                        fprintf(file, "%s\n", sf.owned[i].hash[j]);
                    }
                    break;
                }
            }
            fclose(file);
        } else if (message_type == FINISHED_EVERYTHING) {
            break;
        }
    }
    return NULL;
}

void tracker(int numtasks, int rank) {
    int file_count, chunk_no;
    char received[MAX_FILENAME + 1];
    char hash_recv[HASH_SIZE];
    MPI_Status status;

    struct tracker_info info;
    info.size = 0;

    for (int i = 1; i < numtasks; i++) {
        // Receive the number of files held by a client
        MPI_Recv(&file_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
        // Receive the file names, number of hashes and the hashes themselves
        for (int j = 0; j < file_count; j++) {
            MPI_Recv(received, MAX_FILENAME + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&chunk_no, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            int ok = 1;
            //info.f[k].clients_no = 0;
            for (int k = 0; k < info.size; k++)
                // the file is already stored - by another client
                if (strcmp(info.f[k].name, received) == 0) {
                    info.f[k].chunk_size[info.f[k].clients_no] = chunk_no;
                    info.f[k].clients[info.f[k].clients_no++] = i;
                    ok = 0;
                }
            //the file is not in the file swarm -- need to add it
            if (ok == 1) {
                strcpy(info.f[info.size].name, received);
                info.f[info.size].chunk_size[0] = chunk_no;
                info.f[info.size].clients_no = 1;
                info.f[info.size].clients[0] = i;

                //Receive the chunks -- only do it once, when the file is loaded
                //in swarm
                for (int k = 0; k < chunk_no; k++) {
                    MPI_Recv(hash_recv, HASH_SIZE + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
                    strcpy(info.f[info.size].hash[k], hash_recv);
                }
                // pana aici merge
                info.size++;
            } else if (ok == 0) {
                for (int k = 0; k < chunk_no; k++)
                    MPI_Recv(hash_recv, HASH_SIZE + 1, MPI_CHAR, i ,0, MPI_COMM_WORLD, &status);
            }
        }
    }

    /// aici nu mai merge 

    // Received all the info needed from the clients
    for (int i = 1; i < numtasks; i++)
        MPI_Send(ACK_MESSAGE, strlen(ACK_MESSAGE) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);

    int recv_rank;
    int message_type;
    char file[MAX_FILENAME + 1];

    while(1) {
        MPI_Recv(&recv_rank, 1, MPI_INT, MPI_ANY_SOURCE, 30, MPI_COMM_WORLD, &status);
        MPI_Recv(&message_type, 1, MPI_INT, recv_rank, 1, MPI_COMM_WORLD, &status);
        
        if (message_type == REQUIRE_FILE) {
            int cnt = -1;
            
            MPI_Recv(file, MAX_FILENAME + 1, MPI_CHAR, recv_rank, 2, MPI_COMM_WORLD, &status);
            
            //search for the file in the file swarm
            for (int i = 0; i < info.size; i++) {
                // if I find the file, I will save the i and
                // send the client vector 
                if (strcmp(file, info.f[i].name) == 0) {
                    cnt = i;
                    break;
                }
            }

            if (cnt != -1) {
                MPI_Send(&info.f[cnt].clients_no, 1, MPI_INT, recv_rank, 20, MPI_COMM_WORLD);
                for (int i = 0; i < info.f[cnt].clients_no; i++) {
                    MPI_Send(&info.f[cnt].clients[i], 1, MPI_INT, recv_rank, 21, MPI_COMM_WORLD);    
                    MPI_Send(&info.f[cnt].chunk_size[i], 1, MPI_INT, recv_rank, 22, MPI_COMM_WORLD);
                }
            }
        /* Received an update message, will tell me:
            -   if a client started downloading a new file, the name of the file
            in order to add it to the file swarm
            -   if the client is already part of the file swarm, update the number
            of chunks
        */
        } else if (message_type == REFRESH_FILE_INFO) {
            int current_chunks;
            MPI_Recv(&file, MAX_FILENAME + 1, MPI_CHAR, recv_rank, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&current_chunks, 1, MPI_INT, recv_rank, 3, MPI_COMM_WORLD, &status);

            int ok = 1;
            // search for the filename in the file swarm
            for (int i = 0; i < info.size; i++) {
                // the file is in the file swarm
                if (strcmp(info.f[i].name, file) == 0) {
                    for (int j = 0; j < info.f[i].clients_no; j++)
                        // update the number of chuncks of a file
                        if (info.f[i].clients[j] == recv_rank) {
                            info.f[i].chunk_size[j] = current_chunks;
                            ok = 0;
                            break;
                        }
                    // the client was no found in the file swarm
                    if (ok == 1) {
                        info.f[i].clients[info.f[i].clients_no] = recv_rank;
                        info.f[i].chunk_size[info.f[i].clients_no] = current_chunks;
                        info.f[i].clients_no++;
                    }
                    break;
                }
            }
        } else if (message_type == FINISHED_ONE_FILE) {
            // the file that finished downloading
            int final_chunks;
            MPI_Recv(&file, MAX_FILENAME + 1, MPI_CHAR, recv_rank, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&final_chunks, 1, MPI_INT, recv_rank, 3, MPI_COMM_WORLD, &status);

            for (int i = 0; i < info.size; i++) {
                if (strcmp(info.f[i].name, file) == 0) {
                    for (int j = 0; j < info.f[i].clients_no; j++) {
                        if (info.f[i].clients[j] == recv_rank) {
                            info.f[i].chunk_size[j] = final_chunks;
                            break;
                        }
                        break;
                    }
                }
            }
        } else if (message_type == FINISHED_ALL_FILES) {
            // Don't need any additional info, just mark the client as finished
            info.finished_clients[recv_rank] = IS_FINISHED;
            MPI_Send(&rank, 1, MPI_INT, recv_rank, 15, MPI_COMM_WORLD);
            MPI_Send(&message_type, 1, MPI_INT, recv_rank, 16, MPI_COMM_WORLD);
        
            int ok = 1;
            for (int i = 1; i < numtasks; i++) {
                if (info.finished_clients[i] != IS_FINISHED) {
                    ok = 0;
                    break;
                }
            }
            
            // all clients are finished, need to send a finished_everything
            // message
            if (ok == 1) {
                message_type = FINISHED_EVERYTHING;
                for (int i = 1; i < numtasks; i++) {
                     MPI_Send(&rank, 1, MPI_INT, i, 5, MPI_COMM_WORLD);
                    MPI_Send(&message_type, 1, MPI_INT, i, 6, MPI_COMM_WORLD);

                    MPI_Send(&rank, 1, MPI_INT, i, 15, MPI_COMM_WORLD);
                    MPI_Send(&message_type, 1, MPI_INT, i,16, MPI_COMM_WORLD);
                }
                return;
            }
        }   
    } // end while
}

void parse_file(FILE *file, struct file_info *owned, int finished,
                        int *req_files_count, char (*req_files)[MAX_FILENAME]) {

    char buf[HASH_SIZE + 2];
    char name[MAX_FILENAME];
    char extra, before;
    int number, current_file = -1, curr_file_req = 0;

    while (fgets(buf, 34, file) != NULL) {
        // Check for the number of chuncks in a file and the name of the file
        if (finished == 0) { // Read hashes and file names and chunck sizes
            buf[HASH_SIZE] = '\0';
            char buf_copy[HASH_SIZE + 2];
            strcpy(buf_copy, buf);
            if (sscanf(buf, "%s%c%d%c", name, &before, &number, &extra) == 4) {
                if (before == ' ' && extra == '\n') {
                    name[strlen(name)] = '\0';
                    strcpy(owned[++current_file].name, name);
                    owned[current_file].chunk_no = number;
                    owned[current_file].current_chunk = 0;
                } 
            } else if (sscanf(buf, "%d%c", &number, &extra) == 2 && strlen(buf) <= 2) {
                // Check for the number of files owned
                if (extra == '\n') {
                    finished = 1;
                    *req_files_count = number;
                }
            } else {
                int current = owned[current_file].current_chunk;
                strcpy(owned[current_file].hash[current], buf_copy);
                owned[current_file].current_chunk++;
            }
        } else if (finished == 1){
            // check for the number of required files
            if (sscanf(buf, "%d%c", &number, &extra) == 2 && strlen(buf) <= 2) {
                if (extra == '\n') {
                    *req_files_count = number;
                }
            } else {
                buf[strcspn(buf, "\n")] = '\0';
                strcpy(req_files[curr_file_req], buf);
                curr_file_req++;
            }
        }
    }
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r, number;
    char buf[HASH_SIZE + 2], extra;
    char req_files[MAX_FILES][MAX_FILENAME];
    int finished = 0; // finished reading the file owned

    // Read from the in{rank}.txt
    char filename[100];
    sprintf(filename, "in%d.txt", rank);
    int file_count; // number of files already owned
    int required_files_count; // number of files that will be downloaded
    MPI_Status mpi_status;

    FILE *file = fopen(filename, "r");

    // Read the number of files owned
    fgets(buf, sizeof(buf), file);
    if (sscanf(buf, "%d%c", &number, &extra) == 2) {
        if (extra == '\n') {
            file_count = number;
        }
    }
    struct file_info owned[MAX_FILES + 1];

    if (file_count == 0)
        finished = 1; // Dont need to read the hashed - there are none
    parse_file(file, owned, finished, &required_files_count, req_files);

    MPI_Send(&file_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    for (int i = 0; i < file_count; i++) {
        MPI_Send(owned[i].name, strlen(owned[i].name) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&owned[i].chunk_no, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

        for (int j = 0; j < owned[i].chunk_no; j++)
            MPI_Send(owned[i].hash[j], strlen(owned[i].hash[j]) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    char ack_message[4];
    // Receive the ack_message -- can send the download files
    
    MPI_Recv(ack_message, 4, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &mpi_status);
    
    // struct that will be sent to the download thread
    struct needed_files n_files;
    n_files.rank = rank;
    n_files.req_files_count = required_files_count;
    n_files.p_owned = owned;
    n_files.owned_files_count = file_count;
    for (int i = 0; i < required_files_count; i++) {
        strcpy(n_files.files[i], req_files[i]);
    }

    struct to_send_files sf;
    sf.file_count = file_count;
    memcpy(sf.owned, owned, file_count * sizeof(struct file_info));

    //r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &rank);
    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &n_files);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &sf);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    int recv_rank, message_type;
    while(1) {
        
        MPI_Recv(&recv_rank, 1, MPI_INT, MPI_ANY_SOURCE, 15, MPI_COMM_WORLD, &mpi_status);
        MPI_Recv(&message_type, 1, MPI_INT, recv_rank, 16, MPI_COMM_WORLD, &mpi_status);

        if (message_type == FINISHED_ALL_FILES) {
            r = pthread_join(download_thread, &status);
            if (r) {
                printf("Eroare la asteptarea thread-ului de download\n");
                exit(-1);
            }
        } else if (message_type == FINISHED_EVERYTHING) {
            r = pthread_join(upload_thread, &status);
            if (r) {
                printf("Eroare la asteptarea thread-ului de upload\n");
                exit(-1);
            }
            break;
        }
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
