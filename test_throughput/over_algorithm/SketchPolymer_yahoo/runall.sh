g++ csv_header.cpp -o csv_header -O2 -std=c++11
g++ checker.cpp -o checker -O2 -std=c++11
g++ new_main.cpp -o new_main -O2 -std=c++11

./csv_header
./new_main 494092 main_04.out
./new_main 985612 main_05.out
./new_main 1968652 main_06.out
./new_main 3934732 main_07.out
./new_main 7866892 main_08.out
./new_main 15731212 main_09.out
./new_main 31459852 main_10.out
./new_main 62917132 main_11.out

./checker 1 main_04.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 2 main_05.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 3 main_06.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 4 main_07.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 5 main_08.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 6 main_09.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 7 main_10.out ../../../answer_yahoo_new_threshold.out 20489969 16877225
./checker 8 main_11.out ../../../answer_yahoo_new_threshold.out 20489969 16877225