python ./cv/CNN_MIO_KERAS/mio.py -t $1 2> tmp.txt 
rm tmp.txt
#pid=$!
#while [ ps ax | grep -v grep | grep $pid > /tmp/null ]; do 
#	a=1
#done
#echo "done"

