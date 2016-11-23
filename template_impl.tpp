#include "common.h"

template <typename T>
T* create_1D_array(int x, T init){
	T* in = new T[x];
	for(int a=0;a<x;a++){
		in[a]= init;
	}
	
	return in;

}

template <typename T>
T** create_2D_array(int x, int y, T init){
	T** in = new T*[x];
	for(int a=0;a<x;a++){
		in[a]= new T[y];
		for(int b=0;b<y;b++){
			in[a][b]=init;
		}
	}
	
	return in;

}

template <typename T>
T*** create_3D_array(int x, int y, int z, T init){
	T*** in = new T**[x];
	for(int a=0;a<x;a++){
		in[a]= new T*[y];
		for(int b=0;b<y;b++){
			in[a][b]=new T[z];
			for(int c=0;c<z;c++){
				in[a][b][c]=init;
			}
		}
	}
	
	return in;

}