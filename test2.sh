#!/bin/bash

# TEST 2


PATH_SOCK=./cs_sock2
PATH_READ=ReadFileFolder

file1=SupportTestDirectory/Moana-poster-palette.jpg
file2=SupportTestDirectory/DecimaSiggraph2017.pdf
file3=SupportTestDirectory/yo/FIGC.html
file4=SupportTestDirectory/yo/sigbusWiki.txt
file5=SupportTestDirectory/yo/FIGC_files/bootstrap.min.js
file6=SupportTestDirectory/yo/FIGC_files/competizioni.css


file7=SupportTestDirectory/yo/FIGC_files/allenamento-full-post-ita-spa.jpg
file8=SupportTestDirectory/yo/FIGC_files/focalpoint-left-05-top-05-src-bel.svg
file9=SupportTestDirectory/yo/FIGC_files/medaglia-olimpiadi.svg
file10=SupportTestDirectory/yo/FIGC_files/player.js
file11=SupportTestDirectory/yo/FIGC_files/slick-theme.css

./server -F config2.txt &

sleep 3

./client -p -t 200 -w ./SupportTestDirectory,n=3 -W $file11,$file10 -f $PATH_SOCK > client1.txt 2>&1

./client -p -t 200 -f $PATH_SOCK -W $file1,$file3,$file4,$file5 -d $PATH_READ -R n=3 > client2.txt 2>&1

./client -p -t 200 -f $PATH_SOCK -W $file2,$file6,$file7,$file8,$file9 -w ./SupportTestDirectory,n=3 > client3.txt 2>&1

sleep 1

killall -HUP memcheck-amd64-
