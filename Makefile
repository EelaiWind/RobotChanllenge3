CAFFE_PATH=/home/tony333ts/workspace/caffe

OPENCV_FLAG=`pkg-config --cflags --libs opencv`
CAFFE_INCLUDE=-I $(CAFFE_PATH)/include/ -I $(CAFFE_PATH)/build/src/ -I /usr/local/cuda/include/
CAFFE_LIBARY=-L $(CAFFE_PATH)/build/lib/ -lglog -lboost_system  -lcaffe -Wl,-rpath,'$(CAFFE_PATH)/build/lib'

TARGET=main
CLASSIFIER=classifier
UTILS=utils


$(TARGET): $(TARGET).cpp $(CLASSIFIER).h $(CLASSIFIER).o $(UTILS).h $(UTILS).o
	g++ $^ -o $@ $(CAFFE_INCLUDE) $(CAFFE_LIBARY) $(OPENCV_FLAG)

$(UTILS).o: $(UTILS).cpp
	g++ $^ -c -o $@

$(CLASSIFIER).o: $(CLASSIFIER).cpp
	g++ $^ -c -o $@ $(CAFFE_INCLUDE)


clean:
	rm *.o $(TARGET)
