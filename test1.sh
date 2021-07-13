#!/bin/bash

# TEST 1


PATH_SOCK=./cs_sock
PATH_READ=ReadFileFolder

file1=SupportTestDirectory/Moana-poster-palette.jpg
file2=SupportTestDirectory/DecimaSiggraph2017.pdf
file3=SupportTestDirectory/yo/FIGC.html
file4=SupportTestDirectory/yo/sigbusWiki.txt
file5=SupportTestDirectory/yo/FIGC_files/bootstrap.min.js
file6=SupportTestDirectory/yo/FIGC_files/competizioni.css

sleep 3

./client -W $file1,$file2 -h -f $PATH_SOCK -t 200
./client -p -t 200 -f $PATH_SOCK -W $file1,$file2,$file3,$file4,$file5,$file6
./client -p -t 200 -f $PATH_SOCK -w ./SupportTestDirectory,n=10 -d $PATH_READ -r $file1,$file5,$file3
./client -d $PATH_READ -R n=100 -p -t 200 -f $PATH_SOCK

# si testa l'operazione -W in modo ripetuto
./client -W $file1,$file2 -d $PATH_READ -r $file1,$file5,$file3 -W $file3,$file4,$file5,$file6 -p -t 200 -f $PATH_SOCK


sleep 1

killall -HUP memcheck-amd64-

