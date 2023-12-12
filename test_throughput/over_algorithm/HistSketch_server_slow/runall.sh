g++ csv_header.cpp -o csv_header -O2 -std=c++11
g++ checker.cpp -o checker -O2 -std=c++11
g++ main_00/new_main.cpp -o main_00.run -O2 -std=c++11
g++ main_01/new_main.cpp -o main_01.run -O2 -std=c++11
g++ main_02/new_main.cpp -o main_02.run -O2 -std=c++11
g++ main_03/new_main.cpp -o main_03.run -O2 -std=c++11
g++ main_04/new_main.cpp -o main_04.run -O2 -std=c++11
g++ main_05/new_main.cpp -o main_05.run -O2 -std=c++11
g++ main_06/new_main.cpp -o main_06.run -O2 -std=c++11
g++ main_07/new_main.cpp -o main_07.run -O2 -std=c++11
g++ main_08/new_main.cpp -o main_08.run -O2 -std=c++11
g++ main_09/new_main.cpp -o main_09.run -O2 -std=c++11
g++ main_10/new_main.cpp -o main_10.run -O2 -std=c++11
g++ main_11/new_main.cpp -o main_11.run -O2 -std=c++11
g++ main_12/new_main.cpp -o main_12.run -O2 -std=c++11
./csv_header
./main_00.run main_00.out
./main_01.run main_01.out
./main_02.run main_02.out
./main_03.run main_03.out
./main_04.run main_04.out
./main_05.run main_05.out
./main_06.run main_06.out
./main_07.run main_07.out
./main_08.run main_08.out
./main_09.run main_09.out
./main_10.run main_10.out
./main_11.run main_11.out
./main_12.run main_12.out

./checker 1 main_00.out
./checker 2 main_01.out
./checker 3 main_02.out
./checker 4 main_03.out
./checker 5 main_04.out
./checker 6 main_05.out
./checker 7 main_06.out
./checker 8 main_07.out
./checker 9 main_08.out
./checker 10 main_09.out
./checker 11 main_10.out
./checker 12 main_11.out
./checker 13 main_12.out