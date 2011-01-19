//Handwring recognition project, AI MSc, University of Amsterdam
//Thijs Kooi, 2011
//This file is created to wrap the HMM and GMM C++ classes to matlab interface. This way the code can be added to the general pipeline with the advantage of speedy C++ computation and easy Matlab interface and plotting functions.

//This is a first test version and therefore highly subject to change!!

#include "mex.h"
// #include "hmm.h"

// map<string, int> dictionary;
// vector<HMM> word_models;
double** convertObservations(double*,int,int);
int findWord(char* word);

using namespace std;

//Gateway function which is addressed by Matlab and redirects the input to C++
//The function should take observations from a sliding window and either train a given HMM
//or take observations from a sliding window and return the most probable word
//The inputs will be a matrix of observations, where every row corresponts to a single observation and every column a feature
//The second argument is a string of the word we are training on, or empty, if we want to get the most likely word.
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	double *input_observations;
	char* train_word;
	mwSize number_of_observations,observation_dimension;

	//We need one or two input arguments, as stated above
	if(nrhs == 0 || nrhs > 2) { mexErrMsgTxt("One or two input arguments required.");  }
	//and at most one output argument
	else if(nlhs > 1) { mexErrMsgTxt("Too many output arguments."); }
	
	//prhs[0] contains a pointer to the first right hand side argument
	//We want the first right hand side argument to be a matrix of observations,
	//mexErrMsgTxt automatically breaks, therefore, we do not need an else statement
	if (!(mxIsDouble(prhs[0]))) { mexErrMsgTxt("Input array must be of type double."); }
	//The second argument will be either be 0 or a string
	if(nrhs == 2)
		if ( mxIsChar(prhs[1]) != 1) { mexErrMsgTxt("Input must be a string."); } 
	
	input_observations = mxGetPr(prhs[0]);
	
	//Get the number of observations and the observation dimension
	number_of_observations = mxGetM(prhs[0]);
	observation_dimension = mxGetN(prhs[0]);
	
	//If we are training
	if(nrhs == 2)
	{
		//Convert the input string to a c string
		train_word = mxArrayToString(prhs[0]);
		
		//Convert observations to multidimensional array
		double** observations = convertObservations(input_observations,number_of_observations,observation_dimension);
		
		//Look for the word in the dictionary, if it doesn't exist, add it and create an HMM at the appropriate place in the dictionary, else give the entry
		//int entry_number = findWord(train_word);
		
		//Train the Markov model for the given observation
		//word_models[entry_number].trainModel(train_word);
	}
// 	else
// 	{
// 		//Check literature.. to do
// 	}
}

//Convert the input array to a multidimensional array, for easier handling by the HMM
double** convertObservations(double* input, int m, int n)
{
	double **out = new double*[m];
	for(size_t i = 0; i < m; ++i)
		out[i] = new double[n];
	
	int count = 0;
	
	for(size_t i = 0; i < m; ++i)
		for(size_t j = 0; j < n; ++j)
		{
			out[i][j] = input[count];
			++count;
		}
}

