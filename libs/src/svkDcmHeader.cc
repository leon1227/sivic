/*
 *  Copyright © 2009-2012 The Regents of the University of California.
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



#include <svkDcmHeader.h>


using namespace svk;


vtkCxxRevisionMacro(svkDcmHeader, "$Rev$");


const float svkDcmHeader::UNKNOWN_TIME = -1;
const vtkstd::string svkDcmHeader::UNKNOWN_STRING = "UNKNOWN";



/*!
 *
 */
svkDcmHeader::svkDcmHeader()
{

#if VTK_DEBUG_ON
    this->DebugOn();
#endif
    vtkDebugMacro(<<this->GetClassName()<<":svkDcmHeader");

    this->lastUpdateTime = this->GetMTime(); 
    this->dataSliceOrder = SLICE_ORDER_UNDEFINED;
    this->numTimePts = 1;
}


/*!
 *
 */
svkDcmHeader::~svkDcmHeader()
{
    vtkDebugMacro(<<this->GetClassName()<<"~svkDcmHeader");
}


/*!
 *  Converts a string representation of a patient name to a DICOM VR = PN representation. 
 *      "[lastName[^firstName[^middleName[^namePrefix[^nameSuffix]]]]"
 *
 *      \param PatientName space delimited patient name string 
 */
vtkstd::string svkDcmHeader::GetDcmPatientName(vtkstd::string PatientName)
{

    size_t delim;

    const string dcmPNDelim("^");

    // There should be no more than 5 elements to the name (4 delims):
    // replace spaces with ^, or add ^ between elements not present
    for (int i = 0; i < 4; i++) {
        if ( (delim = PatientName.find_first_of(' ') ) != string::npos ) {
            PatientName.replace( delim, 1, dcmPNDelim);
        } else {
            PatientName.push_back('^');
        }
    }
    return PatientName;
}


/*!
 *  Converts a string representation of a patient name to a DICOM VR = PN representation
 *  and sets the value in the DICOM header: 
 *      "[lastName[^firstName[^middleName[^namePrefix[^nameSuffix]]]]"
 *
 *      \param PatientName space delimited patient name string 
 */
void svkDcmHeader::SetDcmPatientName(vtkstd::string PatientName)
{

    vtkstd::string dcmPatientName = this->GetDcmPatientName( PatientName ); 

    this->SetValue(
        "PatientName",
        dcmPatientName
    );

}


/*!
 *  Set the format that specifies the word size and representation in the DICOM PixelData.
 *  Note that this represents the in-memory pixel representation and supports 8, 16 and 32 bit
 *  char, int and float representations.  When writing to DICOM MRI only 8 and 16 bit ints are 
 *  supported, so data may require scaling (see DICOM RescaleIntercept and RescaleSlope attributes).   
 */
void svkDcmHeader::SetPixelDataType(DcmPixelDataFormat dataType)
{

    if ( dataType == svkDcmHeader::UNSIGNED_INT_1 ) {
        this->SetValue( "BitsAllocated", 8 );
        this->SetValue( "BitsStored", 8 );
        this->SetValue( "HighBit", 7 );
        this->SetValue( "PixelRepresentation", 0 ); //unsigned
    } else if ( dataType == svkDcmHeader::UNSIGNED_INT_2 ) {
        this->SetValue( "BitsAllocated", 16 );
        this->SetValue( "BitsStored", 16 );
        this->SetValue( "HighBit", 15 );
        this->SetValue( "PixelRepresentation", 0 ); //unsigned
    } else if ( dataType == svkDcmHeader::SIGNED_INT_2 ) {
        this->SetValue( "BitsAllocated", 16 );
        this->SetValue( "BitsStored", 16 );
        this->SetValue( "HighBit", 15 );
        this->SetValue( "PixelRepresentation", 1 ); //signed
    } else if ( dataType == svkDcmHeader::SIGNED_FLOAT_4 ) {
        this->SetValue( "BitsAllocated", 32 );
        this->SetValue( "BitsStored", 32 );
        this->SetValue( "HighBit", 31 );
        this->SetValue( "PixelRepresentation", 1 ); //signed
    } else if ( dataType == svkDcmHeader::SIGNED_FLOAT_8 ) {
        this->SetValue( "BitsAllocated", 64 );
        this->SetValue( "BitsStored", 64 );
        this->SetValue( "HighBit", 63 );
        this->SetValue( "PixelRepresentation", 1 ); //signed
    } else {
        throw runtime_error("Unsupported data type representation.");
    }
}


/*!
 *  Get the format that specifies the word size and representation in the DICOM PixelData.
 *  4Byte representations are ambiguous and can be float or int, thus the vtkDataType arg.   
 */
int svkDcmHeader::GetPixelDataType( int vtkDataType )
{
    int bitsPerWord = this->GetIntValue( "BitsAllocated" );
    int pixRep = this->GetIntValue( "PixelRepresentation");
    if ( bitsPerWord == 8 && pixRep == 0 ) {
        return svkDcmHeader::UNSIGNED_INT_1;
    } else if ( bitsPerWord == 16 && pixRep == 0 ) {
        return svkDcmHeader::UNSIGNED_INT_2;
    } else if ( bitsPerWord == 16 && pixRep == 1 ) {
        return svkDcmHeader::SIGNED_INT_2;
    } else if ( bitsPerWord == 32 && pixRep == 1 && vtkDataType == VTK_FLOAT ) {
        return svkDcmHeader::SIGNED_FLOAT_4;
    } else if ( bitsPerWord == 64 && pixRep == 1 && vtkDataType == VTK_DOUBLE ) {
        return svkDcmHeader::SIGNED_FLOAT_8;
    } else {
        cout << "Unknown Pixel Data Type " << endl;
        return -1;
    }
}


/*!
 *  Get the origin of the data set. 
 */
int svkDcmHeader::GetOrigin(double origin[3], int sliceNumber)
{

    int status = 0; 

    if ( sliceNumber == 0 && this->WasModified() ) {
        this->UpdateSpatialParams();
    }

    if (sliceNumber == 0) {

        origin[0] = this->origin0[0]; 
        origin[1] = this->origin0[1]; 
        origin[2] = this->origin0[2]; 

    } else {

        if ( this->ElementExists( "ImagePositionPatient", "PlanePositionSequence") == true ) {

            istringstream* iss = new istringstream();
            string originString; 

            //  Iterate of all 3 positions of the ImagePositionPatient element;
            for (int i = 0; i < 3; i++) {
                originString = this->GetStringSequenceItemElement(
                    "PlanePositionSequence",
                    0,
                    "ImagePositionPatient",
                    i, 
                    "PerFrameFunctionalGroupsSequence",
                    sliceNumber
                );
                iss->str(originString);
                *iss >> origin[i];
                iss->clear();
            }
            delete iss;

        } else {

            if (this->GetDebug()) {
                vtkWarningWithObjectMacro(
                    this, 
                    "::GetOrigin(): ImagePositionPatient not defined, can't return origin." 
                );    
            }
            status = 1; 
        }
    }

    //cout << "Origin: " << origin[0] << " " << origin[1] << " " << origin[2] << endl;
    return status; 
}


/*!
 *  Get the size of the voxels.  
 */
void svkDcmHeader::GetPixelSize(double size[3])
{

    if ( this->WasModified() ) {
        this->UpdateSpatialParams();
    }

    size[0] = this->pixelSize[0]; 
    size[1] = this->pixelSize[1]; 
    size[2] = this->pixelSize[2]; 
}


/*!
 *  Get the spacing of the voxels.  This accounts for any gaps between samples.  It is the actial
 *  spacing between samples.  May differ from pixel(sample) size(thickness).
 */
void svkDcmHeader::GetPixelSpacing(double spacing[3])
{
    if ( this->WasModified() ) {
        this->UpdateSpatialParams();
    }

    spacing[0] = this->pixelSpacing[0]; 
    spacing[1] = this->pixelSpacing[1]; 
    spacing[2] = this->pixelSpacing[2]; 

}


/*!
 *  Get the DICOM orientation vectors (in plane vector in direction of row and column data) 
 *  from the DICOM header. 
 */
