SRC=GGk_default.cpp handle_arrive.cpp handle_depart.cpp class.cpp common.cpp

default: $(SRC)
	g++ -g -o GGk_default $(SRC) -lm
