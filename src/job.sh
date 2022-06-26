#!/bin/bash
#SBATCH --nodes 2
#SBATCH --ntasks-per-node 16
#SBATCH -t 00:10:00
#SBATCH -o results_1.txt

echo "-----------------6------------------"
mpiexec -n 2 ./graph500_reference_bfs 6 16
echo "-----------------7------------------"
mpiexec -n 2 ./graph500_reference_bfs 7 16
echo "-----------------8------------------"
mpiexec -n 2 ./graph500_reference_bfs 8 16
echo "-----------------9------------------"
mpiexec -n 2 ./graph500_reference_bfs 9 16
echo "-----------------10------------------"
mpiexec -n 2 ./graph500_reference_bfs 10 16
echo "-----------------11------------------"
mpiexec -n 2 ./graph500_reference_bfs 11 16
echo "-----------------12------------------"
mpiexec -n 2 ./graph500_reference_bfs 12 16
echo "-----------------13------------------"
mpiexec -n 2 ./graph500_reference_bfs 13 16
echo "-----------------14------------------"
mpiexec -n 2 ./graph500_reference_bfs 14 16
echo "-----------------15------------------"
mpiexec -n 2 ./graph500_reference_bfs 15 16
echo "-----------------16------------------"
mpiexec -n 2 ./graph500_reference_bfs 16 16
echo "-----------------17------------------"
mpiexec -n 2 ./graph500_reference_bfs 17 16
echo "-----------------18------------------"
mpiexec -n 2 ./graph500_reference_bfs 18 16
echo "-----------------19------------------"
mpiexec -n 2 ./graph500_reference_bfs 19 16
echo "-----------------20------------------"
mpiexec -n 2 ./graph500_reference_bfs 20 16
