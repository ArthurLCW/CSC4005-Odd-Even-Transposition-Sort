#!/bin/bash
#SBATCH --account=csc4005
#SBATCH --partition=debug
#SBATCH --qos=normal
#SBATCH --ntasks=50
#SBATCH --nodes=4
#SBATCH --output test_result_10000.out



echo "mainmode: " && /bin/hostname
mpirun /pvfsmnt/118010134/assignment/csc4005-assignment-1/build/main /pvfsmnt/118010134/assignment/csc4005-assignment-1/input_data_10000.txt /pvfsmnt/118010134/assignment/csc4005-assignment-1/output_data.txt 
