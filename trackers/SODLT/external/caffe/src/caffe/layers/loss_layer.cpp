// Copyright 2013 Yangqing Jia
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <fstream>

#include "caffe/layer.hpp"
#include "caffe/vision_layers.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/util/io.hpp"

using namespace std;
using std::max;

namespace caffe {

const float kLOG_THRESHOLD = 1e-20;

template <typename Dtype>
void MultinomialLogisticLossLayer<Dtype>::SetUp(
    const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Loss Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 0) << "Loss Layer takes no output.";
  CHECK_EQ(bottom[0]->num(), bottom[1]->num())
      << "The data and label should have the same number.";
  CHECK_EQ(bottom[1]->channels(), 1);
  CHECK_EQ(bottom[1]->height(), 1);
  CHECK_EQ(bottom[1]->width(), 1);
};


template <typename Dtype>
Dtype MultinomialLogisticLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* bottom_data = (*bottom)[0]->cpu_data();
  const Dtype* bottom_label = (*bottom)[1]->cpu_data();
  Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
  int num = (*bottom)[0]->num();
  int dim = (*bottom)[0]->count() / (*bottom)[0]->num();
  memset(bottom_diff, 0, sizeof(Dtype) * (*bottom)[0]->count());
  Dtype loss = 0;
  for (int i = 0; i < num; ++i) {
    int label = static_cast<int>(bottom_label[i]);
    Dtype prob = max(bottom_data[i * dim + label], (Dtype)kLOG_THRESHOLD);
    loss -= log(prob);
    bottom_diff[i * dim + label] = - 1. / prob / num;
  }
  return loss / num;
}

// TODO: implement the GPU version for multinomial loss


template <typename Dtype>
void InfogainLossLayer<Dtype>::SetUp(
    const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Loss Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 0) << "Loss Layer takes no output.";
  CHECK_EQ(bottom[0]->num(), bottom[1]->num())
      << "The data and label should have the same number.";
  CHECK_EQ(bottom[1]->channels(), 1);
  CHECK_EQ(bottom[1]->height(), 1);
  CHECK_EQ(bottom[1]->width(), 1);
  BlobProto blob_proto;
  ReadProtoFromBinaryFile(this->layer_param_.source(), &blob_proto);
  infogain_.FromProto(blob_proto);
  CHECK_EQ(infogain_.num(), 1);
  CHECK_EQ(infogain_.channels(), 1);
  CHECK_EQ(infogain_.height(), infogain_.width());
};


template <typename Dtype>
Dtype InfogainLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* bottom_data = (*bottom)[0]->cpu_data();
  const Dtype* bottom_label = (*bottom)[1]->cpu_data();
  const Dtype* infogain_mat = infogain_.cpu_data();
  Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
  int num = (*bottom)[0]->num();
  int dim = (*bottom)[0]->count() / (*bottom)[0]->num();
  CHECK_EQ(infogain_.height(), dim);
  Dtype loss = 0;
  for (int i = 0; i < num; ++i) {
    int label = static_cast<int>(bottom_label[i]);
    for (int j = 0; j < dim; ++j) {
      Dtype prob = max(bottom_data[i * dim + j], (Dtype)kLOG_THRESHOLD);
      loss -= infogain_mat[label * dim + j] * log(prob);
      bottom_diff[i * dim + j] = - infogain_mat[label * dim + j] / prob / num;
    }
  }
  return loss / num;
}


template <typename Dtype>
void EuclideanLossLayer<Dtype>::SetUp(
  const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Loss Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 0) << "Loss Layer takes no as output.";
  CHECK_EQ(bottom[0]->num(), bottom[1]->num())
      << "The data and label should have the same number.";
  CHECK_EQ(bottom[0]->channels(), bottom[1]->channels());
  CHECK_EQ(bottom[0]->height(), bottom[1]->height());
  CHECK_EQ(bottom[0]->width(), bottom[1]->width());
  difference_.Reshape(bottom[0]->num(), bottom[0]->channels(),
      bottom[0]->height(), bottom[0]->width());
}

template <typename Dtype>
Dtype EuclideanLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down, vector<Blob<Dtype>*>* bottom) {
  int count = (*bottom)[0]->count();
  int num = (*bottom)[0]->num();
  caffe_sub(count, (*bottom)[0]->cpu_data(), (*bottom)[1]->cpu_data(),
      difference_.mutable_cpu_data());
  Dtype loss = caffe_cpu_dot(
      count, difference_.cpu_data(), difference_.cpu_data()) / num / Dtype(2);
  // Compute the gradient
  caffe_axpby(count, Dtype(1) / num, difference_.cpu_data(), Dtype(0),
      (*bottom)[0]->mutable_cpu_diff());
  return loss;
}

