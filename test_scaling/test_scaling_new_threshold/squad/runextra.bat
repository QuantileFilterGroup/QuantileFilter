g++ csv_header.cpp -o csv_header -O2 -std=c++11 -Wl,--stack=1145141919 -Wall -Wextra -Wconversion
g++ checker.cpp -o checker -O2 -std=c++11 -Wl,--stack=1145141919 -Wall -Wextra -Wconversion
g++ new_main.cpp lossycountwSgk.cc prng.cc rand48.cc -o new_main -O2 -std=c++11  -Wl,--stack=1145141919 -Wall -Wextra -Wconversion

new_main 26121161 0.005 main_14.out 19260817
new_main 26121161 0.002 main_15.out 19260817

checker 15 main_14.out
checker 16 main_15.out

