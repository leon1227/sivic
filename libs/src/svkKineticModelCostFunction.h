/*
 *  Copyright © 2009-2014 The Regents of the University of California.
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *  •   Redistributions of source code must retain the above copyright notice, 
 *      this list of conditions and the following disclaimer.
 *  •   Redistributions in binary form must reproduce the above copyright notice, 
 *      this list of conditions and the following disclaimer in the documentation 
 *      and/or other materials provided with the distribution.
 *  •   None of the names of any campus of the University of California, the name 
 *      "The Regents of the University of California," or the names of any of its 
 *      contributors may be used to endorse or promote products derived from this 
 *      software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 *  OF SUCH DAMAGE.
 */



/*
 *  $URL$
 *  $Rev$
 *  $Author$
 *  $Date$
 *
 *  Authors:
 *      Jason C. Crane, Ph.D.
 *      Beck Olson,
 */

#ifndef SVK_KINETIC_MODEL_COST_FUNCTION_H
#define SVK_KINETIC_MODEL_COST_FUNCTION_H

#include <itkParticleSwarmOptimizer.h>
using namespace svk;


/*
 *  Cost function for ITK optimizer: 
 *  Abstract base class for kinetic modeling cost functions.  
 */
class svkKineticModelCostFunction : public itk::SingleValuedCostFunction 
{

    public:

        typedef svkKineticModelCostFunction             Self;
        typedef itk::SingleValuedCostFunction           Superclass;
        typedef itk::SmartPointer<Self>                 Pointer;
        typedef itk::SmartPointer<const Self>           ConstPointer;
        itkTypeMacro( svkKineticModelCostFunction, SingleValuedCostFunction );

        typedef Superclass::ParametersType              ParametersType;
        typedef Superclass::DerivativeType              DerivativeType;
        typedef Superclass::MeasureType                 MeasureType;


        /*
         *
         */
        svkKineticModelCostFunction()
        {
        }


        /*
         *
         */
        ~svkKineticModelCostFunction()
        {
        }


        /*
         *
         */
        void GetDerivative( const ParametersType & ,
                            DerivativeType &  ) const
        {
        }


        /*!
         *  Cost function based on minimizing the residual of fitted and observed dynamics. 
         */
        virtual MeasureType  GetResidual( const ParametersType& parameters) const = 0; 


        /*!
         *  For a given set of parameter values, compute the model kinetics:
         *
         *  Function to calculate metabolite signal vs time based on parametric model.  
         *      uses the following : 
         *      parameters    = kinetic model parameters to be fit (1/T1all, Kpl).  
         *      kineticModelN = model metabolite signal intensity vs time ( pyr, lac, urea)
         *      signalN       = input signal vs time for each of 3 measured metabolites (pyr, lac, urea) 
         *      numTimePoints = number of observed time points.  
         */
        virtual void GetKineticModel( const ParametersType& parameters,
                    float* kineticModel0,
                    float* kineticModel1,
                    float* kineticModel2
        ) const = 0; 


        /*!  
         *  returns the cost function for the current param values: 
         *  typedef double MeasureType
         */
        MeasureType  GetValue( const ParametersType & parameters ) const
        {
            double cost = GetResidual( parameters );
            MeasureType measure = cost;
            return measure;
        }


        /*
         *
         */
        virtual unsigned int GetNumberOfParameters(void) const = 0; 


        /*
         *  Set the kinetic signal for metabolite N
         */
        void SetNumSignals( int numSignals )
        {
            this->numSignals = numSignals; 
            this->signalVector.resize(numSignals); 
        }


        /*
         *  Set the kinetic signal for metabolite N
         */
        void SetSignal( float* signal, int signalIndex, string signalName)
        {
            map<string, float*>  signalMap;  
            signalMap[signalName] = signal; 
            if (signalIndex >  this->signalVector.size() ) {
                cout << "ERROR, signal index is out of range" << endl; 
                exit(1); 
            }
            this->signalVector[signalIndex] = signalMap;
        }


        /*
         *  Get pointer to the vector of input kinetic signals
         */
        vector < map < string, float* > >* GetSignals( )
        {
            return &(this->signalVector);
        }


        /*
         *  Get the specific signal at the specified time point.
         */
        float GetSignalAtTime(int signalIndex, int timePt) const
        {
            //map< string, float*>::iterator mapIter;
            return  (this->signalVector[ signalIndex ].begin()->second)[timePt]; 
        }

        /*
         *  Get the specific signal 
         */
        float* GetSignal(int signalIndex) const
        {
            return  (this->signalVector[ signalIndex ]).begin()->second; 
        }


        /*
         *  Initializes the arrays that will hold the fitted data
         *  with the correct number of points. 
         */
        void SetNumTimePoints( int numTimePoints )
        {
            this->numTimePoints = numTimePoints;
            this->kineticModel0 = new float [this->numTimePoints];
            this->kineticModel1 = new float [this->numTimePoints];
            this->kineticModel2 = new float [this->numTimePoints];
        }


    protected:

        //  this is the vector of float arrays representing the metabolite signals as a function of time
        //  for different number of sites this should be flexibile
        vector< map < string, float* > >  signalVector;
        float*      kineticModel0;
        float*      kineticModel1;
        float*      kineticModel2;
        int         numTimePoints;
        int         numSignals;

};



#endif// SVK_KINETIC_MODEL_COST_FUNCTION_H