template <typename Dtype>
void AccuracyLayer<Dtype>::SetUp(
  const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Accuracy Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 1) << "Accuracy Layer takes 1 output.";
  CHECK_EQ(bottom[0]->num(), bottom[1]->num())
      << "The data and label should have the same number.";
  CHECK_EQ(bottom[1]->channels(), 1);
  CHECK_EQ(bottom[1]->height(), 1);
  CHECK_EQ(bottom[1]->width(), 1);
  (*top)[0]->Reshape(1, 2, 1, 1);
}

template <typename Dtype>
void AccuracyLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  ofstream out,out2;
  if(this->layer_param_.has_data_dump() && !this->layer_param_.has_label_dump()){
    LOG(FATAL)<<"data dump and label dump files should exist at the same time.";
  }
  if(!this->layer_param_.has_data_dump() && this->layer_param_.has_label_dump()){
    LOG(FATAL)<<"data dump and label dump files should exist at the same time.";
  }
  if(this->layer_param_.has_data_dump()){
    out.open(this->layer_param_.data_dump().c_str(),ios::out|ios::app);
  }
  if(this->layer_param_.has_label_dump()){
    out2.open(this->layer_param_.label_dump().c_str(),ios::out|ios::app);
  }
  Dtype accuracy = 0;
  Dtype logprob = 0;
  const Dtype* bottom_data = bottom[0]->cpu_data();
  const Dtype* bottom_label = bottom[1]->cpu_data();
  int num = bottom[0]->num();
  int dim = bottom[0]->count() / bottom[0]->num();
  for (int i = 0; i < num; i++) {
    // Accuracy
  //  out<<"new vector"<<endl;
    Dtype maxval = -FLT_MAX;
    Dtype eps = 1e-6;
    int max_id = 0;
    for (int j = 0; j < dim; ++j) {  
	    if(Caffe::phase()==Caffe::TEST){
		    if(this->layer_param_.has_data_dump()){
          if(bottom_data[i*dim+j]>=eps){
            out<<bottom_data[i*dim+j]<<" ";
          } else {
            out<<0<<" ";
          }
          if(j==dim-1)
            out<<endl;
        }
      }
      if (bottom_data[i*dim+j]> maxval) {
        maxval = bottom_data[i*dim+j];
        max_id = j;
      }
    }
    if(Caffe::phase()==Caffe::TEST){
      if(this->layer_param_.has_label_dump()){
    	  out2<<(int)bottom_label[i]<<endl;
      }
    }
    if (max_id == (int)bottom_label[i]) {
      ++accuracy;
    }
    Dtype prob = max(bottom_data[i * dim + (int)bottom_label[i]], (Dtype)kLOG_THRESHOLD);
   //  Dtype prob = max(val[(int)bottom_label[i]], KLOG_THRESHOLD);
     logprob -= log(prob);
  }
  // LOG(INFO) << "Accuracy: " << accuracy;
  (*top)[0]->mutable_cpu_data()[0] = accuracy / num;
  (*top)[0]->mutable_cpu_data()[1] = logprob / num;
	if(this->layer_param_.has_data_dump()){
    out.close();
  }
  if(this->layer_param_.has_label_dump()){
    out2.close();
  }
}

template <typename Dtype>
void L2HingeLossLayer<Dtype>::SetUp(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Hinge Loss Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 0) << "Hinge Loss Layer takes no output.";
  //C_ =  this->layer_param_.hinge_loss_param().C();
  C_ = 1;
}

template <typename Dtype>
void L2HingeLossLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* bottom_diff = bottom[0]->mutable_cpu_diff();
  const Dtype* label = bottom[1]->cpu_data();
  int num = bottom[0]->num();
  int count = bottom[0]->count();
  int dim = count / num;

  caffe_copy(count, bottom_data, bottom_diff);
  for (int i = 0; i < num; ++i) {
    bottom_diff[i * dim + static_cast<int>(label[i])] *= -1;
  }
  for (int i = 0; i < num; ++i) {
    for (int j = 0; j < dim; ++j) {
      bottom_diff[i * dim + j] = max(Dtype(0), 1 + bottom_diff[i * dim + j]);
    }
  }
  
}

