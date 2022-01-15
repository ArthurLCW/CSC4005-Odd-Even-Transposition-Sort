#include <odd-even-sort.hpp>
#include <mpi.h>
#include <iostream>
#include <vector>
#include <limits.h>

namespace sort {
    using namespace std::chrono;

    //partner = neighbour. get the right neighbour of a proc
    int get_partner_right_process(int idx, int phase, int process_assign){//important: both idx, phase starts from 0.
        int partner;
        if (idx!=process_assign-1){
            partner = idx+1;
        }else{
            partner = -1;
        }
        return partner;
    }

    //partner = neighbour. get the left neighbour of a proc
    int get_partner_left_process(int idx, int phase, int process_assign){//important: both idx, phase starts from 0.
        int partner;
        if (idx!=0){
            partner = idx-1;
        }else{
            partner = -1;
        }
        return partner;
    }

    //partner = neighbour. get the switching element's index in a local array
    int get_partner_within_process(int process_rank, int element_idx, int phase, int process_length){//important: both idx, phase starts from 0.
        int partner;
        if (process_length%2==0){
            if (element_idx%2==0 && phase%2==0){
                partner = element_idx + 1;
            }
            else if (element_idx%2==1 && phase%2==0){
                partner = element_idx - 1;
            }
            else if (element_idx%2==1 && phase%2==1){
                partner = element_idx + 1;
            }
            else if (element_idx%2==0 && phase%2==1){
                partner = element_idx - 1;
            }
        }
        else if (process_length%2==1){
            if (process_rank%2==0){
                if (element_idx%2==0 && phase%2==0){
                    partner = element_idx + 1;
                }
                else if (element_idx%2==1 && phase%2==0){
                    partner = element_idx - 1;
                }
                else if (element_idx%2==1 && phase%2==1){
                    partner = element_idx + 1;
                }
                else if (element_idx%2==0 && phase%2==1){
                    partner = element_idx - 1;
                }
            }
            else if (process_rank%2==1){
                if (element_idx%2==0 && phase%2==0){
                    partner = element_idx - 1;
                }
                else if (element_idx%2==1 && phase%2==0){
                    partner = element_idx + 1;
                }
                else if (element_idx%2==1 && phase%2==1){
                    partner = element_idx - 1;
                }
                else if (element_idx%2==0 && phase%2==1){
                    partner = element_idx + 1;
                }
            }
        }

        if (partner<0 || partner>=process_length){
            return -1;
        }
        return partner;
    }

    //swap in local array in odd/even phases
    void local_data_sort(Element *local_arr, int phase, int process_length, int rank){
        for (int i=0; i<process_length; i++){
            int partner = get_partner_within_process(rank, i, phase, process_length);
            if (partner!=-1){
                if ((partner>i) && (local_arr[i]>local_arr[partner])){//swap value
                    Element temp = local_arr[i];
                    local_arr[i] = local_arr[partner];
                    local_arr[partner] = temp;
                }
            }
        }
    }

    //only used in the final phase to put the sorted array and remainder array together
    //if the data length cannot be divided by num of proc without remainder, this function will be used.
    //for instance, if there are 15 number with 4 procs, each proc get 3 num, sorted into a 12 length array.
    //then, merge the 12 length sorted array with the left 3 remainder numbers. 
    //param: arr1: the sorted big array; arr2: the remainder array
    void merge_sorted_big_array_and_remainder_array(Element *arr1, Element *arr2, int len1, int len2){
        
        std::sort(arr2,arr2+len2); 
        //sorted the remainder array. Only len2(less than the length of a proc) will be sorted this way.
        //arr1 is not sorted this way. the program is still implementing parallely
        int i=0;
        int j=0;
        Element arrnew[len1+len2];
        for (int k=0; k<len1+len2; k++){
            if (i==len1){
                arrnew[k]=arr2[j];
                j++;
            }
            else if (j==len2){
                arrnew[k]=arr1[i];
                i++;
            }

            else if (arr1[i]<=arr2[j]){
                arrnew[k]=arr1[i];
                i++;
            }else{
                arrnew[k]=arr2[j];
                j++;
            }
        }
        for (int k=0; k<len1+len2;k++){
            if (k<len1){
                arr1[k]=arrnew[k];
            }else{
                arr2[k-len1]=arrnew[k];
            }
        }
    }

    Context::Context(int &argc, char **&argv) : argc(argc), argv(argv) {
        MPI_Init(&argc, &argv);
    }

    Context::~Context() {
        MPI_Finalize();
    }

