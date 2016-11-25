SRC=GGk_default.cpp ISN_handler.cpp read_trace.cpp class.cpp common.cpp

default: $(SRC)
	g++ -g -o GGk_default $(SRC) -lm