void svkDcmHeader::GetOrientation(double orientation[2][3])
{

    if ( this->WasModified() ) {
        this->UpdateSpatialParams();
    }

    orientation[0][0] = this->orientation[0][0]; 
    orientation[0][1] = this->orientation[0][1]; 
    orientation[0][2] = this->orientation[0][2]; 
    orientation[1][0] = this->orientation[1][0]; 
    orientation[1][1] = this->orientation[1][1]; 
    orientation[1][2] = this->orientation[1][2]; 

    //cout << "orientation: " << orientation[0][0] << " " << orientation[0][1] << " " << orientation[0][2] << endl;
    //cout << "orientation: " << orientation[1][0] << " " << orientation[1][1] << " " << orientation[1][2] << endl;
}



/*!
 *
 *  This method is used for determining the index of the dcos matrix that corresponds to the 
 *  normal vector of a given user defined slice orientation in LPS coordinates.
 *
 *
 * Each row of the dcos represents the COLUMN, ROW, or SLICE dimension of the data
 * as defined by the data ordering.
 *
 * We are going to check them each sequentially to make sure that if two rows have the same
 * magnitude in a given direction they are assigned to different slice view orientations.
 *
 * EXAMPLE 1----------------------------------------------------------------------
 *
 * 
 *
 * For example if we have the dcos:
 *
 *                       LR        AP        SI
 *
 *     COLUMN INDEX       1         0         0 
 *     ROW INDEX          0         1         0 
 *     SLICE INDEX        0         0         1 
 *
 * If user requests the index of a coronal view of the data:
 *
 *    int coronalIndex = GetOrientationIndex( svkDcmHeader::CORONAL )
 *
 * First we will find the AXIAL direction. This is defined as the greatest absolute value
 * in the SI column. It is clear that this is the SLICE INDEX. Second we search for
 * the CORONAL direction-- this is the greatest absolute value in the AP column and 
 * is clearly the COLUMN INDEX. Lastly we look for the SAGITTAL directions which is
 * again clear and is the ROW INDEX
 *     Result:
 *         AXIAL    index = 2 <-- This is the data's orientation type
 *         CORONAL  index = 1
 *         SAGITTAL index = 0
 *
 * And the method would return the value 1, for CORONAL 
 *
 *    GetOrientationIndex( svkDcmHeader::CORONAL ) = 1;
 *
 * EXAMPLE 2----------------------------------------------------------------------
 *
 * For Example if we have the dcos:
 *
 *                       LR        AP        SI
 *
 *     COLUMN INDEX       0         1         0 
 *     ROW INDEX          1         0         0 
 *     SLICE INDEX        0         0         1 
 *
 * If user requests the index of a sagittal view of the data:
 *
 *    int axialIndex = GetOrientationIndex( svkDcmHeader::AXIAL )
 *
 * First we will find the AXIAL direction. This is defined as the greatest absolute value
 * in the SI column. It is clear that this is the SLICE INDEX. Second we search for
 * the CORONAL direction-- this is the greatest absolute value in the AP column and 
 * is clearly the ROW INDEX. Lastly we look for the SAGITTAL directions which is
 * again clear and is the COLUMN INDEX
 *
 *     Result:
 *         AXIAL    index = 2 <-- This is the data's orientation type
 *         CORONAL  index = 0
 *         SAGITTAL index = 1
 *
 * And the method would return the value 2, for AXIAL 
 *
 *    GetOrientationIndex( svkDcmHeader::AXIAL ) = 2;
 *
 *
 * EXAMPLE 3----------------------------------------------------------------------
 *
 * For Example if we have the dcos:
 *
 *                        LR        AP        SI
 *
 *     COLUMN INDEX    0.7071    0.5000    0.5000
 *     ROW INDEX            0    0.7071   -0.7071
 *     SLICE INDEX    -0.7071    0.5000    0.5000
 *
 * If user requests the index of a sagittal view of the data:
 *
 *    int coronalIndex = GetOrientationIndex( svkDcmHeader::CORONAL )
 *
 * This case is of an image rotated 45 degrees in two dimensions. In this scenario
 * the data is so oblique that defining AXIAL, SAGITTAL, and CORONAL is somewhat
 * arbitrary.
 *
 * First we will find the AXIAL direction. This is defined as the greatest absolute value
 * in the SI column. In this case we would take the ROW INDEX and declare that the
 * AXIAL direction. Next we would search for the CORONAL direction. The highest component
 * in the AP direction is also the COLUMN INDEX, but we have already defined that
 * as the AXIAL direction so we take the next highest which is the same for both
 * the COLUMN and SLICE INDEX. In this case it is arbitrary, the method searches in the
 * order ROW, COLUMN, SLICE and searches for values greater than or equal to so it would 
 * take the SLICE INDEX. Since the only INDEX left is the COLUMN INDEX that will be our 
 * SAGITTAL direction.
 *
 *     Result:
 *         AXIAL    index = 1 
 *         CORONAL  index = 2 <-- This is the data's orientation type
 *         SAGITTAL index = 0
 *
 * And the method would return the value 0, for CORONAL 
 *
 *    GetOrientationIndex( svkDcmHeader::CORONAL ) = 0;
 *
 *  \param orientation the view orientation you want to get the index of 
 *  \return the index, 0 is COLUMN, 1 is ROW, 2 is SLICE as defined by the data ordering.
 */
int svkDcmHeader::GetOrientationIndex( svkDcmHeader::Orientation orientation )
{

    // First we need our dcos
    double dcos[3][3];
    this->GetDataDcos(dcos);

    // We need to map each row of the dcos to be AXIAL, SAGITTAL, or CORONAL....
    map<int,int> primaryComponents;

    // Lets initialize with negative values
    primaryComponents[svkDcmHeader::AXIAL]    = -1;
    primaryComponents[svkDcmHeader::CORONAL]  = -1;
    primaryComponents[svkDcmHeader::SAGITTAL] = -1;

    // Lets start by determining the AXIAL direction...
    double maxValue = 0;
    int maxIndex = 0;
    for( int i = 0; i < 3; i++ ) {
        int SIIndex = 2; // 2 is the SI direction which corresponds to AXIAL 
        double value = fabs(dcos[i][SIIndex]); 
        if( value >= maxValue ) {
            maxValue = value;
            maxIndex = i;
        }
    }
    primaryComponents[svkDcmHeader::AXIAL] = maxIndex;

    // Now lets determin the coronal direction...
    maxValue = 0;
    maxIndex = 0;
    for( int i = 0; i < 3; i++ ) {
        if( i == primaryComponents[svkDcmHeader::AXIAL] ) {
            continue; // If this row has already been assigned to AXIAL, do not consider it for coronal
        }
        int PAIndex = 1; // 1 is the PA direction  which corresponds to CORONAL
        double value = fabs(dcos[i][PAIndex]); 
        if( value >= maxValue ) {
            maxValue = value;
            maxIndex = i;
        }
    }
    primaryComponents[svkDcmHeader::CORONAL] = maxIndex;

    // Now lets determin the sagittal direction...
    maxValue = 0;
    maxIndex = 0;
    for( int i = 0; i < 3; i++ ) {
        if( i == primaryComponents[svkDcmHeader::AXIAL] || i == primaryComponents[svkDcmHeader::CORONAL] ) {
            continue; // If this row has already been assigned to AXIAL or for CORONAL do not consider it for SAGITTAL
        }
        int LRIndex = 0; // 0 is the LR direction  which corresponds to CORONAL
        double value = fabs(dcos[i][LRIndex]);  
        if( value >= maxValue ) {
            maxValue = value;
            maxIndex = i;
        }
    }
    primaryComponents[svkDcmHeader::SAGITTAL] = maxIndex;

    // Now they we have defined each index, lets give the user the requested index
    return primaryComponents[orientation];
}


/*!
 * Determine if the data set is coronal/sagital/axial
 */
