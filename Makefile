CAFFE_PATH=/media/ubuntu/aee3f03f-31fc-4296-a788-c0993e0442b3/sdcard/caffe
CUDA_LIB=-Wl,-rpath,'/usr/local/cuda/lib'

OPENCV_FLAG=`pkg-config --cflags --libs opencv`
CAFFE_INCLUDE=-I $(CAFFE_PATH)/include -I $(CAFFE_PATH)/build/include -I $(CAFFE_PATH)/build/src -I /usr/local/cuda/include
CAFFE_LIBARY=-L $(CAFFE_PATH)/build/lib -lglog -lboost_system  -lcaffe -Wl,-rpath,'$(CAFFE_PATH)/build/lib'

TARGET=main
CLASSIFIER=classifier
UTILS=utils

FLAGS=-Wall -g

$(TARGET): $(TARGET).cpp $(CLASSIFIER).h $(CLASSIFIER).o $(UTILS).h $(UTILS).o
	g++ $^ -o $@ $(FLAGS) $(CAFFE_INCLUDE) $(CAFFE_LIBARY) $(CUDA_LIB) $(OPENCV_FLAG)

$(UTILS).o: $(UTILS).cpp
	g++ $^ -c -o $@ $(FLAGS)

$(CLASSIFIER).o: $(CLASSIFIER).cpp
	g++ $^ -c -o $@ $(FLAGS) $(CAFFE_INCLUDE)


clean:
	rm *.o $(TARGET)
