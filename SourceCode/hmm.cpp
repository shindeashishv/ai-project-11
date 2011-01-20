// Hidden Markov model class
// Hand writing recognition Januari project, MSc AI, University of Amsterdam
// Thijs Kooi, 2011

#include "hmm.h"

// To do:

// - Do hand calculations on forward/backward probability and probability of observation sequences, for different models (2).
// - Do hand calculations for viterbi sequence
// - check for possible underflow forward/backward probability, normalise/log probabilities
// - Test to higher dimensional observations
// - Test Gaussian/MOG
// - Check model with HMM toolkit available
// - In time, make a seperate LA library, such that the HMM doesnt need to call the GMM for math functions

int main()
{
	//Testing
	//GMM(data_dimension, mixture_components)
	GMM mog(4,1);
	
	vector<GMM> mixture_models;
	for(size_t i = 0; i < 3; ++i)
		mixture_models.push_back(mog);
	
	HMM markov_model(3,mixture_models);

	double **test_obs = markov_model.readTestFile(8,4);

// 	cout << "Observation sequence probability " << markov_model.observationSequenceProbability(test_obs,8) << endl;
// 	markov_model.printTransitionProbabilities();
	markov_model.trainModel(test_obs,8);
}

//Reads a file of observations
//Assumes every line has one observation
//Takes as argument the dimension of the observation
double** HMM::readTestFile(int sequence_length, int dimension)
{
	string line;
	observation_sequence_length = sequence_length;
	double** output = new double*[sequence_length];
	double* observation;
	int obs_count = 0;
	ifstream observations ("test_observations");
	if(!observations.is_open()) cout << "Unable to open file test_observations" << endl;
	else
	{
		while(!observations.eof())
		{
			getline(observations,line);
			output[obs_count] = processLine(line,dimension);
			++obs_count;
		}
	}
	return output;
}

double* HMM::processLine(string line, int dim)
{
	double *obs = new double[dim];
	int i = 0;
	int j = line.find_first_of(" ");
	int it = 0;
	
	while(j != -1)
	{
		obs[it] = atof(line.substr(i,j-i).c_str());
		i=j+1;
		j = line.find_first_of(" ",i);
		++it;
	}
	obs[it] = atof(line.substr(i,line.size()-i).c_str());
	
	return obs;
}

//Constructors and initialisation functions
//When no further model parameters are passed, the model is initialised uniform
//In the case of discrete observations, no takes the number of possible observations
//Please note that the observations should not be passed as actual values, but as indexes of the vocabulary.
HMM::HMM(int ns, int no, int od) 
{ 
	number_of_states = ns;
	number_of_observations = no;
	observation_dimension = od;
	initialiseUniform();
}

HMM::HMM(int ns, int no, int od, double* p, map<int, map<int, double> > t, map<int, map<int, map<int, double> > > o)
{
	number_of_states = ns;
	number_of_observations = no;
	observation_dimension = od;
	prior_probabilities = p;
	transition_probabilities = t;
	observation_probabilities = o;
}

//Initialise the HMM with ns states and a mixture of Gaussians, corresponding to every state
//Note that this can also be a mixture of just 1 Gaussian
HMM::HMM(int ns, vector<GMM> MOG)
{
	number_of_states = ns;
	mixture_model = MOG;
	observation_dimension = mixture_model[0].getDimension();
	if(mixture_model[0].getMixtureComponents() == 1)
		gaussian = 1;
	else 
		gaussian = 2;
	initialiseUniform();
}

