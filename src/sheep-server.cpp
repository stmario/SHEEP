#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <random>
#include <sys/time.h>

#include "sheep-server.hpp"


using namespace std;
using namespace web; 
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;
using namespace SHEEP;

typedef std::chrono::duration<double, std::micro> Duration; /// for storing execution times


/// constructor - bind listener to endpoints and create empty structs for config and result
SheepServer::SheepServer(utility::string_t url) : m_listener(url)
{
    m_job_config = {};  /// zero all fields
    m_job_config.setDefaults();
    m_job_result = {};
    m_job_finished = false;
    /// bind generic methods to listen for any request - the URL will be parsed within these functions.
    m_listener.support(methods::GET, std::bind(&SheepServer::handle_get, this, std::placeholders::_1));
    m_listener.support(methods::PUT, std::bind(&SheepServer::handle_put, this, std::placeholders::_1));
    m_listener.support(methods::POST, std::bind(&SheepServer::handle_post, this, std::placeholders::_1));
   
}

/// templated functions to interact with the contexts.

template <typename PlaintextT>
BaseContext<PlaintextT>*
SheepServer::make_context(std::string context_type) {
  cout<<" in make context "<<context_type<<endl;
  if (context_type == "HElib_F2") {
    return  new ContextHElib_F2<PlaintextT>();    
  } else if (context_type == "HElib_Fp") {
    return new ContextHElib_Fp<PlaintextT>();    
  } else if (context_type == "TFHE") {
    return  new ContextTFHE<PlaintextT>();
    //	} else if (context_type == "SEAL") {
    //    return  new ContextSeal<PlaintextT>();
  } else if (context_type == "Clear") {
    return new ContextClear<PlaintextT>();
  } else {
    throw std::runtime_error("Unknown context requested");
  }
}

template<typename PlaintextT>
std::vector<PlaintextT>
SheepServer::make_plaintext_inputs() {
  std::vector<PlaintextT> inputs;
  for (auto input : m_job_config.input_vals) {
    inputs.push_back( (PlaintextT)(input) );
  }
  return inputs;
}
//// populate the stored m_job_config.parameters map
void
SheepServer::get_parameters() {
  if (m_job_config.parameters.size() == 0) { 
    /// call a function that will create a context and set m_job_config.parameters to default values
    if (m_job_config.input_type == "bool") update_parameters<bool>(m_job_config.context);
    if (m_job_config.input_type == "uint8_t") update_parameters<uint8_t>(m_job_config.context);
    if (m_job_config.input_type == "uint16_t") update_parameters<uint16_t>(m_job_config.context);
    if (m_job_config.input_type == "uint32_t") update_parameters<uint32_t>(m_job_config.context);
    if (m_job_config.input_type == "int8_t") update_parameters<int8_t>(m_job_config.context);
    if (m_job_config.input_type == "int16_t") update_parameters<int16_t>(m_job_config.context);
    if (m_job_config.input_type == "int32_t") update_parameters<int32_t>(m_job_config.context);    
  }
}
template <typename PlaintextT>
void
SheepServer::update_parameters(std::string context_type,
			       json::value parameters) {
  //			       std::string param_name,
  //			       long param_value) {

  BaseContext<PlaintextT>* context;
  if (context_type == "HElib_F2") {
    context = new ContextHElib_F2<PlaintextT>();
  } else if (context_type == "HElib_Fp") {
    context = new ContextHElib_Fp<PlaintextT>();    
  } else if (context_type == "TFHE") {
    context = new ContextTFHE<PlaintextT>();
    //	} else if (context_type == "SEAL") {
    //    context = new ContextSeal<PlaintextT>();
  } else if (context_type == "Clear") {
    context = new ContextClear<PlaintextT>();
  } else {
    throw std::runtime_error("Unknown context requested");
  }
  /// first set parameters to current values stored in the server (if any)
  for (auto map_iter : m_job_config.parameters) {
    cout<<" values in job_config param map are :"<<map_iter.first<<" "<<map_iter.second<<endl;
    context->set_parameter(map_iter.first, map_iter.second);
  }
  /// update parameters if specified
  auto params = parameters.as_object();
  for (auto p : params) {
    std::string param_name = p.first;
    long param_value = (long)(p.second.as_integer());
    context->set_parameter(param_name, param_value);
  }
  /// apply the new parameters
  context->configure();
  //update the servers param map.
  m_job_config.parameters = context->get_parameters();
  // cleanup
  delete context;
}

template <typename PlaintextT>
bool
SheepServer::check_job_outputs(std::vector<PlaintextT> test_outputs,
			       std::vector<PlaintextT> clear_outputs) {
  if (test_outputs.size() != clear_outputs.size()) return false;
  for (int i=0; i< test_outputs.size(); i++) {
    if (test_outputs[i] != clear_outputs[i]) return false;
  }
  return true;
}

