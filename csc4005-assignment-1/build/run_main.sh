#!/bin/bash
#SBATCH --account=csc4005
#SBATCH --partition=debug
#SBATCH --qos=normal
#SBATCH --ntasks=32
#SBATCH --nodes=1
#SBATCH --output result_main.out
#SBATCH --time=10



echo "mainmode: " && /bin/hostname
mpirun /pvfsmnt/118010134/assignment/csc4005-assignment-1/build/main /pvfsmnt/118010134/assignment/csc4005-assignment-1/input_data_100000.txt /pvfsmnt/118010134/assignment/csc4005-assignment-1/output_data_32_10w.txt 