void HMM::initialiseUniform() 
{ 
	//Initialise elements of transition map
	for(size_t i = 0; i < number_of_states; ++i)
		for(size_t j = 0; j < number_of_states; ++j)
			transition_probabilities[i][j] = 1.0;
		
	//Initialise uniform probabilities
	for(map<int, map<int,double> >::iterator i = transition_probabilities.begin(); i != transition_probabilities.end(); ++i)
		for(map<int, double>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
			(*j).second/=(*i).second.size();
		
	prior_probabilities = new double[number_of_states];
	//Initialise uniform prior probabilities
	for(size_t i = 0; i < number_of_states; ++i)
		prior_probabilities[i] = 1.0/number_of_states;
	
	if(!gaussian)
		initialiseUniformObservations();
}

void HMM::initialiseUniformObservations()
{
	for(size_t i = 0; i < number_of_states; ++i)
		for(size_t j = 0; j < number_of_observations; ++j)
			for(size_t d = 0; d < observation_dimension; ++d)
				observation_probabilities[i][j][d] = 1.0;
		
	for(map<int, map<int, map<int,double> > >::iterator i = observation_probabilities.begin(); i != observation_probabilities.end(); ++i)
		for(map<int, map<int, double> >::iterator j = i->second.begin(); j != i->second.end(); ++j)
			for(map<int,double>::iterator d = j->second.begin(); d != j->second.end(); ++d)
				d->second = pow(1.0/i->second.size(), 1.0/observation_dimension);
}

//End constructors and initialisation functions

//Getters and setters
int HMM::getStates(){ return number_of_states;  }
int HMM::getNumberOfObservations(){ return number_of_observations; }
int HMM::getObservationDimension(){ return observation_dimension; }
//End getters and setters

//Training functions
void HMM::trainModel(double** observation_sequence, int length)
{
	observations = observation_sequence;
	observation_sequence_length = length;
	
	cout << "Training model..." << endl;
	printObservations();
	
	double previous_likelihood = 0.0;
	current_likelihood = 1.0;
	int it = 0;
	
	while(current_likelihood - previous_likelihood > 0.0)
	{
		eStep();
		mStep();
		previous_likelihood = current_likelihood;
		current_likelihood = observationSequenceProbability(observation_sequence,length);
		cout << "Likelihood at iteration " << it+1 << ": " << current_likelihood << endl;
		++it;
	}
	
	cout << "Converged after " << it << " iterations, with likelihood " << current_likelihood << endl;
}

void HMM::eStep() 
{
	//Pre compute forward/backward probability
	for(size_t t = 0; t < observation_sequence_length; ++t)
	{
		cout << "t " << t << endl;
		for(size_t i = 0; i < number_of_states; ++i)
		{
			alpha[i][t] = forwardProbability(i,t);
			beta[i][observation_sequence_length-t-1] = backwardProbability(i,observation_sequence_length-t-1);
		}
	}
	
	for(size_t i = 0; i < number_of_states; ++i)
			for(size_t t = 0; t < observation_sequence_length; ++t)
				gamma[i][t] = stateProbability(i,t);
	if(gaussian == 2)
		for(size_t i = 0; i < number_of_states; ++i)
			for(size_t t = 0; t < observation_sequence_length; ++t)
				for(size_t k = 0; k < mixture_model[0].getMixtureComponents(); ++k)
					gmm_gamma[i][t][k] = stateProbability(i,t,k);
	
	for(size_t i = 0; i < number_of_states; ++i)
		for(size_t j = 0; j < number_of_states; ++j)
			for(size_t t = 0; t < observation_sequence_length-1; ++t)
				xi[i][j][t] = stateToStateProbability(i,j,t);
}

//Generally denoted b_j(o_{t}) in the literature
double HMM::observationProbability(int state, int timestep)
{
	if(!gaussian)
	{
		double probability = 1.0;
		for(size_t d = 0; d < observation_dimension; ++d)
			probability*=observation_probabilities[state][observations[timestep][d]][d];
	
		return probability;	
	}
	else
	{
// 		cout << "Timestep " << timestep << " prob: " << mixture_model[state].gmmProb(mixture_model[state].arrayToVector(observations[timestep],observation_dimension)) << endl;
		return mixture_model[state].gmmProb(mixture_model[state].arrayToVector(observations[timestep],observation_dimension));
	}
}

//Generally denoted alpha in the literature
//Please note that the literature typically numbers states and observations 1-N, 1-K respectively,
//whereas the programming language starts enumerating at 0
double HMM::forwardProbability(int state, int timestep)
{
	if(timestep == 0)
		return prior_probabilities[state]*observationProbability(state,timestep);
	
	double sum = 0.0;
	for(size_t i = 0; i < number_of_states; ++i)
// 		sum+=forwardProbability(i,timestep-1)*transition_probabilities[i][state];
		sum+=alpha[i][timestep-1]*transition_probabilities[i][state];
		
	return sum*observationProbability(state,timestep);
}

//Generally denoted beta in the literature
double HMM::backwardProbability(int state, int timestep)
{
	if(timestep == observation_sequence_length-1)
		return 1.0;
	
	double sum = 0.0;
	for(size_t j = 0; j < number_of_states; ++j)
// 		sum+=transition_probabilities[state][j]*observationProbability(j,timestep+1)*backwardProbability(j,timestep+1);
		sum+=transition_probabilities[state][j]*observationProbability(j,timestep+1)*beta[j][timestep+1];
	
	return sum;
}

//Generally denoted gamma in the literature
//Probability of being in state at timestep, given the model parameters and observation sequence.
double HMM::stateProbability(int state, int timestep)
{
	double normalisation_constant = 0.0;
	
	for(size_t i = 0; i < number_of_states; ++i)
// 		normalisation_constant+= forwardProbability(i,timestep)*backwardProbability(i,timestep);
		normalisation_constant+= alpha[i][timestep]*beta[i][timestep];	
		
// 	return (forwardProbability(state,timestep)*backwardProbability(state,timestep))/normalisation_constant;
	return (alpha[state][timestep]*beta[state][timestep])/normalisation_constant;
}

//Generally denoted gamma in the literature
//Same functions as the previous, but overloaded for GMM components
double HMM::stateProbability(int state, int timestep, int component)
{
	vector<double> x = mixture_model[state].arrayToVector(observations[timestep],observation_dimension);
	double observation_component_probability = mixture_model[state].gmmProb(x,component);
	double normalisation_constant = mixture_model[state].gmmProb(x);
	
	return gamma[state][timestep]*(observation_component_probability/normalisation_constant);
}

//Generally denoted xi in the literature
//Probability of being in state i at timestep and transfering to state j, given observation sequence and model parameters
//Normalisation constant can probably be replaced by P(O\model) = \sum_{i = 1}^{k} \alpha_{T}(i)
double HMM::stateToStateProbability(int state_i, int state_j, int timestep)
{
	if(timestep == observation_sequence_length-1)
		return 0.0;
	
	double normalisation_constant = 0.0;
	for(size_t k = 0; k < number_of_states; ++k)
		for(size_t l = 0; l < number_of_states; ++l)
// 			normalisation_constant+= forwardProbability(k,timestep)*transition_probabilities[k][l]*observationProbability(l,timestep+1)*backwardProbability(l,timestep+1);
			normalisation_constant+=alpha[k][timestep]*transition_probabilities[k][l]*observationProbability(l,timestep+1)*beta[l][timestep+1];
	
// 	return (forwardProbability(state_i, timestep)*transition_probabilities[state_i][state_j]*observationProbability(state_j,timestep+1)*backwardProbability(state_j,timestep+1))/normalisation_constant;
	return (alpha[state_i][timestep]*transition_probabilities[state_i][state_j]*observationProbability(state_j,timestep+1)*beta[state_j][timestep+1])/normalisation_constant;
}

void HMM::mStep()
{
	maximisePriors();
	maximiseTransitions();
	maximiseObservationDistribution();
}

void HMM::maximisePriors()
{
	for(size_t i = 0; i < number_of_states; ++i)
		prior_probabilities[i] = gamma[i][0];
}

void HMM::maximiseTransitions()
{
	for(map<int, map<int,double> >::iterator i = transition_probabilities.begin(); i != transition_probabilities.end(); ++i)
		for(map<int,double>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			updateTransition(i->first,j->first);
		
// 	double test;
// 	for(size_t i = 0; i < number_of_states; ++i)
// 	{
// 		test = 0.0;
// 		for(size_t j = 0; j < number_of_states; ++j)
// 			test+=transition_probabilities[i][j];
// 		cout << "Test, should be 1: " << test << endl;
// 	}
	printTransitionProbabilities();
}

void HMM::updateTransition(int i, int j)
{
	double numerator = 0.0;
	double denominator = 0.0;
	
	for(size_t t = 0; t < observation_sequence_length-1; ++t)
	{
		numerator+=xi[i][j][t];
		denominator+=gamma[i][t];
	}
	
	transition_probabilities[i][j] = numerator/denominator;
}

void HMM::maximiseObservationDistribution()
{
	vector<double> new_mean;
	vector<vector<double> > new_covariance;
	
	//If we are using a single Gaussian
	if(gaussian == 1)//This may be replaced by the MOG update after testing
	{
		for(size_t i = 0; i < number_of_states; ++i)
		{
			new_mean.clear();
			new_covariance.clear();
			for(size_t d = 0; d < observation_dimension; ++d)
				new_mean.push_back(updateGaussianMean(i,d));
			
			cout << "State " << i << endl;
			cout << "New mean: " << endl;
			mixture_model[i].printMatrix(new_mean);
		
			new_covariance = updateGaussianCovariance(i,new_mean);
			cout << "New covariance: " << endl;
			mixture_model[i].printMatrix(new_covariance);
			mixture_model[i].setMean(new_mean);
			mixture_model[i].setCovariance(new_covariance);
		}	
	}
	else if(gaussian == 2)//if we use a Gaussian mixture model
		for(size_t i = 0; i < number_of_states; ++i)
// 			mixture_model[i].EM(observations);
			updateGMMparameters();
	else//or a discrete observation distribution
		for(size_t i = 0; i < number_of_states; ++i)
			for(size_t m = 0; m < number_of_observations; ++m)
				for(size_t d = 0; d < observation_dimension; ++d)
					updateObservationDistribution(i,m,d);
	
// 	printObservationProbabilities();
}

//Update rule for a single Guassian
double HMM::updateGaussianMean(int state, int dimension)
{
	double numerator = 0.0;
	double denominator = 0.0;
	
	//Update mean of current dimension
	for(size_t t = 0; t < observation_sequence_length; ++t)
	{
		numerator+= gamma[state][t]*observations[t][dimension];
		denominator+= gamma[state][t];
	}
	return numerator/denominator;
}

//Update rule for a Gaussian mixture model
void HMM::updateGMMparameters()
{
	for(size_t i = 0; i < number_of_states; ++i)
	{
		updateGMMweights(i);
		updateGMMmean(i);
		updateGMMcovariance(i);
	}
}

void HMM::updateGMMweights(int state)
{
	double normalisation_constant = 0.0;
	for(size_t t = 0; t < observation_sequence_length; ++t)
		normalisation_constant+=gamma[state][t];
	
	double numerator;
	for(size_t k = 0; k < mixture_model[state].getMixtureComponents(); ++k)
	{
		numerator = 0.0;
		for(size_t t = 0; t < observation_sequence_length; ++t)
			numerator+=gmm_gamma[state][t][k];
		mixture_model[state].setPrior(k,numerator/normalisation_constant);
	}
}

void HMM::updateGMMmean(int state)
{
	double normalisation_constant,numerator;
	vector<double> new_mean;
	
	for(size_t k = 0; k < mixture_model[state].getMixtureComponents(); ++k)
	{
		new_mean.clear();
		for(size_t d = 0; d < observation_dimension; ++d)
		{
			numerator = 0.0;
			normalisation_constant = 0.0;
			for(size_t t = 0; t < observation_sequence_length; ++t)
			{
				numerator+=(gmm_gamma[state][t][k]*observations[state][d]);
				normalisation_constant+=gmm_gamma[state][t][k];
			}
			new_mean.push_back(numerator/normalisation_constant);
		}
		mixture_model[state].setMean(k,new_mean);
	}
}

void HMM::updateGMMcovariance(int state)
{
	double normalisation_constant;
	vector<vector<double > > new_covariance;
	vector<double> difference, obs_vector, mixture_mean;
	for(size_t k = 0; k < mixture_model[state].getMixtureComponents(); ++k)
	{
		new_covariance.clear();
		normalisation_constant = 0.0;
		for(size_t t = 0; t < observation_sequence_length; ++t)
		{
			obs_vector = mixture_model[state].arrayToVector(observations[t],observation_sequence_length);
			mixture_mean = mixture_model[state].getMean(k);
			difference = mixture_model[state].vectorSubtract(obs_vector,mixture_mean);
			new_covariance = mixture_model[state].outerProduct(difference,difference);
			new_covariance = mixture_model[state].vectorScalarProduct(new_covariance,gmm_gamma[state][t][k]);
			normalisation_constant+=gmm_gamma[state][t][k];
		}
		
		new_covariance = mixture_model[state].vectorScalarProduct(new_covariance,1.0/normalisation_constant);
		mixture_model[state].setCovariance(k,new_covariance);
	}
}

//Update covariance matrix of state
//This needs some work, maybe a seperate math class, such that we dont have to address the gmm class for linear algebra functions
vector<vector<double> > HMM::updateGaussianCovariance(int state, vector<double> mean)
{
	vector<double> obs_timestep;
	vector<double> difference;
	obs_timestep.reserve(observation_dimension);
	
	vector<vector<double> > covariance_matrix;
	vector<vector<double> > current_mat;
	
	//Initialise covariance
	difference.assign(observation_dimension,0.0);
	covariance_matrix.assign(observation_dimension,difference);
	difference.clear();
	
	double normalisation_constant = 0.0;
	
	for(size_t t = 0; t < observation_sequence_length; ++t)
	{
		obs_timestep = mixture_model[0].arrayToVector(observations[t],observation_dimension);
		difference = mixture_model[0].vectorSubtract(obs_timestep,mean);
		
		current_mat = mixture_model[0].outerProduct(difference,difference);
		current_mat = mixture_model[0].vectorScalarProduct(current_mat,gamma[state][t]);
		
		covariance_matrix = mixture_model[0].vectorAdd(covariance_matrix,current_mat);
		normalisation_constant+= gamma[state][t];
	}
	
	return mixture_model[0].vectorScalarProduct(covariance_matrix,1.0/normalisation_constant);
}

void HMM::updateObservationDistribution(int state, int observation_index, int dimension)
{
	double numerator = 0.0;
	double denominator = 0.0;
	
	for(size_t t = 0; t < observation_sequence_length; ++t)
	{
		if(observations[t][dimension] == observation_index)
			numerator+=gamma[state][t];
		denominator+=gamma[state][t];
	}
	
	observation_probabilities[state][observation_index][dimension] = numerator/denominator;
}
//End training functions

//Model properties
double HMM::stateSequenceProbability(vector<int> sequence)
{
	double probability = prior_probabilities[sequence[0]];
	for(size_t i = 1; i < sequence.size(); ++i)
		probability*=transition_probabilities[sequence[i-1]][sequence[i]];
	
	return probability;
}

//Returns the probability of the sequence under the given model
double HMM::observationSequenceProbability(double **observation_sequence,int length)
{
	double probability = 0.0;
	observation_sequence_length = length;
	observations = observation_sequence;
	
	for(size_t i = 0; i < number_of_states; ++i)
	{
		probability+=forwardProbability(i,observation_sequence_length-1);
// 		cout << prior_probabilities[i] << endl;
// 		probability+=(prior_probabilities[i]*backwardProbability(i,0));
	}
	
	return probability;
}

//These functions need some work in efficiency and readability
int* HMM::viterbiSequence(double** observation_sequence, int length)
{
	observation_sequence_length = length;
	observations = observation_sequence;
	
	//Declare and initialise dynammic programming table
	//{delta,psi}[states,timesteps]
	double **delta = new double*[number_of_states];
	int **psi = new int*[number_of_states];
	
	for(size_t i = 0; i < number_of_states; ++i)
	{
		delta[i] = new double[observation_sequence_length];
		psi[i] = new int[observation_sequence_length];
	}
	
	for(size_t i = 0; i < number_of_states; ++i)
	{
		delta[i][0] = prior_probabilities[i]*observationProbability(i,0);
		psi[i][0] = 0;
	}
	
	//Compute table
	int index;
	for(size_t t = 1; t < observation_sequence_length; ++t)
	{
		for(size_t i = 0; i < number_of_states; ++i)
		{
			delta[i][t] = highestPathProbability(i, t, delta, index);
			psi[i][t] = index;
		}
	}
	
	//Termination
	double max_probability = 0.0;
	for(size_t i = 0; i < number_of_states; ++i)
		if(delta[i][observation_sequence_length-1] > max_probability)
		{
			max_probability = delta[i][observation_sequence_length-1];
			index = i;
		}
	
	//Perform the backtrack
	int *state_sequence = new int[observation_sequence_length];
	for(size_t t = 0; t < observation_sequence_length-1; ++t)
		state_sequence[t] = 0;
	
	state_sequence[observation_sequence_length-1] = index;
	
	for(size_t t = observation_sequence_length-2; t > 0; --t)
		state_sequence[t] = psi[state_sequence[t+1]][t+1];

	state_sequence[0] = psi[state_sequence[1]][1];
	return state_sequence;
	
}

double HMM::highestPathProbability(int state, int timestep, double **delta, int index)
{
	double *probabilities = new double[number_of_states];
	for(size_t i = 0; i < number_of_states; ++i)
		probabilities[i] = delta[i][timestep-1]*transition_probabilities[i][state];
	
	return maxValue(probabilities,index)*observationProbability(state,timestep);
}

double HMM::maxValue(double* array, int index)
{
	double value = 0.0;
	index = 0;
	for(size_t i = 0; i < number_of_states; ++i)
		if(array[i] > value)
		{
			value = array[i];
			index = i;
			
		}
	return value;
}
//End properties

//Print functions
void HMM::printObservations()
{
	cout << "Training on observations: " << endl;
	for(size_t t = 0; t < observation_sequence_length; ++t)
	{
		cout << "O_" << t << " ";
		for(size_t d = 0; d < observation_dimension; ++d)
			cout << observations[t][d] << " ";
		cout << endl;
	}
}
void HMM::printPriorProbabilities()
{
	cout << "Current prior probabilities: " << endl;
	for(size_t i = 0; i < number_of_states; ++i)
		cout << prior_probabilities[i] << " ";
	cout << endl;
}
void HMM::printTransitionProbabilities()
{
	cout << "Current transition probabilities: " << endl;
	for(map<int, map< int, double> >::iterator i = transition_probabilities.begin(); i != transition_probabilities.end(); ++i)
	{
		for(map<int, double>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
			cout << (*j).second << " ";
		cout << endl;
	}
}

void HMM::printObservationProbabilities()
{
	if(gaussian !=0)
	{
		cout << "No discrete observation model is used" << endl;
	}
	else
	{
		cout << "Current observation probabilities: " << endl;
		for(map<int, map<int, map< int, double> > >::iterator i = observation_probabilities.begin(); i != observation_probabilities.end(); ++i)
		{
			cout << "State " << i->first << endl;
			for(map<int,map<int,double> >::iterator j = i->second.begin(); j != i->second.end(); ++j)
				for(map<int, double>::iterator d = j->second.begin(); d != j->second.end(); ++d)
					cout << d->second << " ";
			cout << endl;
		}
	}
}
//End print functions