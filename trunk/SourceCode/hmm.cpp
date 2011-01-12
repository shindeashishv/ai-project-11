// Hidden Markov model class
// Hand writing recognition Januari project, MSc AI, University of Amsterdam
// Thijs Kooi, 2011

// The Baum-Welch algorithm, an instance of the EM-algorithm, is used to train the output probabilities
// To output the most likely sequence, the Viterbi algorithm, a dynamic programming method, is applied.

#include "hmm.h"

// To do:
// - Do hand calculations on forward/backward probability and probability of observation sequences, for different models (2).
// - Do hand calculations for viterbi sequence
// - Make two clear example models, clear manual
// - check for possible underflow forward/backward probability, normalise/log probabilities

int main()
{
	//Testing
	HMM model;
	model.setStates(4);
	model.setNumberOfObservations(4);
	model.initialiseUniform();
	
	cout << "Observations: " << model.getNumberOfObservations() << endl;
	cout << "State: " << model.getStates() << endl;
	
	//Forward model initial probabilities
	double A[4][4] = 	{
				{0.5,0.5,0.0,0.0},
				{0.0,0.5,0.5,0.0},
				{0.0,0.0,0.5,0.5},
				{0.0,0.0,0.0,1.0},
				};
				
	//Test state sequence, fully connected model
// 	double test_sequence[6] = {0,3,2,1,2,2};
	//Test state sequence, forward model
// 	double test_sequence[6] = {0,1,1,2,2,3};

	//Test observation sequence
	int test_sequence[6] = {0,1,1,2,2,3};

	model.printPriorProbabilities();
	model.printTransitionProbabilities();
	model.printObservationProbabilities();
	
	cout << "Observation sequence probability: " << model.observationSequenceProbability(test_sequence,6) << endl;
	int *seq = model.viterbiSequence(test_sequence,6);
	for(int i = 0; i < 6; ++i)
		cout << seq[i] << " ";
	cout << endl;
// 	double observations[] = {2,3,4,5,3,1,4};
// 	model.trainModel(observations);
// 	model.printObservations();
}

//Constructors
HMM::HMM() { }

HMM::HMM(int ns, int no, map<int, map<int, double> > t, map<int, map<int, double> > o)
{
	number_of_states = ns;
	number_of_observations = no;
	transition_probabilities = t;
	observation_probabilities = o;
}
//End constructors

//Getters and setters
int HMM::getStates(){ return number_of_states;  }
void HMM::setStates(int s){ number_of_states = s;  }

int HMM::getNumberOfObservations(){ return number_of_observations; }
void HMM::setNumberOfObservations(int o){ number_of_observations = o; }
//End getters and setters

//Training functions
void HMM::trainModel(int* o)
{
	observations = o;
	cout << "Training model..." << endl;
	int converged=0;
	initialiseParameters();
	while(!converged)
	{
		eStep();
		mStep();
		converged =1;
	}
	cout << "Done!" << endl;
}

void HMM::initialiseUniform() { initialiseParameters(); }

void HMM::initialiseParameters()
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
	
	//Initialise uniform probabilities for observations
	for(size_t i = 0; i < number_of_states; ++i)
		for(size_t j = 0; j < number_of_observations; ++j)
			observation_probabilities[i][j] = 1.0;
		
	for(map<int, map<int,double> >::iterator i = observation_probabilities.begin(); i != observation_probabilities.end(); ++i)
		for(map<int, double>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
			(*j).second/=(*i).second.size();
	
}

//To verify
void HMM::eStep() { current_likelihood = forwardProbability(number_of_states,number_of_observations); }

//Generally denoted alpha in the literature
//Please note that the literature typically numbers states and observations 1-N, 1-K respectively,
//whereas the programming language starts enumerating at 0
double HMM::forwardProbability(int state, int timestep)
{
	if(state == 0 && timestep == 0)
		return 1.0;
	else if(timestep == 0)
		return prior_probabilities[timestep]*observation_probabilities[state][observations[0]];
	
	double sum = 0.0;
	for(size_t i = 0; i < number_of_states; ++i)
		sum+=forwardProbability(i,timestep-1)*transition_probabilities[i][state];
		
	return sum*observation_probabilities[state][observations[timestep]];
}