template <typename PlaintextT>
void
SheepServer::configure_and_run() {
  if (! m_job_config.isConfigured()) throw std::runtime_error("Job incompletely configured");
  /// we can now assume we have values for context, inputs, circuit, etc
  auto context = make_context<PlaintextT>(m_job_config.context);
  /// set parameters for this context
  if (context != NULL) cout<<"have a context"<<endl;
  else cout<<"null context pointer"<<endl;
  for ( auto map_iter = m_job_config.parameters.begin(); map_iter != m_job_config.parameters.end(); ++map_iter) {
    context->set_parameter(map_iter->first, map_iter->second);
  }
  std::vector<PlaintextT> plaintext_inputs = make_plaintext_inputs<PlaintextT>();
  cout<<"Have "<<plaintext_inputs.size()<<" plaintext inputs"<<endl;
  std::vector<Duration> timings;
  std::vector<PlaintextT> output_vals = context->eval_with_plaintexts(m_job_config.circuit,
  								      plaintext_inputs,
  								      timings,
  								      m_job_config.eval_strategy);
  cout<<" finished running job !! "<<endl;
  m_job_finished = true;
  ///  store outputs values as strings, to avoid ambiguity about type.
  for (int i=0; i < output_vals.size(); ++i ) {    
    std::cout<<" output_vals "<<std::to_string(output_vals[0])<<std::endl;
    auto output = std::make_pair<const std::string,std::string>(
								m_job_config.circuit.get_outputs()[i].get_name(),
								std::to_string(output_vals[i])
								);
    m_job_result.outputs.insert(output);
  }
  if (timings.size() != 3) throw std::runtime_error("Unexpected length of timing vector");
  auto encryption = std::make_pair<std::string, std::string>("encryption",std::to_string(timings[0].count()));
  m_job_result.timings.insert(encryption);
  auto evaluation = std::make_pair<std::string, std::string>("evalation",std::to_string(timings[1].count()));
  m_job_result.timings.insert(evaluation);
  auto decryption = std::make_pair<std::string, std::string>("decryption",std::to_string(timings[2].count()));
  m_job_result.timings.insert(decryption);

  //// now do the plaintext evaluation
  auto clear_context = make_context<PlaintextT>("Clear");
  std::vector<PlaintextT> clear_output_vals = clear_context->eval_with_plaintexts(m_job_config.circuit,
										  plaintext_inputs,
										  timings);
  bool is_correct = check_job_outputs<PlaintextT>(output_vals, clear_output_vals);
  m_job_result.is_correct = is_correct;
}

void SheepServer::handle_get(http_request message)
{
  auto path = message.relative_uri().path();
  if (path == "context/") return handle_get_context(message);  
  else if (path == "parameters/") return handle_get_parameters(message);
  else if (path == "input_type/") return handle_get_input_type(message);
  else if (path == "inputs/") return handle_get_inputs(message);  
  else if (path == "job/") return handle_get_job(message);
  else if (path == "config/") return handle_get_config(message);
  else if (path == "results/") return handle_get_results(message);
  else if (path == "eval_strategy/") return handle_get_eval_strategy(message);
  else message.reply(status_codes::InternalError,("Unrecognized request"));
};

void SheepServer::handle_post(http_request message)
{
  cout<<" in handle post " <<endl;
  auto path = message.relative_uri().path();
  cout<<" path is "<<path<<endl;
  if (path == "context/") return handle_post_context(message);  
  else if (path == "input_type/") return handle_post_input_type(message);
  else if (path == "inputs/") return handle_post_inputs(message);  
  else if (path == "job/") return handle_post_job(message);
  else if (path == "circuit/") return handle_post_circuit(message);
  else if (path == "run/") return handle_post_run(message);
  else message.reply(status_codes::InternalError,("Unrecognized request"));  
};


void SheepServer::handle_put(http_request message)
{
  auto path = message.relative_uri().path();
  if (path == "parameters/") return handle_put_parameters(message);
  else if (path == "eval_strategy/") return handle_put_eval_strategy(message); 
  message.reply(status_codes::OK);
};

/////////////////////////////////////////////////////////////////////

void SheepServer::handle_post_run(http_request message) {
  
  /// get a context, configure it with the stored
  /// parameters, and run it.
  cout<<"Will run job! now"<<endl;
  if (m_job_config.input_type == "bool") configure_and_run<bool>();
  else if (m_job_config.input_type == "uint8_t") configure_and_run<uint8_t>();
  else if (m_job_config.input_type == "uint16_t") configure_and_run<uint16_t>();
  else if (m_job_config.input_type == "uint32_t") configure_and_run<uint32_t>();
  else if (m_job_config.input_type == "int8_t") configure_and_run<int8_t>();
  else if (m_job_config.input_type == "int16_t") configure_and_run<int16_t>();
  else if (m_job_config.input_type == "int32_t") configure_and_run<int32_t>();  
  message.reply(status_codes::OK);
}



