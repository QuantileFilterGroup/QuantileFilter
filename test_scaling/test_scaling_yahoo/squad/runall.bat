g++ csv_header.cpp -o csv_header -O2 -std=c++11 -Wl,--stack=1145141919 -Wall -Wextra -Wconversion
g++ checker.cpp -o checker -O2 -std=c++11 -Wl,--stack=1145141919 -Wall -Wextra -Wconversion
g++ new_main.cpp lossycountwSgk.cc prng.cc rand48.cc -o new_main -O2 -std=c++11  -Wl,--stack=1145141919 -Wall -Wextra -Wconversion

csv_header
new_main 26121161 0.005 main_00.out 19260817
new_main 26121161 5 main_01.out 19260817
new_main 26121161 2 main_02.out 19260817
new_main 26121161 1 main_03.out 19260817
new_main 26121161 0.8 main_04.out 19260817
new_main 26121161 0.6 main_05.out 19260817
new_main 26121161 0.5 main_06.out 19260817
new_main 26121161 0.4 main_07.out 19260817
new_main 26121161 0.3 main_08.out 19260817
new_main 26121161 0.2 main_09.out 19260817
new_main 26121161 0.1 main_10.out 19260817
new_main 26121161 0.05 main_11.out 19260817
new_main 26121161 0.02 main_12.out 19260817
new_main 26121161 0.01 main_13.out 19260817

checker 1 main_00.out ../../../answer_yahoo.out 20489969 16877225
checker 2 main_01.out ../../../answer_yahoo.out 20489969 16877225
checker 3 main_02.out ../../../answer_yahoo.out 20489969 16877225
checker 4 main_03.out ../../../answer_yahoo.out 20489969 16877225
checker 5 main_04.out ../../../answer_yahoo.out 20489969 16877225
checker 6 main_05.out ../../../answer_yahoo.out 20489969 16877225
checker 7 main_06.out ../../../answer_yahoo.out 20489969 16877225
checker 8 main_07.out ../../../answer_yahoo.out 20489969 16877225
checker 9 main_08.out ../../../answer_yahoo.out 20489969 16877225
checker 10 main_09.out ../../../answer_yahoo.out 20489969 16877225
checker 11 main_10.out ../../../answer_yahoo.out 20489969 16877225
checker 12 main_11.out ../../../answer_yahoo.out 20489969 16877225
checker 13 main_12.out ../../../answer_yahoo.out 20489969 16877225
checker 14 main_13.out ../../../answer_yahoo.out 20489969 16877225