svkDcmHeader::Orientation svkDcmHeader::GetOrientationType( )
{
    Orientation orientation = UNKNOWN_ORIENTATION;
    int axialIndex =    this->GetOrientationIndex(AXIAL);
    int coronalIndex =  this->GetOrientationIndex(CORONAL);
    int sagittalIndex = this->GetOrientationIndex(SAGITTAL);

    // Orientation type is defined as the SLICE dimension
    int sliceIndex = 2;
    if( axialIndex == sliceIndex ) {
        orientation = AXIAL;
    } else if ( coronalIndex == sliceIndex ) {
        orientation = CORONAL;
    } else if( sagittalIndex == sliceIndex ) {
        orientation = SAGITTAL;
    }

    return orientation;
 
}


/*!
 *  Returns the right handed normal vector to the slice defined by DICOM imageOrientationPatient. 
 *  from the DICOM header. 
 */
void svkDcmHeader::GetNormalVector(double normal[3])
{

    double orientation[2][3]; 
    this->GetOrientation( orientation );

    double colVector[3];
    colVector[0] = orientation[0][0];
    colVector[1] = orientation[0][1];
    colVector[2] = orientation[0][2];

    double rowVector[3];
    rowVector[0] = orientation[1][0];
    rowVector[1] = orientation[1][1];
    rowVector[2] = orientation[1][2];

    vtkMath* math = vtkMath::New();
    math->Cross(colVector, rowVector, normal);
    math->Delete();
}


/*!
 *  Sets the slice order value so that the sense of the normal to the orientation 
 *  plane can be calculated correctly.  
 */
void svkDcmHeader::SetSliceOrder( DcmDataOrderingDirection sliceOrderVal )
{
    this->dataSliceOrder = sliceOrderVal; 
}


/*!
 *  Returns the 3x3 DCOS matrix with slice vector indicating the direction of data 
 *  ordering in the data set.  i.e. in LPS, 0,0,-1 indicates data is ordered by slice 
 *  from S to I. 
 */
void svkDcmHeader::GetDataDcos(double dcos[3][3], DcmDataOrderingDirection sliceOrderVal)
{
    if (sliceOrderVal == SLICE_ORDER_UNDEFINED && 
        this->dataSliceOrder == SLICE_ORDER_UNDEFINED) {
        vtkErrorWithObjectMacro(this, "GetDataDcos slice order is undefined");
    } else if (sliceOrderVal == SLICE_ORDER_UNDEFINED && 
               this->dataSliceOrder != SLICE_ORDER_UNDEFINED) {
        sliceOrderVal = this->dataSliceOrder; 
    }

    double orientation[2][3];
    this->GetOrientation( orientation );

    double normal[3];   
    this->GetNormalVector( normal );
    if (sliceOrderVal == INCREMENT_ALONG_NEG_NORMAL) {
        for (int i = 0; i < 3; i++) {
            normal[i] *= -1; 
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            dcos[i][j] = orientation[i][j];  
        } 
    } 

    for (int j = 0; j < 3; j++) {
        dcos[2][j] = normal[j]; 
    }

    //
    //  change -0.0000 to 0.0000
    //
    for( int i = 0; i <= 2; i++ ) {
        for( int j = 0; j <= 2; j++ ) {
            dcos[i][j] = (dcos[i][j] == 0.0)? 0.0 : dcos[i][j];
        }
    }

}


/*!
 *  Method that copies a DICOM heder to the input (headerCopy).  New instance UIDs are generated
 *  for SeriesInstanceUID, SOPInstanceUID, MediaStorageSOPInstanceUID.  StudyInstanceuUID is  
 *  preserved.  
 */
void svkDcmHeader::MakeDerivedDcmHeader(svkDcmHeader* headerCopy, string seriesDescription)
{
    this->CopyDcmHeader(headerCopy);  
    headerCopy->InsertUniqueUID("SeriesInstanceUID");
    headerCopy->InsertUniqueUID("SOPInstanceUID");
    headerCopy->SetValue("ImageType", "DERIVED\\SECONDARY");
    headerCopy->SetSliceOrder(this->dataSliceOrder);
    if ( !seriesDescription.empty() ) {
        headerCopy->SetValue("SeriesDescription", seriesDescription);
    }
}


/*!
 *  Method that updates the current DICOM heder. New instance UIDs are generated
 *  for SeriesInstanceUID, SOPInstanceUID, MediaStorageSOPInstanceUID.  StudyInstanceuUID is  
 *  preserved.  
 */
void svkDcmHeader::MakeDerivedDcmHeader(string seriesDescription)
{
    this->InsertUniqueUID("SeriesInstanceUID");
    this->InsertUniqueUID("SOPInstanceUID");
    this->SetValue("ImageType", "DERIVED\\SECONDARY");
    this->SetSliceOrder(this->dataSliceOrder);
    if ( !seriesDescription.empty() ) {
        this->SetValue("SeriesDescription", seriesDescription);
    }
}


/*!
 *
 */
bool svkDcmHeader::WasModified()
{
    if (this->GetMTime() > lastUpdateTime) {
        return 1;  
    } else {
       return 0;  
    }
}


/*!
 *  
 */
void svkDcmHeader::UpdateSpatialParams()
{

    //  
    //  Note:
    //      that this is sensitive to the order of operations!!!!!    
    //  
    this->lastUpdateTime = this->GetMTime();
    this->UpdateNumTimePoints();
    this->UpdateNumCoils();
    this->UpdateOrientation();
    this->UpdatePixelSize();
    this->UpdateOrigin0();
    this->UpdatePixelSpacing();
}


/*!
 *
 */
void svkDcmHeader::UpdateOrientation()
{

    istringstream* iss = new istringstream();
    string orientationString;

    //  The 6 elements of the ImageOrientationPatient are the in plane vectors along the
    //  row and column directions:

    //  for multi-frame objects, orientation is in shared functional group sequence: 
    bool inSharedFunctionalGroup = false; 
    if( this->ElementExists( "ImageOrientationPatient", "SharedFunctionalGroupsSequence") ) {
        inSharedFunctionalGroup = true; 
    } 

    int linearIndex = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            if ( inSharedFunctionalGroup ) {
                orientationString = this->GetStringSequenceItemElement(
                    "PlaneOrientationSequence",
                    0,
                    "ImageOrientationPatient",
                    linearIndex,
                    "SharedFunctionalGroupsSequence",
                    0
                );
            } else {
                orientationString = this->GetStringValue( "ImageOrientationPatient", linearIndex ); 
            }
            iss->str(orientationString);
            *iss >> this->orientation[i][j];
            iss->clear();
            linearIndex++;
        }
    }

    delete iss;
}


/*!
 *
 */
void svkDcmHeader::UpdatePixelSize()
{

    istringstream* iss = new istringstream();
    string sizeString;
    string parentSequence;

    //  for multi-frame objects, orientation is in shared functional group sequence: 
    bool inFunctionalGroup = false; 
    if( this->ElementExists( "PixelSpacing", "PerFrameFunctionalGroupsSequence" ) ) {
        parentSequence = "PerFrameFunctionalGroupsSequence";
        inFunctionalGroup = true; 
    } else if( this->ElementExists( "PixelSpacing", "SharedFunctionalGroupsSequence" ) ) {
        parentSequence = "SharedFunctionalGroupsSequence";
        inFunctionalGroup = true; 
    } 


    for (int i = 0; i < 2; i++ ) {
        if ( inFunctionalGroup ) {
            sizeString = this->GetStringSequenceItemElement(
                "PixelMeasuresSequence",
                0,
                "PixelSpacing",
                i,
                parentSequence.c_str(),
                0
            );
        } else {
            sizeString = this->GetStringValue( "PixelSpacing", i ); 
        }
        iss->str(sizeString);
        *iss >> pixelSize[i];
        iss->clear();
    }

    if( this->ElementExists( "SliceThickness", parentSequence.c_str()) ) {
        pixelSize[2] = this->GetDoubleSequenceItemElement(
            "PixelMeasuresSequence",
            0,
            "SliceThickness",
            parentSequence.c_str(),
            0
        );
    } else {
        pixelSize[2] = 0;
        
    }

    delete iss;
}


/*!
 *
 */
