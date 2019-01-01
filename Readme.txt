export PATH="$PATH:/home/$USER/.openmpi/bin"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/$USER/.openmpi/lib/"

clear && mpirun -np 3 3PC | tee log && clear && echo "----- SORTED LOG -----" && cat log | sort
