#!/bin/bash
export GLOG_minloglevel=2

./main  "vehicle_model/model.prototxt"  "vehicle_model/weights.caffemodel"  "vehicle_model/mean.binaryproto" \
        "cat_model/model.prototxt"      "cat_model/weights.caffemodel"      "cat_model/mean.binaryproto"