void svkDcmHeader::UpdatePixelSpacing()
{
    double size[3];
    this->GetPixelSize(size);
    for (int i = 0; i < 3; i++) {
        this->pixelSpacing[i] = size[i];
    }

    if ( this->GetNumberOfSlices() >= 2 ) {

        //  If can't get origins, then return pixelSize as slice spacing
        double origin0[3];
        if ( this->GetOrigin(origin0, 0) != 0 ) { 
            return; 
        }
        double origin1[3];
        if ( this->GetOrigin(origin1, 1) != 0 ) {
            return; 
        }
        double sliceSpacing = 0;
        for (int i = 0; i < 3; i++ ) {
            sliceSpacing += pow(origin1[i] - origin0[i], 2);
        }
        sliceSpacing = pow(sliceSpacing, .5);
        //cout << "SLICE SPACING WITH GAP: " << sliceSpacing << endl;

        this->pixelSpacing[2] = sliceSpacing;
    }
    //cout << "Spacing: " << this->pixelSpacing[0] << " " << this->pixelSpacing[1] << " " << this->pixelSpacing[2] << endl;

}


/*!
 *
 */
void svkDcmHeader::UpdateOrigin0()
{

    if ( this->ElementExists("ImagePositionPatient") == true ) {

        istringstream* iss = new istringstream();
        string originString; 
        int frameNumber = 0; 

        bool inFunctionalGroup = false; 
        if( this->ElementExists( "ImagePositionPatient", "PerFrameFunctionalGroupsSequence") ) {
            inFunctionalGroup = true; 
        } 

        //  Iterate of all 3 positions of the ImagePositionPatient element;
        for (int i = 0; i < 3; i++) {
            if ( inFunctionalGroup ) {
                originString = this->GetStringSequenceItemElement(
                    "PlanePositionSequence",
                    0,
                    "ImagePositionPatient",
                    i, 
                    "PerFrameFunctionalGroupsSequence",
                    frameNumber
                );
            } else {
                originString = this->GetStringValue( "ImagePositionPatient", i ); 
            } 
            iss->str(originString);
            *iss >> this->origin0[i];
            iss->clear();
        }

        delete iss;

    } else {

        if (this->GetDebug()) {
            vtkWarningWithObjectMacro(
                this, 
                "::UpdateOrigin0(): ImagePositionPatient not defined, can't update Origin0." 
            );    
        }
    }
}


/*!
 *  Returns the number of time points from the DcmHeader.
 *  Parses FrameContentSequence for the number of 
 *  coils, if specified in the Multi Frame Dimension Module
 */
int svkDcmHeader::GetNumberOfCoils()
{

    if ( this->WasModified() ) {
        this->UpdateSpatialParams();
    }

    return this->numCoils; 

}


/*!
 *
 */
void svkDcmHeader::UpdateNumCoils()
{
    this->numCoils = 1;

    //  Determine which index in the DimensionIndexValues attribute represents
    //  the coil number index.  Should use "DimensionIndexPointer" (to do).
    int coilIndexNumber = this->GetDimensionIndexPosition( "Coil Number" ); 
    this->numCoils = this->GetNumberOfFramesInDimension( coilIndexNumber ); 
}


/*!
 *  Returns the number of time points from the DcmHeader.
 *  Parses FrameContentSequence for the number of 
 *  time points, if specified in the Multi Frame Dimension Module
 */
int svkDcmHeader::GetNumberOfTimePoints()
{

    if ( this->WasModified() ) {
        this->UpdateSpatialParams();
    }

    return this->numTimePts; 
}


/*!
 *
 */
void svkDcmHeader::UpdateNumTimePoints()
{
    this->numTimePts = 1;

    //  Determine which index in the DimensionIndexValues attribute represents
    //  the coil number index.  Should use "DimensionIndexPointer" (to do).
    int timeIndexNumber = GetDimensionIndexPosition( "Time Point" ); 
    if (timeIndexNumber != -1 ) {
        this->numTimePts = GetNumberOfFramesInDimension( timeIndexNumber ); 
    }
}


/*!
 *  Gets the position of the index representing the specified dimension type
 *  from the DimensionIndexSequence.  e.g. given 3 indices "0/1/2", which
 *  one represents slice, coil number, etc.
 *  Returns -1 if dimension can't be determined.   
 */
int svkDcmHeader::GetDimensionIndexPosition(string indexLabel)    
{
    int dimIndexNumber = -1; 

    int numDims = this->GetNumberOfItemsInSequence("DimensionIndexSequence");
    for ( int i = 0; i < numDims; i++ ) {

        string dimensionLabel = this->GetStringSequenceItemElement(
            "DimensionIndexSequence",
            i, 
            "DimensionDescriptionLabel",
            NULL, 
            i
        );
        if ( dimensionLabel.compare( indexLabel ) == 0 ) {
            dimIndexNumber = i; 
        }

    }

    return dimIndexNumber; 
}


/*!
 *  Gets the number of frames in the specified dimension. 
 */
int svkDcmHeader::GetNumberOfFramesInDimension( int dimensionIndex )
{
    int numberOfFrames = this->GetNumberOfFrames();

    int numFramesInDimension; 

    if ( dimensionIndex >= 0 ) {
        set <int> frames;
        for (int i = 0; i < numberOfFrames; i++ ) {

            //get number of coils and divide numberof frames by it to get number of slices
            int value = this->GetIntSequenceItemElement(
                "FrameContentSequence",
                0, //frame number
                "DimensionIndexValues",
                "PerFrameFunctionalGroupsSequence",
                i,
                dimensionIndex  // position of the coil index in the index array. 
            );
            frames.insert(value);
        }

        numFramesInDimension = frames.size();

    } else {
        numFramesInDimension = 1; 
    }

    return numFramesInDimension; 
}


/*!
 *
 */
int svkDcmHeader::GetNumberOfSlices()
{
    int numberOfFrames = this->GetNumberOfFrames();
    int numberOfCoils = this->GetNumberOfCoils();
    int numberOfTimePts = this->GetNumberOfTimePoints();
    return numberOfFrames/(numberOfCoils * numberOfTimePts ) ;
}


/*!
 *  Get the number of Dimension indices requred 
 *  for specifying the frame's slice, time, coil. 
 *  See FrameContentSequence DimensionIndexValues
 */
int svkDcmHeader::GetNumberOfDimensionIndices(int numTimePts, int numCoils)
{
    //  retain order slice, time, coil
    int numDimensionIndices = 1;    //Default is slice only 
    if ( numTimePts > 1 ) {
        numDimensionIndices++;
    }
    if ( numCoils > 1 ) {
        numDimensionIndices++;
    }
    return numDimensionIndices;

}


/*!
 *  Get the Dimension Indices for a given slice, timePt and Coil
 */
void svkDcmHeader::SetDimensionIndices(unsigned int* indexValues, int numFrameIndices, int sliceNum, int timePt, int coilNum, int numTimePts, int numCoils)
{
    indexValues[0] = sliceNum;
    if ( numFrameIndices >= 2 ) {
        if ( numTimePts > 1 ) {
            indexValues[1] = timePt;
        } else if (numTimePts == 1 && numCoils > 1) {
            indexValues[1] = coilNum;
        }
    }
    if ( numFrameIndices == 3 ) {
        if ( numCoils > 1 ) {
            indexValues[2] = coilNum;
        }
    }
}


/*!
 *  Initializes the DICOM Patient Module (Patient IE)
 */
void svkDcmHeader::InitPatientModule(vtkstd::string PatientName, vtkstd::string patientID, vtkstd::string PatientBirthDate, vtkstd::string PatientSex)
{
    if ( !PatientName.empty() ) {
        this->SetValue(
            "PatientName",
            PatientName
        );
    }

    if ( !patientID.empty() ) {
        this->SetValue(
            "PatientID",
            patientID
        );
    }
    if ( !PatientBirthDate.empty() ) {
        this->SetValue(
            "PatientBirthDate",
            PatientBirthDate
        );
    }
    if ( !PatientSex.empty() ) {
        this->SetValue(
            "PatientSex",
            PatientSex
        );
    }
}


/*!
 *  Initializes the DICOM Patient Module (Patient IE)
 */
