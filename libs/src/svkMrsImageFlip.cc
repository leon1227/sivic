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



#include <svkMrsImageFlip.h>
#include <vtkImageFlip.h>


using namespace svk;


vtkCxxRevisionMacro(svkMrsImageFlip, "$Rev$");
vtkStandardNewMacro(svkMrsImageFlip);


/*!
 *  Constructor.  Initialize any member variables. 
 */
svkMrsImageFlip::svkMrsImageFlip()
{

#if VTK_DEBUG_ON
    this->DebugOn();
#endif

    vtkDebugMacro(<<this->GetClassName() << "::" << this->GetClassName() << "()");

    //  Initialize any member variables
    this->filteredAxis = -1; 
}


/*!
 *  Clean up any allocated member variables. 
 */
svkMrsImageFlip::~svkMrsImageFlip()
{
}


/*
 *  Sets the axis to reverse: 0 = cols(x), 1 = rows(y), 2 = slice(z)
 */
void svkMrsImageFlip::SetFilteredAxis( int axis )
{
    this->filteredAxis = axis; 
}


/*! 
 *  This method is called during pipeline execution.  This is where you should implement your algorithm. 
 */
int svkMrsImageFlip::RequestData( vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector )
{

    //  Get pointer to input data set. 
    svkMrsImageData* mrsData = svkMrsImageData::SafeDownCast(this->GetImageDataInput(0)); 

    //  Get pointer to data's meta-data header (DICOM object). 
    svkDcmHeader* hdr = mrsData->GetDcmHeader();  

    int numSpecPts = hdr->GetIntValue( "DataPointColumns" );
    int cols       = hdr->GetIntValue( "Columns" );
    int rows       = hdr->GetIntValue( "Rows" );
    int slices     = hdr->GetNumberOfSlices();
    int numTimePts = hdr->GetNumberOfTimePoints();
    int numCoils   = hdr->GetNumberOfCoils();  

    vtkImageData* tmpData = NULL;
    svkMriImageData* singleFreqImage = svkMriImageData::New();

    vtkImageFlip* flip = vtkImageFlip::New();
    flip->SetFilteredAxis( this->filteredAxis ); 

    for( int timePt = 0; timePt < numTimePts; timePt++ ) {
        for( int coil = 0; coil < numCoils; coil++ ) {
            for( int freq = 0; freq < numSpecPts; freq++ ) {

                mrsData->GetImage( singleFreqImage, freq, timePt, coil);
                singleFreqImage->Modified();

                tmpData = singleFreqImage;

                flip->SetInput( tmpData ); 
                tmpData = flip->GetOutput();
                tmpData->Update();

                mrsData->SetImage( tmpData, freq, timePt, coil);

            }
        }
    }

    flip->Delete(); 

    //  Trigger observer update via modified event:
    this->GetInput()->Modified();
    this->GetInput()->Update();

    return 1; 
} 


/*!
 *  Set the input data type, e.g. svkMrsImageData for an MRS algorithm.
 */
int svkMrsImageFlip::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svkMrsImageData"); 
    return 1;
}