//Generally denoted beta in the literature
double HMM::backwardProbability(int state, int timestep)
{
	if(timestep == observation_sequence_length-1)
		return 1.0;
	
	double sum = 0.0;
	for(size_t j = 0; j < number_of_states; ++j)
		sum+=transition_probabilities[state][j]*observation_probabilities[j][observations[timestep+1]]*backwardProbability(j,timestep+1);
	
	return sum;
}

void HMM::mStep()
{
	maximisePriors();
	maximiseTransitions();
	maximiseObservationDistribution();
}

void HMM::maximisePriors()
{
}

void HMM::maximiseTransitions()
{
	for(map<int, map<int,double> >::iterator i = transition_probabilities.begin(); i != transition_probabilities.end(); ++i)
		for(map<int,double>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			updateTransition(i->first,j->first);
}

void HMM::updateTransition(int i, int j)
{
	double numerator,denominator,sum;
	
	sum = 0.0;
	for(size_t t = 0; t < number_of_observations-1; ++t)
		sum += (forwardProbability(i,t)*transition_probabilities[i][j]*observation_probabilities[j][observations[t+1]]*backwardProbability(j,t+1));
	numerator = (1.0/current_likelihood)*sum;
		
	sum = 0.0;
	for(size_t t = 0; t < number_of_observations-1; ++t)
		sum+=(forwardProbability(i,t)*backwardProbability(i,t));
	denominator = (1.0/current_likelihood)*sum;
	
	transition_probabilities[i][j] = numerator/denominator;
}

void HMM::maximiseObservationDistribution()
{
}
//End training functions

//Model properties
//Returns the likelihood of the observation sequence
double HMM::likelihood() { return current_likelihood; }
double HMM::stateSequenceProbability(vector<int> sequence)
{
	double probability = prior_probabilities[sequence[0]];
	for(size_t i = 1; i < sequence.size(); ++i)
		probability*=transition_probabilities[sequence[i-1]][sequence[i]];
	
	return probability;
}

//Returns the probability of the sequence under the given model
double HMM::observationSequenceProbability(int *sequence,int length)
{
	double probability = 0.0;
	observation_sequence_length = length;
	observations = sequence;
	
	for(size_t i = 0; i < number_of_states; ++i)
	{
		cout << i << " " << forwardProbability(i,observation_sequence_length-1) << endl;
		probability+=forwardProbability(i,observation_sequence_length-1);
	}
	
	return probability;
}

//These functions need some work in efficiency and readability
int* HMM::viterbiSequence(int* observation_sequence, int l)
{
	observation_sequence_length = l;
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
		delta[i][0] = prior_probabilities[i]*observation_probabilities[i][observations[0]];
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
	
	//Perform the back track
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
	
	return maxValue(probabilities,index)*observation_probabilities[state][observations[timestep]];
}

double HMM::maxValue(double* array, int index)
{
// 	for(int i = 0; i < number_of_states; ++i)
// 		cout << array[i] << endl;
// 	
	double value = 0.0;
	index = 0;
	for(size_t i = 0; i < number_of_states; ++i)
		if(array[i] > value)
		{
			value = array[i];
			index = i;
			
		}
	
// 	cout << index << endl;
	return value;
}
//End properties




//Testing and debugging
void HMM::printObservations()
{
	cout << "Training on observations: " << endl;
	for(size_t i = 0; i < number_of_observations; ++i)
		cout << observations[i] << " ";
	cout << endl;
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
	cout << "Current observation probabilities: " << endl;
	for(map<int, map< int, double> >::iterator i = observation_probabilities.begin(); i != observation_probabilities.end(); ++i)
	{
		for(map<int, double>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
			cout << (*j).second << " ";
		cout << endl;
	}
}
//End testing and debugging