void svkDcmHeader::InitGeneralStudyModule(vtkstd::string studyDate, vtkstd::string studyTime, vtkstd::string referringPhysiciansName, vtkstd::string studyID, vtkstd::string accessionNumber, vtkstd::string studyInstanceUID)
{

    if ( !studyDate.empty() ) {
        this->SetValue(
            "StudyDate",
            studyDate
        );
    }

    if ( !studyTime.empty() ) {
        this->SetValue(
            "StudyTime",
            studyTime
        );
    }

    if ( !referringPhysiciansName.empty() ) {
        this->SetValue(
            "ReferringPhysicianName",
            referringPhysiciansName
        );
    }

    if ( !studyID.empty() ) {
        this->SetValue(
            "StudyID",
            studyID
        );
    }

    if ( !accessionNumber.empty() ) {
        this->SetValue(
            "AccessionNumber",
            accessionNumber
        );
    }

    if ( !studyInstanceUID.empty() ) {
        this->SetValue(
            "StudyInstanceUID",
            studyInstanceUID
        );
    }
}


/*!
 *  Initializes the DICOM General Series Module (Series IE)
 *  Modality is set on initialztion of IOD (svkIOD).  
 */
void svkDcmHeader::InitGeneralSeriesModule(vtkstd::string seriesNumber, vtkstd::string seriesDescription, vtkstd::string patientPosition)
{

    if ( !seriesNumber.empty() ) {
        this->SetValue(
            "SeriesNumber",
            seriesNumber
        );
    }

    if ( !seriesDescription.empty() ) {
        this->SetValue(
            "SeriesDescription",
            seriesDescription
        );
    }

    if ( !patientPosition.empty() ) {
        this->SetValue(
            "PatientPosition",
            patientPosition
        );
    }
}


/*!
 *  Initializes the DICOM Image Pixel Module
 */
void svkDcmHeader::InitImagePixelModule( int rows, int columns, svkDcmHeader::DcmPixelDataFormat dataType)
{
    this->SetValue( "Rows", rows );
    this->SetValue( "Columns", columns );
    this->SetPixelDataType( dataType );
}


/*!
 *
 */
void svkDcmHeader::InitPlaneOrientationMacro( double dcos[3][3] )
{

    ostringstream ossDcos;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            ossDcos << dcos[i][j];
            if (i * j != 2) {
                ossDcos<< "\\";
            }
        }
    }


    this->InitPlaneOrientationMacro( ossDcos.str() ); 

    //  Set slice order based on the normal to the newly set orientation: 
    double normal[3];
    this->GetNormalVector(normal);

    double dcosSliceOrder[3];
    for (int i = 0; i < 3; i++) {
        dcosSliceOrder[i] = dcos[2][i];
    }

    //  Use the scalar product to determine whether the data in the .cmplx
    //  file is ordered along the slice normal or antiparalle to it.
    vtkMath* math = vtkMath::New();
    if (math->Dot(normal, dcosSliceOrder) > 0 ) {
        this->dataSliceOrder = svkDcmHeader::INCREMENT_ALONG_POS_NORMAL;
    } else {
        this->dataSliceOrder = svkDcmHeader::INCREMENT_ALONG_NEG_NORMAL;
    }
    this->SetSliceOrder( this->dataSliceOrder );
    math->Delete();

}


/*!
 *
 */
void svkDcmHeader::InitPlaneOrientationMacro( vtkstd::string orientationString )
{

    this->ClearSequence( "PlaneOrientationSequence" );

    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "PlaneOrientationSequence"
    );


    this->AddSequenceItemElement(
        "PlaneOrientationSequence",
        0,
        "ImageOrientationPatient",
        orientationString,
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initialize the MR Image Module. 
 */
void svkDcmHeader::InitMRImageModule( vtkstd::string repetitionTime, vtkstd::string echoTime)
{
    if ( !repetitionTime.empty() ) {
        this->SetValue( "RepetitionTime", repetitionTime);
    }

    if ( !echoTime.empty() ) {
        this->SetValue( "EchoTime", echoTime);
    }
}


/*!
 *  Initialize the ImagePlane Module. 
 *  For single frame objects this may be called repeatedly changing only the value of the 
 *  current instance ImagePositionPatient value. 
 */
void svkDcmHeader::InitImagePlaneModule( vtkstd::string imagePositionPatient, vtkstd::string pixelSpacing, vtkstd::string imageOrientationPatient, vtkstd::string sliceThickness)
{
    if ( !pixelSpacing.empty() ) {
        this->SetValue( "PixelSpacing", pixelSpacing);
    }

    if ( !imageOrientationPatient.empty() ) {
        this->SetValue( "ImageOrientationPatient", imageOrientationPatient);
    }

    if ( !imagePositionPatient.empty() ) {
        this->SetValue( "ImagePositionPatient", imagePositionPatient);
    }

    if ( !sliceThickness.empty() ) {
        this->SetValue( "SliceThickness", sliceThickness);
    }
}


/*!
 *  Initialize the Pixel Measures Macro. 
 */
void svkDcmHeader::InitPixelMeasuresMacro( vtkstd::string pixelSpacing, vtkstd::string sliceThickness )
{
    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "PixelMeasuresSequence"
    );


    this->AddSequenceItemElement(
        "PixelMeasuresSequence",
        0,
        "PixelSpacing",
        pixelSpacing,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "PixelMeasuresSequence",
        0,
        "SliceThickness",
        sliceThickness,
        "SharedFunctionalGroupsSequence",
        0
    );
}



/*!
 *  Initializes the Per Frame Functional Gruop Sequence and attributes derived from 
 *  method args, eg. dcos -> ImageOrientationPatient.  
 */
void svkDcmHeader::InitPerFrameFunctionalGroupSequence(double toplc[3], double voxelSpacing[3],
                                             double dcos[3][3], int numSlices, int numTimePts, int numCoils)
{

    this->ClearSequence( "PerFrameFunctionalGroupsSequence" );
    this->SetValue( "NumberOfFrames", numSlices * numTimePts * numCoils );  
    this->InitMultiFrameDimensionModule( numSlices, numTimePts, numCoils ); 
    this->InitFrameContentMacro( numSlices, numTimePts, numCoils ); 
    this->InitPlaneOrientationMacro( dcos ); 
    this->InitPlanePositionMacro( toplc, voxelSpacing, dcos, numSlices, numTimePts, numCoils); 
}
 

/*!
 *
 */
void svkDcmHeader::InitMultiFrameDimensionModule( int numSlices, int numTimePts, int numCoils )
{

    this->ClearSequence( "DimensionIndexSequence" );

    int indexCount = 0;

    this->AddSequenceItemElement(
        "DimensionIndexSequence",
        indexCount,
        "DimensionDescriptionLabel",
        "Slice"
    );

    if ( numTimePts > 1 ) {
        indexCount++;
        this->AddSequenceItemElement(
            "DimensionIndexSequence",
            indexCount,
            "DimensionDescriptionLabel",
            "Time Point"
        );
    }

    if ( numCoils > 1 ) {
        indexCount++;
        this->AddSequenceItemElement(
            "DimensionIndexSequence",
            indexCount,
            "DimensionDescriptionLabel",
            "Coil Number"
        );
    }
}


/*!
 *
 */
void svkDcmHeader::InitPlanePositionMacro(double toplc[3], double voxelSpacing[3],
                                             double dcos[3][3], int numSlices, int numTimePts, int numCoils)
{

    int frame = 0;

    for (int coilNum = 0; coilNum < numCoils; coilNum++) {
        for (int timePt = 0; timePt < numTimePts; timePt++) {

            float displacement[3]={0,0,0};
            float frameLPSPosition[3]={0,0,0};

            for (int i = 0; i < numSlices; i++) {

                this->AddSequenceItemElement(
                    "PerFrameFunctionalGroupsSequence",
                    i,
                    "PlanePositionSequence"
                );

                //add displacement along normal vector:
                for (int j = 0; j < 3; j++) {
                    displacement[j] = dcos[2][j] * voxelSpacing[2] * i;
                }
                for(int j = 0; j < 3; j++) { //L, P, S
                    frameLPSPosition[j] = toplc[j] +  displacement[j] ;
                }
    
                string imagePositionPatient;
                for (int j = 0; j < 3; j++) {
                    ostringstream oss;
                    oss.setf(ios::fixed);
                    oss.precision(5);
                    oss << frameLPSPosition[j];
                    imagePositionPatient += oss.str();
                    if (j < 2) {
                        imagePositionPatient += '\\';
                    }
                }
    
                this->AddSequenceItemElement(
                    "PlanePositionSequence",
                    0,
                    "ImagePositionPatient",
                    imagePositionPatient,
                    "PerFrameFunctionalGroupsSequence",
                    frame
                );

                frame++;
            }
        }
    }
}


