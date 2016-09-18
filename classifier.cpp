#include "classifier.h"

const cv::Size Classifier::IMAGE_SIZE = cv::Size(32, 32);

Classifier::Classifier(const string& modelFile, const string& weightFile, const string& meanFile, uint labelCount)
{
    loadNet(modelFile, weightFile, labelCount);
    loadMeanFile(meanFile);
}

/* Return the top N predictions. */
Prediction Classifier::classify(const Mat& img)
{
    vector<float> output = predict(img);
    uint maxIndex = 0;
    float maxProbability = output.at(0);

    for (uint i = 1; i < output.size(); ++i){
        if ( output.at(i) > maxProbability ){
            maxIndex = i;
            maxProbability = output.at(i);
        }
    }

    return std::make_pair(maxIndex, output[maxIndex]);
}

void Classifier::loadNet(const string& modelFile, const string& weightFile, uint labelCount)
{
    Caffe::set_mode(Caffe::GPU);
    /* Load the network. */
    m_net.reset(new caffe::Net<float>(modelFile, caffe::TEST));
    m_net->CopyTrainedLayersFrom(weightFile);

    CHECK_EQ(m_net->num_inputs(), 1) << "Network should have exactly one input.";
    CHECK_EQ(m_net->num_outputs(), 1) << "Network should have exactly one output.";

    caffe::Blob<float>* inputLayer = m_net->input_blobs()[0];
    m_numChannels = inputLayer->channels();
    CHECK_EQ(m_numChannels, 3) << "Input layer should have 3 channels (RGB image)";
    m_inputGeometry = cv::Size(inputLayer->width(), inputLayer->height());

    caffe::Blob<float>* outputLayer = m_net->output_blobs()[0];
    CHECK_EQ(labelCount, outputLayer->channels()) << "Number of labels is different from the output layer dimension.";
}

/* Load the mean file in binaryproto format. */
void Classifier::loadMeanFile(const string& meanFile)
{
    caffe::BlobProto blobProto;
    caffe::ReadProtoFromBinaryFileOrDie(meanFile.c_str(), &blobProto);

    /* Convert from BlobProto to Blob<float> */
    caffe::Blob<float> meanBlob;
    meanBlob.FromProto(blobProto);
    CHECK_EQ(meanBlob.channels(), m_numChannels) << "Number of channels of mean file doesn't match input layer.";

    /* The format of the mean file is planar 32-bit float BGR or grayscale. */
    vector<Mat> channels;
    float* data = meanBlob.mutable_cpu_data();
    const uint height = meanBlob.height();
    const uint width = meanBlob.width();
    for (uint i = 0; i < m_numChannels; ++i) {
        /* Extract an individual channel. */
        Mat channel(height, width, CV_32FC1, data);
        channels.push_back(channel);
        data += height * width;
    }

    cv::merge(channels, m_mean);
}

vector<float> Classifier::predict(const Mat& img)
{
    caffe::Blob<float>* inputLayer = m_net->input_blobs()[0];
    inputLayer->Reshape(1, m_numChannels, m_inputGeometry.height, m_inputGeometry.width);
    /* Forward dimension change to all layers. */
    m_net->Reshape();

    vector<Mat> inputChannels;
    wrapInputLayer(&inputChannels);
    preprocess(img, &inputChannels);
    m_net->Forward();

    /* Copy the output layer to a vector */
    caffe::Blob<float>* outputLayer = m_net->output_blobs()[0];
    const float* begin = outputLayer->cpu_data();
    const float* end = begin + outputLayer->channels();
    return vector<float>(begin, end);
}

/* Wrap the input layer of the network in separate Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void Classifier::wrapInputLayer(vector<Mat>* inputChannels)
{
    caffe::Blob<float>* inputLayer = m_net->input_blobs()[0];

    const int width = inputLayer->width();
    const int height = inputLayer->height();
    float* input_data = inputLayer->mutable_cpu_data();
    for (int i = 0; i < inputLayer->channels(); ++i) {
        Mat channel(height, width, CV_32FC1, input_data);
        inputChannels->push_back(channel);
        input_data += width * height;
    }
}

void Classifier::cropImage(const Mat& sourceImage, Mat &croppedImage, uint croppedSize)
{
    const uint heightOffset = (sourceImage.rows - croppedSize) / 2;
    const uint widthOffset = (sourceImage.cols - croppedSize) / 2;
    const cv::Rect croppedRegion(widthOffset, heightOffset, croppedSize, croppedSize);
    croppedImage = sourceImage(croppedRegion);
}

void Classifier::preprocess(const Mat& image, vector<Mat>* inputChannels)
{
    /* Convert the input image to the input image format of the network. */
    Mat sample;
    if (image.channels() == 4){
        cv::cvtColor(image, sample, cv::COLOR_BGRA2BGR);
    }
    else if (image.channels() == 1){
        cv::cvtColor(image, sample, cv::COLOR_GRAY2BGR);
    }
    else if (image.channels() == 3){
        sample = image;
    }
    else{
        cerr << "This Classifier is trained for a 3-channels RGB image" << endl;
        exit(1);
    }

    Mat resizedImage;
    if (sample.size() != IMAGE_SIZE){
        cv::resize(sample, resizedImage, IMAGE_SIZE);
    }
    else{
        resizedImage = sample;
    }

    Mat sample_float;
    resizedImage.convertTo(sample_float, CV_32FC3);

    Mat normalizedImage;
    cv::subtract(sample_float, m_mean, normalizedImage);

    static const int CROPPED_SIZE = 28;
    Mat croppedImage;
    cropImage(normalizedImage, croppedImage, CROPPED_SIZE);
    /* This operation will write the separate BGR planes directly to the
     * input layer of the network because it is wrapped by the Mat
     * objects in inputChannels. */
    cv::split(croppedImage, *inputChannels);

    CHECK( reinterpret_cast<float*>(inputChannels->at(0).data) == m_net->input_blobs()[0]->cpu_data()) << "Input channels are not wrapping the input layer of the network.";
}
