g++  -std=c++11 encoder.cpp -o mjpgenc `mysql_config --cflags --libs `  `pkg-config --libs opencv`   -lavcodec -lavformat -lavfilter -lavutil -lswscale -lswresample
