#include <odd-even-sort.hpp>
#include <mpi.h>
#include <iostream>
#include <vector>
#include <limits.h>

namespace sort {
    using namespace std::chrono;

    int get_partner(int idx, int phase, int process_assign){//important: both idx, phase starts from 0.
        int partner;
        if (idx%2==0 && phase%2==0){
            partner = idx + 1;
        }
        else if (idx%2==1 && phase%2==0){
            partner = idx - 1;
        }
        else if (idx%2==1 && phase%2==1){
            partner = idx + 1;
        }
        else if (idx%2==0 && phase%2==1){
            partner = idx - 1;
        }

        if (partner<0 || partner>=process_assign){
            return -1;
        }
        return partner;

    }

    void deal_with_insuffiency(Element *arr1, int total_len, int deal_len){
        Element deal_arr[deal_len];
        for (int i=0; i<deal_len; i++){
            deal_arr[i]=arr1[i+total_len-deal_len];
        }
        std::sort(deal_arr,deal_arr+deal_len);


        for (int i=0; i<deal_len; i++){
            bool deal = false;
            for (int j=0; j<total_len; j++){


                if (arr1[j]>deal_arr[i]){
                    //get the target pos
                    int temp0 = deal_arr[i]; //temp0: the one substitues others
                    int temp1 = arr1[j]; //temp1: the one substitued by others
                    if (j!=i+total_len-deal_len+1){

                        arr1[j]=temp0;

                        for (int k=j+1; k<total_len; k++){

                            temp0 = temp1;
                            temp1 = arr1[k];
                            arr1[k] = temp0;

                        }

                    }


                    deal=true;
                    break;
                }

            }

            if (!deal){
                arr1[total_len-1]=deal_arr[i];
            }

        }
    }

    void merge_sort(Element *arr1, Element *arr2, int len1, int len2){
        std::sort(arr1,arr1+len1);
        std::sort(arr2,arr2+len2);
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

        Element max_val = INT_MAX;
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
            std::cout<<"first initialization!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
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
            ////

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
            std::cout<<"variables: process "<<rank<<": "<<"process_assign: "<<process_assign<<" "<<"data_length: "<<data_length<<" "<<"len_proc: "<<len_proc<<" "<<"last_offset: "<<last_offset<<std::endl;
            Element *recv_buffer = new Element[len_proc];
            Element *local_data = new Element[len_proc];
            MPI_Scatter(begin, len_proc*2, MPI_UNSIGNED, local_data, len_proc*2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);


            for (int phase=0; phase<process_assign; phase++){
                int partner = get_partner(rank, phase, process_assign);

                if (partner!=-1){
                    MPI_Sendrecv(local_data, len_proc*2, MPI_UNSIGNED, partner, 0, recv_buffer, len_proc*2, MPI_UNSIGNED, partner, 0, MPI_COMM_WORLD, &status);
                }

                if (partner!=-1){
                    if (partner<rank){
                        merge_sort(recv_buffer, local_data, len_proc, len_proc);
                    }else{
                        merge_sort(local_data, recv_buffer, len_proc, len_proc);
                    }
                }



                MPI_Gather(local_data,len_proc*2,MPI_UNSIGNED,begin,len_proc*2,MPI_UNSIGNED,0,MPI_COMM_WORLD);

                if (rank==0){
                    if (phase == process_assign-1){
                        deal_with_insuffiency(begin, data_length, last_offset);
                    }

                }
            }

        }

        else if (variables[0]>variables[1]){
            if (0 == rank) {
                std::sort(begin, begin+variables[1]);
//                std::cout<<"current data sorted "<<rank<<": ";
//                for (int i=0;i<variables[1];i++){
//                    std::cout<<begin[i]<<" ";
//                }
//                std::cout<<std::endl;
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