void SheepServer::handle_post_circuit(http_request message) {
  /// set circuit filename to use
  cout<<" in handle post circuit" <<endl;
  bool found_circuit = false;
  message.extract_json().then([=](pplx::task<json::value> jvalue) {
      try {
	json::value val = jvalue.get();
	auto circuit_filename = val["circuit_filename"].as_string();
	std::cout<<" circuit is "<<circuit_filename<<std::endl;
	m_job_config.circuit_filename = circuit_filename;
         /// check that the file exists.
        std::ifstream circuit_file(std::string(m_job_config.circuit_filename));
        cout<<" is file good? "<<circuit_file.good()<<endl;
        if (circuit_file.good()) {
	  /// create the circuit
	  Circuit C;
	  circuit_file >> C;
	  cout << C;
	  m_job_config.circuit = C;
	}
      } catch(json::json_exception) {
	  message.reply(status_codes::InternalError,("Unrecognized circuit_filename request"));
      }      
    });
  message.reply(status_codes::OK);
}

void SheepServer::handle_get_inputs(http_request message) {
  
  /// check again that the circuitfile exists.
  std::ifstream circuit_file(std::string(m_job_config.circuit_filename));
  cout<<" is file good? "<<circuit_file.good()<<endl;
  if (! circuit_file.good())
    message.reply(status_codes::InternalError,("Circuit file not found"));
  json::value result = json::value::object();
  json::value inputs = json::value::array();
  int index = 0;
  for (auto input : m_job_config.circuit.get_inputs() ) {
    m_job_config.input_names.insert(input.get_name());
    inputs[index] = json::value::string(input.get_name());
    index++;
  }
  result["inputs"] = inputs;
  message.reply(status_codes::OK, result);
}

void SheepServer::handle_post_inputs(http_request message) {
  message.extract_json().then([=](pplx::task<json::value> jvalue) {
      cout<<"in post inputs"<<endl;
      try {
	json::value input_dict = jvalue.get();
	//	auto input_dict = val["input_dict"].as_object();
	for (auto input_name : m_job_config.input_names) {
	  cout<<" looking for "<<input_name<<endl;
	  int input_val = input_dict[input_name].as_integer();
	  cout<<" input val is "<<input_val<<endl;
	  m_job_config.input_vals.push_back(input_val);
	}
      } catch(json::json_exception) {
	  message.reply(status_codes::InternalError,("Unrecognized inputs"));
      }      
    });
  message.reply(status_codes::OK);
}


void SheepServer::handle_get_eval_strategy(http_request message) {
  /// get the evaluation strategy
  json::value result = json::value::object();
  if (m_job_config.eval_strategy == EvaluationStrategy::parallel) {
    result["eval_strategy"] = json::value::string("parallel");
  } else {
    result["eval_strategy"] = json::value::string("serial");
  }
  message.reply(status_codes::OK, result);
}


void SheepServer::handle_get_context(http_request message) {
  /// list of available contexts?
  json::value result = json::value::object();
  json::value context_list = json::value::array();
  int index = 0;
  for (auto contextIter = available_contexts.begin();
       contextIter != available_contexts.end();
       ++contextIter) {
    context_list[index] = json::value::string(*contextIter);
    index++;
  }
  result["contexts"] = context_list;
  
  message.reply(status_codes::OK, result);
}

void SheepServer::handle_get_input_type(http_request message) {
  /// list of available contexts?
  json::value result = json::value::object();
  json::value type_list = json::value::array();
  int index = 0;
  for (auto typeIter = available_input_types.begin();
       typeIter != available_input_types.end();
       ++typeIter) {
    type_list[index] = json::value::string(*typeIter);
    index++;
  }
  result["input_types"] = type_list;  
  message.reply(status_codes::OK, result);
}


void SheepServer::handle_post_context(http_request message) {
  /// set which context to use.  If it has changed, reset the list of parameters.
  cout<<" in handle post context" <<endl;
  message.extract_json().then([=](pplx::task<json::value> jvalue) {
      try {
	json::value val = jvalue.get();
	auto context = val["context_name"].as_string();
	std::cout<<" context is "<<context<<std::endl;
	if (context != m_job_config.context) {
	  m_job_config.context = context;
	  m_job_config.parameters.clear();
	}
      } catch(json::json_exception) {
	  message.reply(status_codes::InternalError,("Unrecognized context request"));
      }      
    });
  message.reply(status_codes::OK);
}

