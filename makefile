run: main.cpp vdi_read.cpp vdiInfo.cpp
	g++ -std=c++11 main.cpp vdi_read.cpp vdiInfo.cpp -o vdi

clean:
	rm -rf vdi