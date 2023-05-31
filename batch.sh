#!/bin/bash
: ${hours:=00}
: ${minutes:=00}
: ${dt:=2.0}
: ${dtsave:=10.0}
: ${nrep:=1}
jobdir=job_trpcage-${nrep}r-${nstep}s-${hours}h-${minutes}m
mkdir $jobdir
cp amber99sb.prm trpcage.xyz trpcage.key $jobdir
cd $jobdir
if [ "$nrep" -eq 1 ]; then
  tparams="$nstep $dt $dtsave 4 298 1.0"
else
  tparams="$nstep $dt $dtsave 4 298 370 1.0"
fi
cat <<EOF > job
#!/bin/bash
#SBATCH --account=mp309
#SBATCH -N ${nrep}
#SBATCH -C gpu
#SBATCH -t ${hours}:${minutes}:00

module load cudatoolkit
module load craype-accel-nvidia80
export CRAY_ACCEL_TARGET=nvidia80
export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/opt/nvidia/hpc_sdk/Linux_x86_64/23.1/math_libs/12.0/lib64
export MPICH_GPU_SUPPORT_ENABLED=1

srun --ntasks=$nrep --gpus-per-task=1 --gpu-bind=single:1 ../../build/tinker9 dynamic trpcage.xyz $tparams
EOF
sbatch job
