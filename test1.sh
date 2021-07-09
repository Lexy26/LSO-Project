#!/bin/bash
#versione base del test1.sh
TIMER=0.2
PATH_SOCK=./cs_sock
PATH_READ=ReadFileFolder

file1=SupportTestDirectory/Moana-poster-palette.jpg
file2=SupportTestDirectory/DecimaSiggraph2017.pdf
file3=SupportTestDirectory/yo/FIGC.html
file4=SupportTestDirectory/yo/sigbusWiki.txt
file5=SupportTestDirectory/yo/FIGC_files/bootstrap.min.js
file6=SupportTestDirectory/yo/FIGC_files/competizioni.css


sleep 3
./client -p -f $PATH_SOCK -W $file1,$file2,$file3,$file4,$file5,$file6 -d $PATH_READ -r $file1,$file4,$file3 &  #-r $file2,$file1
./client -p -f $PATH_SOCK -t 200 -d $PATH_READ -R n=9 -w ./SupportTestDirectory,n=10 -d $PATH_READ -r $file1,$file5 #&
#./client -p -t 200 -f $PATH_SOCK -w ./SupportTestDirectory/yo/FIGC_files -R n=100 &
#killall -QUIT memcheck-amd64-
sleep 2
#kill -s HUP $pid
killall -HUP memcheck-amd64-
exit 0
#kill -s 1 $pid
#wait $pid
