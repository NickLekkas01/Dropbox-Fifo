# Dropbox-Fifo

Compile:
make

Run:
    ./exec -n 1 -c ./common -i ./1_input -m ./1_mirror -b 50 -l log_file1
    ./exec -n 2 -c ./common -i ./2_input -m ./2_mirror -b 50 -l log_file2
    ./exec -n 3 -c ./common -i ./3_input -m ./3_mirror -b 50 -l log_file3
    
Scripts:
    ./1script.sh new 10 5 2
    cat log_file1 log_file2 log_file3 | ./2script.sh

    
Every client at start creates his mirror folder and also he creates an id file in the common file which is the directory in which all clients share data with. Every client has an input file where he stores his personal data. We create a synchronization program in which all clients will have the data of the other clients. Every client stores his data from his input file to fifo files found in the common directory. In this common directory the other clients will take the data from their personal fifo files. Two clients have 2 fifo files. Client1_to_Client2.fifo and Client2_to_Client1.fifo so they can both store and download data.

In order to achieve good send/receive process we create some rules for these fifo files.
At start we send 2 bytes in which we show if we have a folder(01) or a file(02). After that we send 2 bytes for the length of the name, len bytes for the name, 4 bytes for the size of the file or the folder, and then size bytes for the content of the file if it is a file( If it is a directory we don't send these bytes at all). Having these rules the receiver will know exactly how many bytes he should read and have a clear copy of the files of the other client.

At the log file we have some common rules. Firstly we send two bytes of 0 (00) to show that it is the start of the logfile. After that we send the id of the client. Then we send 2 bytes (01 or 02) to assume if it is a sender file(02) or a receiver file(01). Then we write the name and the size of the file. And lastly if there is a signal like SIGINT or SIQUIT we write 2 bytes (03) to find how many clients exited.

We have a handler for the parent to know when his children send some signals like SIGINT, SIGQUIT OR SIGUSR1. If they send SIGINT, SIGQUIT then parent deletes the whole mirror directory and exits. If they send SIQUSR1 then that means they had a problem sending the data and the parent will let them try again ( 3 times in total). If they fail he aborts them. Also if the children wait for more than 30 seconds(we check that with select function) trying to read then they send a SIGUSR1 signal to father. He kills them and deletes the mirror file of this client who failed.

We use WNOHANG so that we not wait for child to finish and waitpid inform us if the child has finished until this moment. We also have nonblocking in the open of read_from_fifo so that we can use select to check if we can read because otherwise we would not be able to check if we can read because write has not been done yet.

Parent checks if the id is in the common directory. If it is not then he creates a child to delete the mirror directrory of this client which is not in the common dir.

We also store the id files of the other clients to our mirror folder so we can know the process id is the same with the current one. If we had the mirror files of the second client and he leaves and another client with id2 comes then we need to delete the mirror directory of the old client and store the new files.
