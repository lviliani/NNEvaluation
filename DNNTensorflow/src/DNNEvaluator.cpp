#include <memory>
#include "NNEvaluation/DNNTensorflow/interface/DNNEvaluator.hh"

#include <iostream>
#include <fstream>

NNEvaluation::DNNEvaluator::DNNEvaluator(const std::string modelPath)
    : modelPath_(modelPath)
    , graphDef_(nullptr)
    , session_(nullptr)
{
    // show tf debug logs
    tensorflow::setLogging("0");
    
    // Parse scaler configuration
    std::string scalerPath = modelPath_ + "scaler.txt";
    std::ifstream inputfile_scaler{scalerPath};
    std::cout << "Reading file: "<< scalerPath << std::endl;
    if(inputfile_scaler.fail())  
    { 
        std::cout << "error" << std::endl;
        exit(1);
    }else{ 
        // Now read mean, scale factors for each variable
        float m_,s_;
        std::string varname{};
        while (inputfile_scaler >> varname >> m_ >> s_){
            scaler_factors_.push_back({m_, s_});
            std::cout << "variable: "<< varname << " " << m_ << " " << s_ << std::endl; 
        }  
    }   
    inputfile_scaler.close();

    // Save number of variables
    n_inputs_ = scaler_factors_.size();
    std::cout << "Working with " << n_inputs_ << " variables" << std::endl;

    // Parse TF metadata
    std::string tfmetadataPath = modelPath_ + "tf_metadata.txt";
    std::ifstream inputfile_tfmetadata{tfmetadataPath};
    std::cout << "Reading file: "<< tfmetadataPath << std::endl;
    if(inputfile_tfmetadata.fail())  
    { 
        std::cout << "error" << std::endl;
        exit(1);
    }else{ 
        // The first line contains the inputand output tensornames
        inputfile_tfmetadata >> input_tensor_name_ >> output_tensor_name_;
    }   
    inputfile_tfmetadata.close();

    // Initialise the sessions: import TF graph
    // initialise(); // refactored!
    // load the graph
    std::string graphPath = modelPath_ + "model.pb";
    graphDef_ = tensorflow::loadGraphDef(graphPath); // TODO check what happens if file not present
    // create a new session and add the graphDef
    session_ = tensorflow::createSession(graphDef_);
    std::cout << "Tensorflow session ready" <<std::endl;
}

NNEvaluation::DNNEvaluator::~DNNEvaluator()
{
    // close the session
    tensorflow::closeSession(session_);
    session_ = nullptr;

    // delete the graph
    delete graphDef_;
    graphDef_ = nullptr;
}

float NNEvaluation::DNNEvaluator::scale_variable(int var_index, float & var){
    auto [mean,scale] = scaler_factors_.at(var_index);
    return (var - mean) / scale;
}

float NNEvaluation::DNNEvaluator::analyze(std::vector<float> data)
{
    tensorflow::Tensor input(tensorflow::DT_FLOAT, { 1, n_inputs_ });
    float* d = input.flat<float>().data();
    for (uint i = 0; i < n_inputs_; i++, d++)
    {
        *d = scale_variable(i, data[i]);
        //std::cout << data[i] << "(" << *d << ") | ";
    }

    // define the output and run
    std::vector<tensorflow::Tensor> outputs;
    tensorflow::run(session_, { { input_tensor_name_, input } }, { output_tensor_name_ }, &outputs);

    float result = outputs[0].matrix<float>()(0, 0);
    //std::cout << "----> " << result << std::endl;
    return result;
}