/*!
 *  Mandatory, Must be a per-frame functional group
 */
void svkDcmHeader::InitFrameContentMacro( int numSlices, int numTimePts, int numCoils )
{

    if ( numSlices < 0 ) {
        numSlices = this->GetNumberOfSlices(); 
    }

    if ( numTimePts < 0 ) {
        numTimePts = this->GetNumberOfTimePoints(); 
    }

    if ( numCoils < 0 ) {
        numCoils = this->GetNumberOfCoils(); 
    }
    
    int numFrameIndices = svkDcmHeader::GetNumberOfDimensionIndices( numTimePts, numCoils ) ;

    unsigned int* indexValues = new unsigned int[numFrameIndices];

    int frame = 0;

    for (int coilNum = 0; coilNum < numCoils; coilNum++) {

        for (int timePt = 0; timePt < numTimePts; timePt++) {

            for (int sliceNum = 0; sliceNum < numSlices; sliceNum++) {

                svkDcmHeader::SetDimensionIndices(
                    indexValues, numFrameIndices, sliceNum, timePt, coilNum, numTimePts, numCoils
                );

                this->AddSequenceItemElement(
                    "PerFrameFunctionalGroupsSequence",
                    frame,
                    "FrameContentSequence"
                );

                this->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "DimensionIndexValues",
                    indexValues,        //  array of vals
                    numFrameIndices,    // num values in array
                    "PerFrameFunctionalGroupsSequence",
                    frame
                );

                this->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "FrameAcquisitionDateTime",
                    "EMPTY_ELEMENT",
                    "PerFrameFunctionalGroupsSequence",
                    frame
                );

                this->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "FrameReferenceDateTime",
                    "EMPTY_ELEMENT",
                    "PerFrameFunctionalGroupsSequence",
                    frame
                );

                this->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "FrameAcquisitionDuration",
                    "-1",
                    "PerFrameFunctionalGroupsSequence",
                    frame
                );

                frame++;
            }
        }
    }

    delete[] indexValues;
}


/*!
 *  Set the linear scaling factors in the VOI LUT Module.  These define the output range 
 *  of pixel values to be displlayed. 
 */
void svkDcmHeader::InitVOILUTModule(float center, float width)
{

    this->SetValue(
        "WindowCenter",
        center 
    );

    this->SetValue(
        "WindowWidth",
        width 
    );

    this->SetValue(
        "VOILUTFunction",
        "LINEAR" 
    );
}


/*!
 *  Set the linear scaling factors in the Pixel Value Transformation Macro
 */
void svkDcmHeader::InitPixelValueTransformationMacro(float slope, float intercept)
{

    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "PixelValueTransformationSequence"
    );

    this->AddSequenceItemElement(
        "PixelValueTransformationSequence",
        0,
        "RescaleIntercept",
        intercept,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "PixelValueTransformationSequence",
        0,
        "RescaleSlope",
        slope,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "PixelValueTransformationSequence",
        0,
        "RescaleType",
        "US",   //enum unspecified required for MR modality
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initialize MR Imaging Modifier Macro
 */
void svkDcmHeader::InitMRImagingModifierMacro(float transmitFreq, float pixelBandwidth, vtkstd::string magTransfer, vtkstd::string bloodNulling)
{

    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRImagingModifierSequence"
    );

    this->AddSequenceItemElement(
        "MRImagingModifierSequence",
        0,
        "MagnetizationTransfer",
        magTransfer,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRImagingModifierSequence",
        0,
        "BloodSignalNulling",
        bloodNulling,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRImagingModifierSequence",
        0,
        "Tagging",
        vtkstd::string("NONE"),
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRImagingModifierSequence",
        0,
        "TransmitterFrequency",
        transmitFreq,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRImagingModifierSequence",
        0,
        "PixelBandwidth",
        pixelBandwidth, 
        "SharedFunctionalGroupsSequence",
        0
    );
}

/*!
 *  Initializes the Volume Localization Sequence in the MRSpectroscopy
 *  DICOM object for PRESS excitation.
 */
void svkDcmHeader::InitVolumeLocalizationSeq(float size[3], float center[3], float dcos[3][3])
{

    this->InsertEmptyElement( "VolumeLocalizationSequence" );

    string midSlabPosition;
    for (int i = 0; i < 3; i++) {
        ostringstream oss;
        oss << center[i];
        midSlabPosition += oss.str();
        if (i < 2) {
            midSlabPosition += '\\';
        }
    }

    //  Volume Localization (PRESS BOX)
    for (int i = 0; i < 3; i++) {

        this->AddSequenceItemElement(
            "VolumeLocalizationSequence",
            i,
            "SlabThickness",
            size[i]
        );

        this->AddSequenceItemElement(
            "VolumeLocalizationSequence",
            i,
            "MidSlabPosition",
            midSlabPosition
        );

        string slabOrientation;
        for (int j = 0; j < 3; j++) {
            ostringstream oss;
            oss << dcos[i][j];
            slabOrientation += oss.str();
            if (j < 2) {
                slabOrientation += '\\';
            }
        }

        this->AddSequenceItemElement(
            "VolumeLocalizationSequence",
            i,
            "SlabOrientation",
            slabOrientation
        );

    }
}


/*!
 *  Initialize MR Timing and Related Parameters Macro. 
 */