template <typename Dtype>
Dtype L2HingeLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down, vector<Blob<Dtype>*>* bottom) {
  Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
  const Dtype* label = (*bottom)[1]->cpu_data();
  int num = (*bottom)[0]->num();
  int count = (*bottom)[0]->count();
  int dim = count / num;

 // caffe_cpu_sign(count, bottom_diff, bottom_diff);
  for (int i = 0; i < num; ++i) {
    bottom_diff[i * dim + static_cast<int>(label[i])] *= -1;
  }
  caffe_scal(count, Dtype(C_ / num), bottom_diff);
  return caffe_cpu_asum(count, bottom_diff) / num * C_;
}

template <typename Dtype>
void MultiLabelLogisticLossLayer<Dtype>::SetUp(
    const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Loss Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 0) << "Loss Layer takes no output.";
  CHECK_EQ(bottom[0]->num(), bottom[1]->num())
      << "The data and label should have the same number.";
  CHECK_EQ(bottom[1]->height(), 1);
  CHECK_EQ(bottom[1]->width(), 1);
};


template <typename Dtype>
Dtype MultiLabelLogisticLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* bottom_data = (*bottom)[0]->cpu_data();
  const Dtype* bottom_label = (*bottom)[1]->cpu_data();
  Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
  int num = (*bottom)[0]->num();
  int dim = (*bottom)[0]->count() / (*bottom)[0]->num();
  memset(bottom_diff, 0, sizeof(Dtype) * (*bottom)[0]->count());
  Dtype loss = 0;
  for (int i = 0; i < num; ++i) {
    for (int j = 0; j < dim; ++j){
      int label = static_cast<int>(bottom_label[i * dim + j]);
      //if (label == 1) LOG(INFO) << i << " " << j;
      Dtype prob = 1 / (1 + exp(-bottom_data[i * dim + j]));
      prob = max(prob, (Dtype)kLOG_THRESHOLD);
      bottom_diff[i * dim + j] = (prob - (label == 1));
      //if (label == 1){
      //  bottom_diff[i * dim + j] *= 10;
      //}
      //LOG(INFO) << label << " " <<  bottom_data[i * dim + j] << " " << bottom_diff[i * dim + j];
      loss -= (label == 1 ? log(prob + kLOG_THRESHOLD) : log(1 - prob + kLOG_THRESHOLD));
    }
  }
  //LOG(INFO) << loss;
  return loss / num;
}

template <typename Dtype>
void VOCLossLayer<Dtype>::SetUp(
    const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 2) << "Loss Layer takes two blobs as input.";
  CHECK_EQ(top->size(), 0) << "Loss Layer takes no output.";
  CHECK_EQ(bottom[0]->num(), bottom[1]->num() * bottom[1]->channels())
      << "The data and label should have the same number.";
  CHECK_EQ(bottom[1]->height(), 1);
  CHECK_EQ(bottom[1]->width(), 1);
};


template <typename Dtype>
Dtype VOCLossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* bottom_data = (*bottom)[0]->cpu_data();
  const Dtype* bottom_label = (*bottom)[1]->cpu_data();
  Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
  int num = (*bottom)[1]->num();
  int dim = (*bottom)[1]->count() / (*bottom)[1]->num();
  memset(bottom_diff, 0, sizeof(Dtype) * (*bottom)[0]->count());
  Dtype loss = 0;
  for (int i = 0; i < num; ++i) {
    for (int j = 0; j < dim; ++j){
      int label = static_cast<int>(bottom_label[i * dim + j]);
      Dtype prob = 1 / (1 + exp(-bottom_data[(i * dim + j) * dim + j]));
      //LOG(INFO) << bottom_data[(i * dim + j) * dim + j];
      prob = max(prob, (Dtype)kLOG_THRESHOLD);
      bottom_diff[(i * dim + j) * dim + j] = (prob - (label == 1)) / num / dim;
      // if (label == 1){
      //   bottom_diff[(i * dim + j) * dim + j] *= 10;
      // }
      // LOG(INFO) << bottom_data[(i * dim + j) * dim + j] << " " << 
        // bottom_diff[(i * dim + j) * dim + j];
      loss -= (label == 1 ? log(prob) : log(1 - prob));
    }
  }
  LOG(INFO) << loss;
  return loss / num;
}

INSTANTIATE_CLASS(VOCLossLayer);
INSTANTIATE_CLASS(MultinomialLogisticLossLayer);
INSTANTIATE_CLASS(InfogainLossLayer);
INSTANTIATE_CLASS(EuclideanLossLayer);
INSTANTIATE_CLASS(AccuracyLayer);
INSTANTIATE_CLASS(L2HingeLossLayer);
INSTANTIATE_CLASS(MultiLabelLogisticLossLayer);

}  // namespace caffe
