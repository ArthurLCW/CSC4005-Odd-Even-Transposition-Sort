#!/bin/bash
#SBATCH --account=csc4005
#SBATCH --partition=debug
#SBATCH --qos=normal
#SBATCH --ntasks=32
#SBATCH --nodes=1
#SBATCH --output result_gtest_sort.out
#SBATCH --time=10



echo "mainmode: " && /bin/hostname
mpirun /pvfsmnt/118010134/assignment/csc4005-assignment-1/build/gtest_sort