void svkDcmHeader::InitMRTimingAndRelatedParametersMacro(float tr, float flipAngle, int numEchoes)
{

    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRTimingAndRelatedParametersSequence"
    );

    this->AddSequenceItemElement(
        "MRTimingAndRelatedParametersSequence",
        0,
        "RepetitionTime",
        tr,
        "SharedFunctionalGroupsSequence",
        0
    );

    if ( flipAngle == -999 ) {
        this->AddSequenceItemElement(
            "MRTimingAndRelatedParametersSequence",
            0,
            "FlipAngle",
            "UNKNOWN",
            "SharedFunctionalGroupsSequence",
            0
        );
    } else {
        this->AddSequenceItemElement(
            "MRTimingAndRelatedParametersSequence",
            0,
            "FlipAngle",
            flipAngle,
            "SharedFunctionalGroupsSequence",
            0
        );
    }

    this->AddSequenceItemElement(
        "MRTimingAndRelatedParametersSequence",
        0,
        "EchoTrainLength",
        numEchoes,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRTimingAndRelatedParametersSequence",
        0,
        "RFEchoTrainLength",
        1,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRTimingAndRelatedParametersSequence",
        0,
        "GradientEchoTrainLength",
        1,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRTimingAndRelatedParametersSequence",
        0,
        "GradientEchoTrainLength",
        1,
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initializes the MR Echo Macro. 
 */
void svkDcmHeader::InitMREchoMacro(float TE)
{
    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MREchoSequence"
    );
    this->AddSequenceItemElement(
        "MREchoSequence",
        0,
        "EffectiveEchoTime",
        TE,
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initialize MR Modifier Macro. 
 */
void svkDcmHeader::InitMRModifierMacro(float inversionTime)
{   
    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRModifierSequence"
    );

    string invRecov = "NO"; 
    if ( inversionTime >= 0 ) {
        invRecov.assign("YES");
        this->AddSequenceItemElement(
            "MRModifierSequence",
            0,
            "InversionTimes",
            inversionTime,
            "SharedFunctionalGroupsSequence",
            0
        );
    }

    this->AddSequenceItemElement(
        "MRModifierSequence",
        0,
        "InversionRecovery",
        invRecov,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRModifierSequence",
        0,
        "SpatialPresaturation",
        string("SLAB"),
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRModifierSequence",
        0,
        "ParallelAcquisition",
        string("NO"),
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initialize MR Transmit Coil Macro. 
 */
void svkDcmHeader::InitMRTransmitCoilMacro(string coilMfg, string coilName, string coilType)
{   
    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRTransmitCoilSequence"
    );

    this->AddSequenceItemElement(
        "MRTransmitCoilSequence",
        0,
        "TransmitCoilName",
        coilName,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRTransmitCoilSequence",
        0,
        "TransmitCoilManufacturerName",
        coilMfg,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->AddSequenceItemElement(
        "MRTransmitCoilSequence",
        0,
        "TransmitCoilType",
        coilType,
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initialize MR Averages Macro. 
 */
void svkDcmHeader::InitMRAveragesMacro(int numAverages)
{
    this->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRAveragesSequence"
    );

    this->AddSequenceItemElement(
        "MRAveragesSequence",
        0,
        "NumberOfAverages",
        numAverages,
        "SharedFunctionalGroupsSequence",
        0
    );
}


/*!
 *  Initialize Raw Data Module. 
 */
void svkDcmHeader::InitRawDataModule( vtkstd::string contentDate, vtkstd::string contentTime, void* rawFile ) 
{

    if ( !contentDate.empty() ) {
        this->SetValue(
            "ContentDate",
            contentDate
        );
    }

    if ( !contentTime.empty() ) {
        this->SetValue(
            "ContentTime",
            contentTime
        );
    }
}


/*!
 * 
 */
svkDcmHeader::DcmPixelDataFormat svkDcmHeader::GetVtkDataTypeFromSvkDataType( vtkIdType vtkType)
{
    if ( vtkType == VTK_DOUBLE ) {
        return svkDcmHeader::SIGNED_FLOAT_8;
    } else if ( vtkType == VTK_FLOAT ) {
        return svkDcmHeader::SIGNED_FLOAT_4;
    } else if ( vtkType == VTK_FLOAT ) {
        return svkDcmHeader::SIGNED_FLOAT_4;
    } else if ( vtkType == VTK_UNSIGNED_CHAR) {
        return svkDcmHeader::UNSIGNED_INT_1;
    } else if ( vtkType == VTK_UNSIGNED_SHORT) {
        return svkDcmHeader::UNSIGNED_INT_2;
    } else if ( vtkType == VTK_SHORT) {
        return svkDcmHeader::SIGNED_INT_2;
    } else {
        return svkDcmHeader::UNDEFINED; 
    }
}

/*!
 *  Initializes an MR Image Storage header from an Enhanced  MRI Storage header. 
 */
int svkDcmHeader::ConvertEnhancedMriToMriHeader(svkDcmHeader* mri, vtkIdType dataType )
{

    //
    //  Patient IE requires modification
    //
    mri->InitPatientModule(
        this->GetStringValue( "PatientName" ),
        this->GetStringValue( "PatientID" ),
        this->GetStringValue( "PatientBirthDate" ),
        this->GetStringValue( "PatientSex" )
    );


    //
    //  General Study IE requires modification
    //
    mri->InitGeneralStudyModule(
        this->GetStringValue( "StudyDate" ),
        this->GetStringValue( "StudyTime" ),
        this->GetStringValue( "ReferringPhysicianName" ),
        this->GetStringValue( "StudyID" ),
        this->GetStringValue( "AccessionNumber" ), 
        this->GetStringValue( "StudyInstanceUID" )
    );

    //
    //  General Series Module
    //
    mri->InitGeneralSeriesModule(
        "77",
        this->GetStringValue( "SeriesDescription" ),
        this->GetStringValue( "PatientPosition" )
    );

    //  Note that the initial copy doesn't init the position value. 
    //  Each file will need to do this for the appropriate frame
    mri->InitImagePlaneModule( 
        "", 
        this->GetStringSequenceItemElement (
            "PixelMeasuresSequence",
            0,
            "PixelSpacing",
            "SharedFunctionalGroupsSequence"
        ), 
        this->GetStringSequenceItemElement(
            "PlaneOrientationSequence",
            0,
            "ImageOrientationPatient",
            "SharedFunctionalGroupsSequence"
        ), 
        this->GetStringSequenceItemElement (
            "PixelMeasuresSequence",
            0,
            "SliceThickness",
            "SharedFunctionalGroupsSequence"
        )
    );

    //
    //  Image Pixel Module
    //  Set DCM data type based on vtkImageData Scalar type:
    //
    svkDcmHeader::DcmPixelDataFormat dcmDataType;
    dcmDataType = static_cast< svkDcmHeader::DcmPixelDataFormat > (this->GetPixelDataType( dataType ) ); 

    mri->InitImagePixelModule(
        this->GetIntValue( "Rows"),
        this->GetIntValue( "Columns"),
        dcmDataType
    );

    //  Init MR Image Module
    mri->InitMRImageModule( 
        this->GetStringSequenceItemElement(
            "MRTimingAndRelatedParametersSequence",
            0,
            "RepetitionTime",
            "SharedFunctionalGroupsSequence"
        ), 
        this->GetStringSequenceItemElement(
            "MREchoSequence",
            0,
            "EffectiveEchoTime",
            "SharedFunctionalGroupsSequence"
        )
    ); 


    return 0; 
}


/*!
 *  Initializes an Enhanced MR Image header from an MR Spectroscopy header. 
 *  This is a utility for extracting metabolite maps from MRS data, for example. 
 *  Takes an initialized svkDcmHeader from an svkMriImageData object. 
 */
int svkDcmHeader::ConvertMrsToMriHeader(svkDcmHeader* mri, vtkIdType dataType, vtkstd::string seriesDescription)
{

    //"verify that "this" is an svkMRSpectroscopy Object"

    mri->SetValue("ImageType", "DERIVED\\SECONDARY");

    //
    //  Patient IE requires modification
    //
    mri->InitPatientModule(
        this->GetStringValue( "PatientName" ),
        this->GetStringValue( "PatientID" ),
        this->GetStringValue( "PatientBirthDate" ),
        this->GetStringValue( "PatientSex" )
    );


    //
    //  General Study IE requires modification
    //
    mri->InitGeneralStudyModule(
        this->GetStringValue("StudyDate"),
        this->GetStringValue("StudyTime"),
        this->GetStringValue("ReferringPhysicianName"),
        this->GetStringValue("StudyID"),
        this->GetStringValue("AccessionNumber"), 
        this->GetStringValue("StudyInstanceUID")
    );

    //
    //  General Series Module
    //
    mri->InitGeneralSeriesModule(
        "77",
        seriesDescription,
        this->GetStringValue("PatientPosition")
    );

    //
    //  Image Pixel Module
    //  Set DCM data type based on vtkImageData Scalar type:
    //
    svkDcmHeader::DcmPixelDataFormat dcmDataType; 

    if ( dataType == VTK_DOUBLE ) {
        dcmDataType = svkDcmHeader::SIGNED_FLOAT_8;
    } else if ( dataType == VTK_FLOAT ) {
        dcmDataType = svkDcmHeader::SIGNED_FLOAT_4;
    } else {
        cout << this->GetClassName() << ": Unsupported ScalarType " << dataType << endl;
        exit(1);
    }

    mri->InitImagePixelModule(
        this->GetIntValue( "Rows"),
        this->GetIntValue( "Columns"),
        dcmDataType
    );

    //
    //  Per Frame Functinal Groups Module
    //
    int numSlices = this->GetNumberOfSlices();
    double dcos[3][3];
    this->GetDataDcos( dcos );

    double pixelSpacing[3];
    this->GetPixelSpacing( pixelSpacing );

    double toplc[3];
    this->GetOrigin( toplc, 0 );


    mri->SetSliceOrder( this->dataSliceOrder );

    // Add Pixel Spacing
    vtkstd::string pixelSizes = this->GetStringSequenceItemElement (
                                        "PixelMeasuresSequence",
                                        0,
                                        "PixelSpacing",
                                        "SharedFunctionalGroupsSequence"
                                    );

    vtkstd::string sliceThickness = this->GetStringSequenceItemElement (
                                        "PixelMeasuresSequence",
                                        0,
                                        "SliceThickness",
                                        "SharedFunctionalGroupsSequence"
                                    );

    mri->InitPixelMeasuresMacro(  pixelSizes, sliceThickness );

   mri->AddSequenceItemElement(
            "SharedFunctionalGroupsSequence",
            0,
            "PlaneOrientationSequence"
        );

    mri->InitPlaneOrientationMacro(
        this->GetStringSequenceItemElement(
            "PlaneOrientationSequence",
            0,
            "ImageOrientationPatient",
            "SharedFunctionalGroupsSequence"
        )
    );

    mri->InitPerFrameFunctionalGroupSequence( toplc, pixelSpacing, dcos, numSlices, 1, 1 );

    return 0; 
}


/*!
 *  Replaces PHI field with "DEIDENTIFIED" string. If phiType is LIMITED, then 
 *  dates are preserved. This method does not look at private
 *  or nested tags.  
 *      0008,0018 SOPInstanceUID=UIDROOT,SOPInstanceUID
 *      0008,0020 StudyDate 
 *      0008,0021 SeriesDate
 *      0008,0022 AcquisitionDate
 *      0008,0023 ContentDate
 *      0008,0050 AccessionNumber
 *      0008,0080 InstitutionName
 *      0008,0090 ReferringPhysicianName
 *      0008,1155 RefSOPInstanceUID
 *      0010,0010 PatientName
 *      0010,0020 PatientID
 *      0010,0030 PatientBirthDate
 *      0020,000D StudyInstanceUID
 *      0020,000E SeriesInstanceUID
 *      0020,0010 StudyID
 *      0020,0052 FrameOfReferenceUID
 *      0028,0301 BurnedInAnnotation
 *      0040,A124 UID
 *      0088,0140 StorageMediaFileSetUID
 *      3006,0024 ReferencedFrameOfReferenceUID
 *      3006,00C2 RelatedFrameOfReferenceUID
 */
void svkDcmHeader::Deidentify( PHIType phiType )
{    
    // set both patientId and studyId to DEIDENTIFIED:
    this->Deidentify( phiType, "DEIDENTIFIED" );
}


/*!
 *  Replaces PHI field with id. If phiType is LIMITED, then 
 *  dates are preserved. This method does not look at private
 *  or nested tags.  
 *      0008,0018 SOPInstanceUID=UIDROOT,SOPInstanceUID
 *      0008,0020 StudyDate 
 *      0008,0021 SeriesDate
 *      0008,0022 AcquisitionDate
 *      0008,0023 ContentDate
 *      0008,0050 AccessionNumber
 *      0008,0080 InstitutionName
 *      0008,0090 ReferringPhysicianName
 *      0008,1155 RefSOPInstanceUID
 *      0010,0010 PatientName
 *      0010,0020 PatientID
 *      0010,0030 PatientBirthDate
 *      0020,000D StudyInstanceUID
 *      0020,000E SeriesInstanceUID
 *      0020,0010 StudyID
 *      0020,0052 FrameOfReferenceUID
 *      0028,0301 BurnedInAnnotation
 *      0040,A124 UID
 *      0088,0140 StorageMediaFileSetUID
 *      3006,0024 ReferencedFrameOfReferenceUID
 *      3006,00C2 RelatedFrameOfReferenceUID
 */
void svkDcmHeader::Deidentify( PHIType phiType, string id )
{    
    // set both patientId and studyId to the same value:
    this->Deidentify( phiType, id, id );
}


/*!
 *  Replaces PHI patient fields with patientId, study fields and all other PHI fields 
 *  with studyId.  If phiType is LIMITED, then dates are preserved. This
 *  method does not look at private nested tags. 
 *      0008,0018 SOPInstanceUID=UIDROOT,SOPInstanceUID
 *      0008,0020 StudyDate 
 *      0008,0021 SeriesDate
 *      0008,0022 AcquisitionDate
 *      0008,0023 ContentDate
 *      0008,0050 AccessionNumber
 *      0008,0080 InstitutionName
 *      0008,0090 ReferringPhysicianName
 *      0008,1155 RefSOPInstanceUID
 *      0010,0010 PatientName
 *      0010,0020 PatientID
 *      0010,0030 PatientBirthDate
 *      0020,000D StudyInstanceUID
 *      0020,000E SeriesInstanceUID
 *      0020,0010 StudyID
 *      0020,0052 FrameOfReferenceUID
 *      0028,0301 BurnedInAnnotation
 *      0040,A124 UID
 *      0088,0140 StorageMediaFileSetUID
 *      3006,0024 ReferencedFrameOfReferenceUID
 *      3006,00C2 RelatedFrameOfReferenceUID
 */
void svkDcmHeader::Deidentify( PHIType phiType, string patientId, string studyId )
{     

    //  These fields are removed from PHI_LIMITED and PHI_DEIDENTIFIED data sets: 
    if ( phiType == svkDcmHeader::PHI_DEIDENTIFIED || phiType == PHI_LIMITED ) {

            this->SetValue( "SOPInstanceUID",                studyId); 
            this->SetValue( "AccessionNumber",               studyId); 
            this->SetValue( "InstitutionName",               studyId); 
            this->SetValue( "ReferringPhysicianName",        studyId); 
            this->SetValue( "ReferencedSOPInstanceUID",      studyId); 
            this->SetValue( "PatientName",                   patientId); 
            this->SetValue( "PatientID",                     patientId); 
            this->SetValue( "StudyInstanceUID",              studyId); 
            this->SetValue( "SeriesInstanceUID",             studyId); 
            this->SetValue( "StudyID",                       studyId); 
            this->SetValue( "FrameOfReferenceUID",           studyId); 
            this->SetValue( "BurnedInAnnotation",            studyId); 
            this->SetValue( "UID",                           studyId); 
            this->SetValue( "StorageMediaFileSetUID",        studyId); 
            this->SetValue( "ReferencedFrameOfReferenceUID", studyId); 
            this->SetValue( "RelatedFrameOfReferenceUID",    studyId); 
    }   

    //  These fields are not removed from PHI_LIMITED data sets 
    if ( phiType == svkDcmHeader::PHI_DEIDENTIFIED ) {
            this->SetValue( "StudyDate",                     studyId); 
            this->SetValue( "SeriesDate",                    studyId); 
            this->SetValue( "AcquisitionDate",               studyId); 
            this->SetValue( "ContentDate",                   studyId); 
            this->SetValue( "PatientBirthDate",              patientId); 
    }

    if ( phiType == svkDcmHeader::PHI_DEIDENTIFIED ) {
        this->SetValue( "DeidentificationMethod", "DEIDENTIFIED" ); 
    } else if ( phiType == svkDcmHeader::PHI_LIMITED ) {
        this->SetValue( "DeidentificationMethod", "LIMITED" ); 
    }
}


/*!
 *  Check for magic DICM chars at byte 128 to test if the file is a DICOM file. 
 */
bool svkDcmHeader::IsFileDICOM( vtkstd::string fname)
{
    bool isDICOM = false; 

    try {
        ifstream* file = new ifstream();
        file->exceptions( ifstream::eofbit | ifstream::failbit | ifstream::badbit );

        file->open( fname.c_str(), ios::binary ); 

        // get length of file:
        file->seekg (0, ios::end);
        int fileLength = file->tellg();

        if ( fileLength >= 131 ) {

            file->seekg (0, ios::beg);
            //  try to read if not eof
            file->seekg(128, ios::beg); 

            char magicDICOMChars[5]; 
            file->read( magicDICOMChars, 4); 
            //  terminate
            magicDICOMChars[4] = '\0'; 
   
            vtkstd::string magicString( magicDICOMChars ); 

            if ( magicString.compare("DICM") == 0 ) {
                isDICOM = true;
            }
        }

        file->close(); 

        delete file; 

    } catch (const exception& e) {
        cerr << "ERROR(svkDcmHeader::IsFileDICOM opening or reading file (" << fname << "): " << e.what() << endl;
    }

    return isDICOM;
}


/*!
 *  Gets number of frames.  Since this isn't necessarily present for single frame objects, 
 *  default to 1: 
 */
int svkDcmHeader::GetNumberOfFrames()
{
    int numberOfFrames;
    if ( this->ElementExists("NumberOfFrames") ) {
        numberOfFrames = this->GetIntValue( "NumberOfFrames" );
    } else {
        numberOfFrames = 1;
    }
    return numberOfFrames; 
}

