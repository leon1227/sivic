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

#ifndef SVK_2_SITE_EXCHANGE_COST_COST_FUNCTION_H
#define SVK_2_SITE_EXCHANGE_COST_COST_FUNCTION_H

#include <svkKineticModelCostFunction.h>


using namespace svk;


/*
 *  Cost function for ITK optimizer: 
 */
class svk2SiteExchangeCostFunction : public svkKineticModelCostFunction
{

    public:

        typedef svk2SiteExchangeCostFunction            Self;
        itkNewMacro( Self );


        svk2SiteExchangeCostFunction() {
        }


        /*!
         *  Cost function based on minimizing the residual of fitted and observed dynamics. 
         */
        virtual MeasureType  GetResidual( const ParametersType& parameters) const
        {
            cout << "GUESS: " << parameters[0] << " " << parameters[1] << endl;;

            this->GetKineticModel( parameters,
                                    this->kineticModel0,
                                    this->kineticModel1,
                                    this->kineticModel2,
                                    this->signal0,
                                    this->signal1,
                                    this->signal2,
                                    this->numTimePoints );

            double residual = 0;

            int arrivalTime = 2;
            for ( int t = arrivalTime; t < this->numTimePoints; t++ ) {
                residual += ( this->signal0[t] - this->kineticModel0[t] )  * ( this->signal0[t] - this->kineticModel0[t] );
                //residual += ( this->signal1[t] - this->kineticModel1[t] )  * ( this->signal1[t] - this->kineticModel1[t] );
                //residual += ( this->signal2[t] - this->kineticModel2[t] )  * ( this->signal2[t] - this->kineticModel2[t] );
            }
            cout << "RESIDUAL: " << residual << endl;

            MeasureType measure = residual ;

            return measure;
        }


        virtual void GetKineticModel( const ParametersType& parameters,
                    float* kineticModel0,
                    float* kineticModel1,
                    float* kineticModel2,
                    float* signal0,
                    float* signal1,
                    float* signal2,
                    int numTimePoints
        ) const 
        {

            double T1all  = parameters[0];
            double Kpl    = parameters[1];
            int arrivalTime = 2;

            //  use model params and initial signal intensity to calculate the metabolite signals vs time 
            //  solved met(t) = met(0)*invlaplace(phi(t)), where phi(t) = sI - x. x is the matrix of parameters.
            for ( int t = 0; t < numTimePoints; t++ ) {

                if (t < arrivalTime ) {
                    kineticModel0[t] = 0;
                    kineticModel1[t] = 0;
                    kineticModel2[t] = 0;
                }
                if (t >= arrivalTime ) {
                    // PYRUVATE 
                    kineticModel0[t] = signal0[arrivalTime] * exp( -((1/T1all) + Kpl) * (t - arrivalTime) );

                    // LACTATE 
                    kineticModel1[t] = signal0[ arrivalTime ] * (-exp( -t/T1all - (t - arrivalTime)*Kpl ) + exp(-(t - arrivalTime)/T1all))
                                           + signal1[ arrivalTime ] * exp(-(t -arrivalTime)/T1all);
                    // UREA
                    kineticModel2[t] = signal2[arrivalTime] * exp( -(1/T1all) * (t - arrivalTime));
                }
            }
        }


        virtual unsigned int GetNumberOfParameters(void) const
        {
            int numParameters = 2;
            return numParameters;
        }

};



#endif// SVK_2_SITE_EXCHANGE_COST_COST_FUNCTION_H
