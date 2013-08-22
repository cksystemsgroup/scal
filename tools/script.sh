for i in 1 2 3 4 5 
do 
  for b in prodcon seqalt enq deq
  do 
    python b8_benchmark.py ${b} /home/ahaas/ppopp14/b8/results/${b}${i}/  
  done 
done
for b in prodcon seqalt enq deq
do 
  python calc_average.py /home/ahaas/ppopp14/b8/results/${b}/ /home/ahaas/ppopp14/b8/results/${b}1/ /home/ahaas/ppopp14/b8/results/${b}2/ /home/ahaas/ppopp14/b8/results/${b}3/ /home/ahaas/ppopp14/b8/results/${b}4/ /home/ahaas/ppopp14/b8/results/${b}5/  
  python make_data_files_threads.py /home/ahaas/ppopp14/b8/results/${b}/  
done 
