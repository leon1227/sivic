/*
 *  Copyright © 2009-2011 The Regents of the University of California.
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
 *      Beck Olson
 */


#include <svkApodizationWindow.h>
#include <vtkGlobFileNames.h>

using namespace svk;

vtkCxxRevisionMacro(svkApodizationWindow, "$Rev$");
vtkStandardNewMacro(svkApodizationWindow);

//! Constructor
svkApodizationWindow::svkApodizationWindow()
{
}


//! Destructor
svkApodizationWindow::~svkApodizationWindow()
{
}


/*!
 *  Creates a lorentzian window using the equation:
 *  f(t) = e^(-fwhh * PI * dt) from t = 0 to t = N where N the number of tuples in the input array.
 *
 *  \param window    Pre-allocated array that will be populated with the window. 
 *                   The number of tuples allocated determines the number of points in the window.
 *
 *  \param fwhh      Defines the shape of the Lorentzian, also know as the line broadening parameter. 
 *                   Value in Hz. 
 *
 *  \param dt        Temporal resolution of the window in seconds.
 *
 */
void svkApodizationWindow::GetLorentzianWindow( vtkFloatArray* window,  float fwhh, float dt )
{
    if( window != NULL ) {

        int numPoints = window->GetNumberOfTuples();

        for( int i = 0; i < numPoints; i++ ) {
            // NOTE: fabs is used here in case we want to alter the center of the window in the future.
            float value = exp( -fwhh * vtkMath::Pi()* fabs( dt * i ) );
            window->SetTuple2( i, value, value ); 
        }
    }
}



/*!
 *  Creates a lorentzian window using the equation:
 *  f(t) = e^(-fwhh * PI * dt) from t = 0 to t = N where N the number of tuples in the input array.

 *  \param window    Array that will be populated with the window. 
 *                   The number of tuples allocated determines the number of points in the window.
 *
 *  \param data      The data set to generate the window for. Based on the type the number of points
 *                   and dt are determined. 
 *
 *  \param fwhh      Defines the shape of the Lorentzian, also know as the line broadening parameter. 
 *                   Value in Hz. 
 *
 *  \param dt        Temporal resolution of the window in seconds.
 *
 *  \param numPoints The number of points in the window.
 *
 */
void svkApodizationWindow::GetLorentzianWindow( vtkFloatArray* window, svkImageData* data, float fwhh )
{
    if( data->IsA("svkMrsImageData") && window != NULL ) {

        // Lets determine the number of points in our array 
        int numPoints       = data->GetDcmHeader()->GetIntValue( "DataPointColumns" );

        // Lets set the number of components and the number of tuples
        window->SetNumberOfComponents ( 2 );
        window->SetNumberOfTuples( numPoints );

        // Lets determine the point resolution for the window
        float spectralWidth = data->GetDcmHeader()->GetFloatValue( "SpectralWidth" );
        float dt = 1.0/spectralWidth;
        svkApodizationWindow::GetLorentzianWindow( window, fwhh, dt );
    } else {
         vtkErrorWithObjectMacro(data, "Could not generate Lorentzian window for give data type!");
    }
}