    std::unique_ptr<Information> Context::mpi_sort(Element *begin, Element *end) const {
        int res;
        int rank;
        std::unique_ptr<Information> information{};

        res = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (MPI_SUCCESS != res) {
            throw std::runtime_error("failed to get MPI world rank");
        }

        int *variables = new int[4];
        MPI_Status status;

        if (0 == rank) {
            information = std::make_unique<Information>();//by me
            information->length = end - begin;
            res = MPI_Comm_size(MPI_COMM_WORLD, &information->num_of_proc);
            if (MPI_SUCCESS != res) {
                throw std::runtime_error("failed to get MPI world size");
            };
            information->argc = argc;
            for (auto i = 0; i < argc; ++i) {
                information->argv.push_back(argv[i]);
            }
            information->start = high_resolution_clock::now();

            variables[0] = information->num_of_proc;
            variables[1] = information->length; //data_length
            variables[2] = information->length/variables[0];//len_proc
            variables[3] = variables[1] - variables[0] * variables[2];//last_offset

        }

        MPI_Bcast(variables,4,MPI_INT,0,MPI_COMM_WORLD);

        if (variables[0]<=variables[1]){
            /// now starts the main sorting procedure
            /// @todo: please modify the following code
            int process_assign=variables[0];
            int data_length=variables[1];
            int len_proc=variables[2];
            int last_offset=variables[3];
            Element *recv_buffer = new Element[1];
            Element *send_buffer = new Element[1];
            Element *local_data = new Element[len_proc];
            MPI_Scatter(begin, len_proc*2, MPI_UNSIGNED, local_data, len_proc*2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);


            for (int phase=0; phase<data_length; phase++){
                local_data_sort(local_data, phase, len_proc, rank);

                MPI_Barrier(MPI_COMM_WORLD);

                int right_proc = get_partner_right_process(rank, phase, process_assign);
                int left_proc = get_partner_left_process(rank, phase, process_assign);

                // modify send recv based on different situations.
                if (len_proc%2==0 && phase%2==1){
                    if (rank==0){
                        if (right_proc!=-1){
                            send_buffer[0] = local_data[len_proc-1];
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD);
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            if (local_data[len_proc-1] > recv_buffer[0]){
                                local_data[len_proc-1] = recv_buffer[0];
                            }
                        }
                    }
                    else if (rank==process_assign) {
                        if (left_proc!=-1){
                            send_buffer[0] = local_data[0];
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//recv left
                            if (local_data[0] < recv_buffer[0]){
                                local_data[0] = recv_buffer[0];
                            }
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD);
                        }

                    }
                    else{
                        if (left_proc!=-1){
                            send_buffer[0] = local_data[0];
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//recv left
                            if (local_data[0] < recv_buffer[0]){
                                local_data[0] = recv_buffer[0];
                            }
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD);
                        }

                        if (right_proc!=-1){
                            send_buffer[0] = local_data[len_proc-1];
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD);
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            if (local_data[len_proc-1] > recv_buffer[0]){
                                local_data[len_proc-1] = recv_buffer[0];
                            }
                        }
                    }

                }

                else if (len_proc%2==0 && phase%2==0){

                }

                else if (len_proc%2==1 && phase%2==0){
                    if (rank%2==0 && rank!=process_assign-1){
                        if (right_proc!=-1){
                            send_buffer[0] = local_data[len_proc-1];
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD);
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            if (local_data[len_proc-1] > recv_buffer[0]){
                                local_data[len_proc-1] = recv_buffer[0];
                            }
                        }
                    }
                    else if (rank%2==1){
                        if (left_proc!=-1){
                            send_buffer[0] = local_data[0];
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//recv left
                            if (local_data[0] < recv_buffer[0]){
                                local_data[0] = recv_buffer[0];
                            }
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD);
                        }
                    }
                }

                else if (len_proc%2==1 && phase%2==1){
                    if (rank%2==1 && rank!=process_assign-1){
                        if (right_proc!=-1){
                            send_buffer[0] = local_data[len_proc-1];
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD);
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, right_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//recv left
                            if (local_data[len_proc-1] > recv_buffer[0]){
                                local_data[len_proc-1] = recv_buffer[0];
                            }
                        }
                    }
                    else if (rank%2==0 && rank!=0){
                        if (left_proc!=-1){
                            send_buffer[0] = local_data[0];
                            MPI_Recv(recv_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//recv left
                            if (local_data[0] < recv_buffer[0]){
                                local_data[0] = recv_buffer[0];
                            }
                            MPI_Send(send_buffer, 2, MPI_UNSIGNED, left_proc, 0, MPI_COMM_WORLD);
                        }
                    }
                }

                MPI_Gather(local_data,len_proc*2,MPI_UNSIGNED,begin,len_proc*2,MPI_UNSIGNED,0,MPI_COMM_WORLD);

                if (rank==0){
                    if (phase == data_length-1){
                        Element *offset_begin = begin+(data_length-last_offset);
                        merge_sorted_big_array_and_remainder_array(begin, offset_begin, data_length-last_offset, last_offset); // only execute for one time to deal with remainder element

//                        deal_with_insuffiency(begin, data_length, last_offset);

//                        std::cout<<"current data sorted "<<rank<<": ";
//                        for (int i=0;i<data_length;i++){
//                            std::cout<<begin[i]<<" ";
//                        }
//                        std::cout<<std::endl;
                    }

                }
            }

        }

        else if (variables[0]>variables[1]){ //when process number is even larger than data length
            if (0 == rank) {
                std::sort(begin, begin+variables[1]);
            }
        }

        if (0 == rank) {
            information->end = high_resolution_clock::now();
        }
        return information;
    }

    std::ostream &Context::print_information(const Information &info, std::ostream &output) {
        auto duration = info.end - info.start;
        auto duration_count = duration_cast<nanoseconds>(duration).count();
        auto mem_size = static_cast<double>(info.length) * sizeof(Element) / 1024.0 / 1024.0 / 1024.0;
        output << "input size: " << info.length << std::endl;
        output << "proc number: " << info.num_of_proc << std::endl;
        output << "duration (ns): " << duration_count << std::endl;
        output << "throughput (gb/s): " << mem_size / static_cast<double>(duration_count) * 1'000'000'000.0
               << std::endl;
        return output;
    }
}