void SheepServer::handle_post_input_type(http_request message) {
  /// set which input_type to use
  cout<<" in handle post input type" <<endl;
  message.extract_json().then([=](pplx::task<json::value> jvalue) {
      try {
	json::value val = jvalue.get();
	auto input_type = val["input_type"].as_string();
	std::cout<<" input_type is "<<input_type<<std::endl;
	m_job_config.input_type = input_type;
      } catch(json::json_exception) {
	  message.reply(status_codes::InternalError,("Unrecognized context request"));
      }      
    });
  message.reply(status_codes::OK);
}

void SheepServer::handle_put_eval_strategy(http_request message) {
  /// set which eval_strategy to use
  cout<<" in handle put eval_strategy" <<endl;
  message.extract_json().then([=](pplx::task<json::value> jvalue) {
      try {
	json::value val = jvalue.get();
	auto eval_strategy = val["eval_strategy"].as_string();
	std::cout<<" eval_strategy is "<<eval_strategy<<std::endl;
	if (eval_strategy == "parallel") 
	  m_job_config.eval_strategy = EvaluationStrategy::parallel;
	else
	  m_job_config.eval_strategy = EvaluationStrategy::serial;	  
	
      } catch(json::json_exception) {
	  message.reply(status_codes::InternalError,("Unable to set evaluation strategy"));
      }      
    });
  message.reply(status_codes::OK);
}



void SheepServer::handle_get_job(http_request message) {
  /// is the sheep job fully configured?
  bool configured = m_job_config.isConfigured();
  cout<<" in handle_get_job"<<endl;
  json::value result = json::value::object();
  result["job_configured"] = json::value::boolean(configured);
  message.reply(status_codes::OK, result);
}


void SheepServer::handle_post_job(http_request message) {
  // reset the job config and job result structs, we have a new job.
  cout<<"resetting config and results"<<endl;
  m_job_config = {};
  m_job_config.setDefaults();
  m_job_result = {};
  m_job_finished = false;
  message.reply(status_codes::OK);
}


void SheepServer::handle_get_parameters(http_request message) {
  
  /// if the job_config has some parameters set, then return them.
  /// Otherwise, create a new context, and get the default parameters.
  /// return a dict of parameters.
  /// input_type needs to be set already otherwise we can't instantiate context.
  if ((m_job_config.input_type.size() == 0) || (m_job_config.context.size() == 0)) {
    message.reply(status_codes::InternalError,("Need to set input_type and context before getting parameters"));
    return;
  }
  /// call the function to populate m_job_config.parameters
  get_parameters();
  /// build a json object out of it.
  std::map<std::string, long> param_map = m_job_config.parameters;
  json::value result = json::value::object();
  for ( auto map_iter = param_map.begin(); map_iter != param_map.end(); ++map_iter) {
    result[map_iter->first] = json::value::number((int64_t)map_iter->second);
  }
  
  message.reply(status_codes::OK, result);  
}


void SheepServer::handle_get_config(http_request message) {
  /// if we haven't already got the parameters, do this now.
  if (m_job_config.parameters.size() == 0) get_parameters();
  json::value result = m_job_config.as_json();
  message.reply(status_codes::OK, result);
}

void SheepServer::handle_put_parameters(http_request message) {
  /// put parameter name:value into job_config
  if ((m_job_config.input_type.size() == 0) ||
      (m_job_config.context.size() == 0)) {
    message.reply(status_codes::InternalError,("Need to specify input_type and context before setting parameters"));
    return;
  }
  message.extract_json().then([=](pplx::task<json::value> jvalue) {
      try {
	json::value params = jvalue.get();
	/// reset our existing map
	m_job_config.parameters.clear();

	if (m_job_config.input_type == "bool") update_parameters<bool>(m_job_config.context,params);
	else if (m_job_config.input_type == "uint8_t") update_parameters<uint8_t>(m_job_config.context,params);
	else if (m_job_config.input_type == "uint16_t") update_parameters<uint16_t>(m_job_config.context,params);
	else if (m_job_config.input_type == "uint32_t") update_parameters<uint32_t>(m_job_config.context,params);
	else if (m_job_config.input_type == "int8_t") update_parameters<int8_t>(m_job_config.context,params);
	else if (m_job_config.input_type == "int16_t") update_parameters<int16_t>(m_job_config.context,params);
	else if (m_job_config.input_type == "int32_t") update_parameters<int32_t>(m_job_config.context,params);
	else message.reply(status_codes::InternalError,("Unknown input type when updating parameters"));
	  	  
	
      } catch(json::json_exception) {
	  message.reply(status_codes::InternalError,("Unable to set evaluation strategy"));
      }      
    });
  
  message.reply(status_codes::OK);
}

void SheepServer::handle_get_results(http_request message) {

  cout<<"in get_results"<<endl;

  json::value result = m_job_result.as_json();

  message.reply(status_codes::OK, result);
}


