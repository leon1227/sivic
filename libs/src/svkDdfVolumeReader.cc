/*
 *  Copyright © 2009-2010 The Regents of the University of California.
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


#include <svkDdfVolumeReader.h>
#include <vtkCellData.h>
#include <vtkDebugLeaks.h>
#include <vtkGlobFileNames.h>
#include <vtkSortFileNames.h>
#include <vtkByteSwap.h>

#include <sys/stat.h>


using namespace svk;


vtkCxxRevisionMacro(svkDdfVolumeReader, "$Rev$");
vtkStandardNewMacro(svkDdfVolumeReader);


const vtkstd::string svkDdfVolumeReader::MFG_STRING = "GE MEDICAL SYSTEMS";


/*!
 *
 */
svkDdfVolumeReader::svkDdfVolumeReader()
{

#if VTK_DEBUG_ON
    this->DebugOn();
    vtkDebugLeaks::ConstructClass("svkDdfVolumeReader");
#endif

    vtkDebugMacro( << this->GetClassName() << "::" << this->GetClassName() << "()" );

    this->specData = NULL; 
    this->ddfHdr = NULL; 
    this->numCoils = 1; 
    this->numTimePts = 1;

    // Set the byte ordering, as big-endian.
    this->SetDataByteOrderToBigEndian(); 
}



/*!
 *
 */
svkDdfVolumeReader::~svkDdfVolumeReader()
{

    vtkDebugMacro( << this->GetClassName() << "::~" << this->GetClassName() << "()" );

    if ( this->ddfHdr != NULL )  {
        delete ddfHdr;
        this->ddfHdr = NULL;
    }

    if ( this->specData != NULL )  {
        delete [] specData;
        this->specData = NULL;
    }

}



/*!
 *  Check to see if the extension indicates a UCSF IDF file.  If so, try
 *  to open the file for reading.  If that works, then return a success code.
 *  Return Values: 1 if can read the file, 0 otherwise.
 */
int svkDdfVolumeReader::CanReadFile(const char* fname)
{

    //  If file has an extension then check it:
    vtkstd::string fileExtension = this->GetFileExtension( fname );  
    if( ! fileExtension.empty() ) {

        // Also see "vtkImageReader2::GetFileExtensions method" 
        
        if ( 
            fileExtension.compare( "cmplx" ) == 0 || 
            fileExtension.compare( "ddf" ) == 0 
        )  {
            FILE *fp = fopen(fname, "rb");
            if (fp) {
                fclose(fp);

                vtkDebugMacro(<< this->GetClassName() << "::CanReadFile(): It's a UCSF Complex File: " << fname );
                return 1;
            }
        } else {
            vtkDebugMacro(<< this->GetClassName() << "::CanReadFile(): It's NOT a UCSF Complex File: " << fname );
            return 0;
        }

    } else {
        vtkDebugMacro(<< this->GetClassName() <<"::CanReadFile(): It's NOT a valid file name : " << fname );
        return 0;
    }
}
 

/*!
 *  Not sure if this is always extent 5 if the data is coronal and sagital for example
 */
int svkDdfVolumeReader::GetNumSlices()
{
    return (this->GetDataExtent())[5] + 1;
}


/*!
 *  Side effect of Update() method.  Used to load pixel data and initialize vtkImageData
 *  Called before ExecuteData()
 */
void svkDdfVolumeReader::ExecuteInformation()
{

    vtkDebugMacro( << this->GetClassName() << "::ExecuteInformation()" );

    if (this->FileName == NULL) {
        return;
    }

    if ( this->FileName ) {

        struct stat fs;
        if ( stat(this->FileName, &fs) ) {
            vtkErrorMacro("Unable to open file " << this->FileName );
            return;
        }

        this->InitDcmHeader(); 
        this->SetupOutputInformation();

    }

    //  This is a workaround required since the vtkImageAlgo executive
    //  for the reder resets the Extent[5] value to the number of files
    //  which is not correct for 3D multislice volume files. So store
    //  the files in a temporary array until after ExecuteData has been
    //  called, then reset the array.
    this->tmpFileNames = vtkStringArray::New();
    this->tmpFileNames->DeepCopy(this->FileNames);
    this->FileNames->Delete();
    this->FileNames = NULL;

}


/*!
 *  Side effect of Update() method.  Used to load pixel data and initialize vtkImageData
 *  Called after ExecuteInformation()
 */
void svkDdfVolumeReader::ExecuteData(vtkDataObject* output)
{

    this->FileNames = vtkStringArray::New();
    this->FileNames->DeepCopy(this->tmpFileNames);
    this->tmpFileNames->Delete();
    this->tmpFileNames = NULL;

    vtkDebugMacro( << this->GetClassName() << "::ExecuteData()" );

    svkImageData* data = svkImageData::SafeDownCast( this->AllocateOutputData(output) );

    if ( this->GetFileNames()->GetNumberOfValues() ) {
        vtkDebugMacro( << this->GetClassName() << " FileName: " << FileName );
        struct stat fs;
        if ( stat(this->GetFileNames()->GetValue(0), &fs) ) {
            vtkErrorMacro("Unable to open file " << vtkstd::string(this->GetFileNames()->GetValue(0)) );
            return;
        }
        this->ReadComplexFile(data);
    }

    double dcos[3][3];
    this->GetOutput()->GetDcmHeader()->GetDataDcos( dcos );
    this->GetOutput()->SetDcos(dcos);

    //  SetNumberOfIncrements is supposed to call this, but only works if the data has already
    //  been allocated. but that requires the number of components to be specified.
    this->GetOutput()->GetIncrements();

}


/*!
 *
 */
void svkDdfVolumeReader::ReadComplexFile(vtkImageData* data)
{

    for (int fileIndex = 0; fileIndex < this->GetFileNames()->GetNumberOfValues(); fileIndex++) {

        if (this->GetDebug()) { 
            cout << "read data from file " << fileIndex << " / " << this->GetFileNames()->GetNumberOfValues() << endl;
        }

        int coilNum = 0; 
        if ( this->numCoils > 1 ) {
            coilNum = fileIndex;     
        }

        ifstream* cmplxDataIn = new ifstream();
        cmplxDataIn->exceptions( ifstream::eofbit | ifstream::failbit | ifstream::badbit );

        vtkstd::string cmplxFile = this->GetFileRoot( this->GetFileNames()->GetValue( fileIndex ) ) + ".cmplx"; 

        cmplxDataIn->open( cmplxFile.c_str(), ios::binary);

        //  Read in data from 1 coil:
        int numComponents =  this->GetHeaderValueAsInt( ddfMap, "numberOfComponents" ); 
        int numPts = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints0"); 
        int numBytesInVol = this->GetNumPixelsInVol() * numPts * numComponents * sizeof(float) * this->numTimePts; 
        this->specData = new float[ numBytesInVol/sizeof(float) ];  
        cmplxDataIn->read( (char*)(this->specData), numBytesInVol );

        if ( this->GetSwapBytes() ) {
            vtkByteSwap::SwapVoidRange((void *)this->specData, numBytesInVol/sizeof(float), sizeof(float));
        }

        int numVoxels[3] = { this->GetDataExtent()[1], this->GetDataExtent()[3], this->GetDataExtent()[5] }; 
        int denominator = numVoxels[2] * numVoxels[1]  * numVoxels[0] + numVoxels[1]*numVoxels[0] + numVoxels[0];
        double progress = 0;


        for (int timePt = 0; timePt < this->numTimePts ; timePt++) {
            ostringstream progressStream;
            progressStream <<"Reading Time Point " << timePt+1 << "/"
                           << numTimePts << " and Channel: " << coilNum+1 << "/" << numCoils;
            this->SetProgressText( progressStream.str().c_str() );
            for (int z = 0; z < (this->GetDataExtent())[5] ; z++) {
                for (int y = 0; y < (this->GetDataExtent())[3]; y++) {

                    for (int x = 0; x < (this->GetDataExtent())[1]; x++) {
                        SetCellSpectrum(data, x, y, z, timePt, coilNum);
                    }
                    progress = (((z) * (numVoxels[0]) * (numVoxels[1]) ) + ( (y) * (numVoxels[0]) ))
                                       /((double)denominator);
                    this->UpdateProgress( progress );
                }
            }
        }

        cmplxDataIn->close(); 
        delete cmplxDataIn;
        delete [] specData; 
        specData = NULL;
    }
}


/*!
 *  Utility method returns the total number of Pixels in 3D.
 */
int svkDdfVolumeReader::GetNumPixelsInVol()
{
    return (
        ( (this->GetDataExtent())[1] ) *
        ( (this->GetDataExtent())[3] ) *
        ( (this->GetDataExtent())[5] )
    );
}



/*!
 *
 */
void svkDdfVolumeReader::SetCellSpectrum(vtkImageData* data, int x, int y, int z, int timePt, int coilNum)
{

    //  Set XY points to plot - 2 components per tuble for complex data sets:
    vtkDataArray* dataArray = vtkDataArray::CreateDataArray(VTK_FLOAT);
    dataArray->SetNumberOfComponents(2);

    int numPts = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints0"); 
    int numComponents =  this->GetHeaderValueAsInt( ddfMap, "numberOfComponents" ); 
    dataArray->SetNumberOfTuples(numPts);
    char arrayName[30];

    sprintf(arrayName, "%d %d %d %d %d", x, y, z, timePt, coilNum);
    dataArray->SetName(arrayName);

    //  preallocate float array for spectrum and use to iniitialize the vtkDataArray:
    //  don't do this for each call
    int numVoxels[3]; 
    numVoxels[0] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints1"); 
    numVoxels[1] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints2"); 
    numVoxels[2] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints3"); 

    int offset = (numPts * numComponents) *  (
                     ( numVoxels[0] * numVoxels[1] * numVoxels[2] ) * timePt 
                    +( numVoxels[0] * numVoxels[1] ) * z
                    +  numVoxels[0] * y
                    +  x 
                 ); 

    for (int i = 0; i < numPts; i++) {
        dataArray->SetTuple(i, &(this->specData[offset + (i * 2)]));
    }

    //  Add the spectrum's dataArray to the CellData:
    //  vtkCellData is a subclass of vtkFieldData
    data->GetCellData()->AddArray(dataArray);

    dataArray->Delete();

    return;
}


/*!
 *  Initializes the svkDcmHeader adapter to a specific IOD type      
 *  and initizlizes the svkDcmHeader member of the svkImageData 
 *  object.    
 */
void svkDdfVolumeReader::InitDcmHeader()
{
    vtkDebugMacro( << this->GetClassName() << "::InitDcmHeader()" );

    this->iod = svkMRSIOD::New();
    this->iod->SetDcmHeader( this->GetOutput()->GetDcmHeader());
    this->iod->InitDcmHeader();

    this->ParseDdf(); 
    this->PrintKeyValuePairs();

    //  Fill in data set specific values:
    this->InitPatientModule(); 
    this->InitGeneralStudyModule(); 
    this->InitGeneralSeriesModule(); 
    this->InitGeneralEquipmentModule(); 
    this->InitMultiFrameFunctionalGroupsModule();
    this->InitMultiFrameDimensionModule();
    this->InitAcquisitionContextModule();
    this->InitMRSpectroscopyModule(); 
    this->InitMRSpectroscopyPulseSequenceModule(); 
    this->InitMRSpectroscopyDataModule(); 

    if (this->GetDebug()) { 
        this->GetOutput()->GetDcmHeader()->PrintDcmHeader();
    }

    this->iod->Delete();
}


/*!
 *  Read DDF header fields into a string STL map for use during initialization
 *  of DICOM header by Init*Module methods.
 */
void svkDdfVolumeReader::ParseDdf()
{

    this->GlobFileNames();


    try {

        //  Read in the DDF Header:
        this->ddfHdr = new ifstream();
        this->ddfHdr->exceptions( ifstream::eofbit | ifstream::failbit | ifstream::badbit );

        int fileIndex = 0; 
        vtkstd::string currentDdfFileName = this->GetFileRoot( this->GetFileNames()->GetValue( fileIndex ) ) + ".ddf";

        this->ddfHdr->open( currentDdfFileName.c_str(), ifstream::in );

        if ( ! this->ddfHdr->is_open() ) {
            throw runtime_error( "Could not open volume file: " + string( this->GetFileName() ) );
        }
        istringstream* iss = new istringstream();

        //  Skip first line: 
        this->ReadLine(this->ddfHdr, iss);

        // DDF_VERSION
        int ddfVersion;
        this->ReadLine(this->ddfHdr, iss);
        iss->ignore(29);
        *iss>>ddfVersion;

        ddfMap["objectType"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["patientId"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["patientName"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["patientCode"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["dateOfBirth"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["sex"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["studyId"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["studyCode"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["studyDate"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["accessionNumber"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["rootName"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["seriesNumber"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["seriesDescription"] = this->StripWhite( this->ReadLineValue( this->ddfHdr, iss, ':') );
        ddfMap["comment"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["patientEntry"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["patientPosition"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["orientation"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["dataType"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["numberOfComponents"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["sourceDescription"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["numberOfDimensions"] = this->ReadLineValue( this->ddfHdr, iss, ':');
 
        // DIMENSIONS AND SPACING
        int numDimensions = this->GetHeaderValueAsInt(ddfMap, "numberOfDimensions"); 

        for (int i = 0; i < numDimensions; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());

            //  Type
            vtkstd::string dimensionTypeString = "dimensionType" + indexString; 
            vtkstd::string dimensionLine( this->ReadLineSubstr(this->ddfHdr, iss, 0, 100) );
            size_t start = dimensionLine.find( "type:" ); 
            vtkstd::string tmp   = dimensionLine.substr( start + 6 ); 
            size_t end   = tmp.find( "npoints" ); 
            ddfMap[dimensionTypeString] = dimensionLine.substr( start + 6, end - 1); 

            // npoints 
            vtkstd::string nptsString = "dimensionNumberOfPoints" + indexString; 
            start = dimensionLine.find( "npoints:" ); 
            start += 8; 
            end   = dimensionLine.find( "pixel" ); 
            ddfMap[nptsString] = dimensionLine.substr( start , end - start ); 

            //  PixelSpacing
            vtkstd::string pixelSpacingString = "pixelSpacing" + indexString; 
            start = dimensionLine.find( "(mm):" ); 
            if (start != vtkstd::string::npos) {
                ddfMap[pixelSpacingString] = dimensionLine.substr( start + 5 ); 
            }
        }

        //  CENTER(LPS):
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("centerLPS" + indexString)];
        }

        //  TOPLC(LPS):
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("toplcLPS" + indexString)];
        }

        //  DCOS:  
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndexI;
            ossIndexI << i;
            vtkstd::string indexStringI(ossIndexI.str());
            this->ReadLineIgnore( this->ddfHdr, iss, 's' );
            for (int j = 0; j < 3; j++) {
                ostringstream ossIndexJ;
                ossIndexJ << j;
                vtkstd::string indexStringJ(ossIndexJ.str());
                iss->ignore(256, ' ');
                *iss >> ddfMap[vtkstd::string("dcos" + indexStringI + indexStringJ)];
            }
        }

        //MR Parameters 
        this->ReadLine(this->ddfHdr, iss);
        this->ReadLine(this->ddfHdr, iss);
        
        ddfMap["coilName"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["sliceGap"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["echoTime"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["repetitionTime"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["inversionTime"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["flipAngle"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["pulseSequenceName"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["transmitterFrequency"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["isotope"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["fieldStrength"] = this->ReadLineValue( this->ddfHdr, iss, ':');

        ddfMap["numberOfSatBands"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        for (int i = 0; i < this->GetHeaderValueAsInt( ddfMap, "numberOfSatBands" ); i++) {

            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            ddfMap["satBand" + indexString + "Thickness"] = this->ReadLineValue( this->ddfHdr, iss, ':');

            //  Orientation:
            this->ReadLineIgnore( this->ddfHdr, iss, 'r' );
            for (int j = 0; j < 3; j++) {
                ostringstream ossIndexJ;
                ossIndexJ << j;
                vtkstd::string indexStringJ(ossIndexJ.str());
                iss->ignore(256, ' ');
                *iss >> ddfMap[vtkstd::string("satBand" + indexString + "Orientation" + indexStringJ)];
            }

            //  position:
            this->ReadLineIgnore( this->ddfHdr, iss, ')' );
            for (int j = 0; j < 3; j++) {
                ostringstream ossIndexJ;
                ossIndexJ << j;
                vtkstd::string indexStringJ(ossIndexJ.str());
                iss->ignore(256, ' ');
                *iss >> ddfMap[vtkstd::string("satBand" + indexString + "PositionLPS" + indexStringJ)];
            }

        }

        //Spectroscopy Parameters 
        this->ReadLine(this->ddfHdr, iss);
        this->ReadLine(this->ddfHdr, iss);
        ddfMap["localizationType"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["centerFrequency"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["ppmReference"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["sweepwidth"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["dwelltime"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["frequencyOffset"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["centeredOnWater"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["suppressionTechnique"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["residualWater"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["numberOfAcquisitions"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["chop"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["evenSymmetry"] = this->ReadLineValue( this->ddfHdr, iss, ':');
        ddfMap["dataReordered"] = this->ReadLineValue( this->ddfHdr, iss, ':');

        //  ACQ TOPLC(LPS):
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("acqToplcLPS" + indexString)];
        }

        //  ACQ Spacing:
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("acqSpacing" + indexString)];
        }

        ddfMap["acqNumberOfDataPoints"] = this->ReadLineValue( this->ddfHdr, iss, ':');

        //  Acq Num Data Pts:
        this->ReadLineIgnore( this->ddfHdr, iss, 's' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("acqNumberOfPoints" + indexString)];
        }

        //  ACQ DCOS:  
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndexI;
            ossIndexI << i;
            vtkstd::string indexStringI(ossIndexI.str());
            this->ReadLineIgnore( this->ddfHdr, iss, 's' );
            for (int j = 0; j < 3; j++) {
                ostringstream ossIndexJ;
                ossIndexJ << j;
                vtkstd::string indexStringJ(ossIndexJ.str());
                iss->ignore(256, ' ');
                *iss >> ddfMap[vtkstd::string("acqDcos" + indexStringI + indexStringJ)];
            }
        }

        //  Selection Center:
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("selectionCenterLPS" + indexString)];
        }

        //  Selection Size:
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("selectionSize" + indexString)];
        }

        //  Selection DCOS:  
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndexI;
            ossIndexI << i;
            vtkstd::string indexStringI(ossIndexI.str());
            this->ReadLineIgnore( this->ddfHdr, iss, 'd' );
            for (int j = 0; j < 3; j++) {
                ostringstream ossIndexJ;
                ossIndexJ << j;
                vtkstd::string indexStringJ(ossIndexJ.str());
                iss->ignore(256, ' ');
                *iss >> ddfMap[vtkstd::string("selectionDcos" + indexStringI + indexStringJ)];
            }
        }

        //  Reordered :
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("reorderedToplcLPS" + indexString)];
        }

        //  Reordered:
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("reorderedCenterLPS" + indexString)];
        }

        //  Reordered:
        this->ReadLineIgnore( this->ddfHdr, iss, ')' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("reorderedSpacing" + indexString)];
        }

        //  Reordered:
        this->ReadLineIgnore( this->ddfHdr, iss, 's' );
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());
            iss->ignore(256, ' ');
            *iss >> ddfMap[vtkstd::string("reorderedNumberOfPoints" + indexString)];
        }

        //  Selection DCOS:  
        for (int i = 0; i < 3; i++) {
            ostringstream ossIndexI;
            ossIndexI << i;
            vtkstd::string indexStringI(ossIndexI.str());
            this->ReadLineIgnore( this->ddfHdr, iss, 's' );
            for (int j = 0; j < 3; j++) {
                ostringstream ossIndexJ;
                ossIndexJ << j;
                vtkstd::string indexStringJ(ossIndexJ.str());
                iss->ignore(256, ' ');
                *iss >> ddfMap[vtkstd::string("reorderedDcos" + indexStringI + indexStringJ)];
            }
        }

        delete iss;

    } catch (const exception& e) {
        cerr << "ERROR opening or reading ddf file (" << this->GetFileName() << "): " << e.what() << endl;
    }

}



/*!
 *  Initializes the VolumeLocalizationSequence in the MRSpectroscopy
 *  DICOM object for PRESS excitation.  
 */
void svkDdfVolumeReader::InitVolumeLocalizationSeq()
{

    //  Get Thickness Values
    float selBoxSize[3]; 
    selBoxSize[0] = this->GetHeaderValueAsFloat(ddfMap, "selectionSize0"); 
    selBoxSize[1] = this->GetHeaderValueAsFloat(ddfMap, "selectionSize1"); 
    selBoxSize[2] = this->GetHeaderValueAsFloat(ddfMap, "selectionSize2"); 

    //  Get Center Location Values
    float selBoxCenter[3]; 
    selBoxCenter[0] = this->GetHeaderValueAsFloat(ddfMap, "selectionCenterLPS0"); 
    selBoxCenter[1] = this->GetHeaderValueAsFloat(ddfMap, "selectionCenterLPS1"); 
    selBoxCenter[2] = this->GetHeaderValueAsFloat(ddfMap, "selectionCenterLPS2"); 

    //  Get Orientation Values 
    float selBoxOrientation[3][3]; 
    selBoxOrientation[0][0] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos00"); 
    selBoxOrientation[0][1] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos01"); 
    selBoxOrientation[0][2] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos02"); 
    selBoxOrientation[1][0] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos10"); 
    selBoxOrientation[1][1] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos11"); 
    selBoxOrientation[1][2] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos12"); 
    selBoxOrientation[2][0] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos20"); 
    selBoxOrientation[2][1] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos21"); 
    selBoxOrientation[2][2] = this->GetHeaderValueAsFloat(ddfMap, "selectionDcos22"); 

    this->GetOutput()->GetDcmHeader()->InitVolumeLocalizationSeq(
        selBoxSize, 
        selBoxCenter, 
        selBoxOrientation
    ); 

}


/*!
 *
 */
void svkDdfVolumeReader::InitPatientModule() 
{

    this->GetOutput()->GetDcmHeader()->InitPatientModule(
        this->GetOutput()->GetDcmHeader()->GetDcmPatientsName(  ddfMap["patientName"] ),  
        ddfMap["patientId"], 
        ddfMap["dateOfBirth"], 
        ddfMap["sex"] 
    );

}


/*!
 *
 */
void svkDdfVolumeReader::InitGeneralStudyModule() 
{
    this->GetOutput()->GetDcmHeader()->InitGeneralStudyModule(
        this->RemoveSlashesFromDate( &(ddfMap["studyDate"]) ), 
        "", 
        "", 
        ddfMap["studyId"], 
        ddfMap["accessionNumber"], 
        ""
    );
}


/*!
 *
 */
void svkDdfVolumeReader::InitGeneralSeriesModule() 
{

    vtkstd::string patientEntryPos; 
    vtkstd::string patientEntry( ddfMap["patientEntry"]); 
    if ( patientEntry.compare("head first") == 0) {
        patientEntryPos = "HF";
    } else if ( patientEntry.compare("feet first") == 0) {
        patientEntryPos = "FF";
    }

    vtkstd::string patientPosition( ddfMap["patientPosition"]); 
    if ( patientPosition.compare("supine") == 0 ) {
        patientEntryPos.append("S");
    } else if ( patientPosition.compare("prone") == 0 ) {
        patientEntryPos.append("P");
    }

    this->GetOutput()->GetDcmHeader()->InitGeneralSeriesModule(
        ddfMap["seriesNumber"], 
        ddfMap["seriesDescription"], 
        patientEntryPos 
    );

}


/*!
 *  DDF is historically the UCSF representation of a GE raw file so 
 *  initialize to svkDdfVolumeReader::MFG_STRING.
 */
void svkDdfVolumeReader::InitGeneralEquipmentModule()
{
    this->GetOutput()->GetDcmHeader()->SetValue(
        "Manufacturer", 
        svkDdfVolumeReader::MFG_STRING 
    );
}


/*!
 *
 */
void svkDdfVolumeReader::InitMultiFrameFunctionalGroupsModule()
{
    InitSharedFunctionalGroupMacros();

    this->GetOutput()->GetDcmHeader()->SetValue( 
        "InstanceNumber", 
        1 
    );

    this->GetOutput()->GetDcmHeader()->SetValue( 
        "ContentDate", 
        this->RemoveSlashesFromDate( &(ddfMap["studyDate"]) ) 
    );

    int numVoxels[3];
    this->numSlices = this->GetHeaderValueAsInt( ddfMap, "dimensionNumberOfPoints3" ); 

    if ( this->GetHeaderValueAsInt( ddfMap, "numberOfDimensions" ) == 5 ) {
        this->numTimePts = this->GetHeaderValueAsInt( ddfMap, "dimensionNumberOfPoints4" ); 
    }

    this->numCoils = this->GetFileNames()->GetNumberOfValues();

    this->GetOutput()->GetDcmHeader()->SetValue( 
        "NumberOfFrames", 
        this->numSlices * this->numCoils * this->numTimePts
    );

    InitPerFrameFunctionalGroupMacros();
}


/*!
 *
 */
void svkDdfVolumeReader::InitSharedFunctionalGroupMacros()
{

    this->InitPixelMeasuresMacro();
    this->InitPlaneOrientationMacro();
    this->InitMRTimingAndRelatedParametersMacro();
    this->InitMRSpectroscopyFOVGeometryMacro();
    this->InitMREchoMacro();
    this->InitMRModifierMacro();
    this->InitMRReceiveCoilMacro();
    this->InitMRTransmitCoilMacro();
    this->InitMRAveragesMacro();
    this->InitMRSpatialSaturationMacro();
    //this->InitMRSpatialVelocityEncodingMacro();
}


/*!
 *
 */
void svkDdfVolumeReader::InitPerFrameFunctionalGroupMacros()
{
    this->InitFrameContentMacro();
    this->InitPlanePositionMacro();
}


/*!
 *  Pixel Spacing:
 */
void svkDdfVolumeReader::InitPixelMeasuresMacro()
{

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
        "SharedFunctionalGroupsSequence",
        0, 
        "PixelMeasuresSequence"
    );

    //  get the spacing for the specified index:
    float colSpacing = this->GetHeaderValueAsFloat(ddfMap, "pixelSpacing1"); 
    float rowSpacing = this->GetHeaderValueAsFloat(ddfMap, "pixelSpacing2"); 

    vtkstd::string pixelSpacing;
    ostringstream oss;
    oss << colSpacing;
    oss << '\\';
    oss << rowSpacing;
    pixelSpacing = oss.str();

    this->GetOutput()->GetDcmHeader()->InitPixelMeasuresMacro(
        pixelSpacing,
        ddfMap["pixelSpacing3"] 
    );
}


/*!
 *  Mandatory, Must be a per-frame functional group
 */
void svkDdfVolumeReader::InitFrameContentMacro()
{

    int numFrameIndices = svkDcmHeader::GetNumberOfDimensionIndices( this->numTimePts, this->numCoils ) ;

    unsigned int* indexValues = new unsigned int[numFrameIndices]; 

    int frame = 0; 

    for (int coilNum = 0; coilNum < numCoils; coilNum++) {

        for (int timePt = 0; timePt < numTimePts; timePt++) {

            for (int sliceNum = 0; sliceNum < this->numSlices; sliceNum++) {

                svkDcmHeader::SetDimensionIndices(
                    indexValues, numFrameIndices, sliceNum, timePt, coilNum, numTimePts, numCoils
                );

                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
                    "PerFrameFunctionalGroupsSequence",
                    frame, 
                    "FrameContentSequence"
                );

                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "DimensionIndexValues", 
                    indexValues,        //  array of vals
                    numFrameIndices,    // num values in array
                    "PerFrameFunctionalGroupsSequence",
                    frame
                );

                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "FrameAcquisitionDateTime",
                    "EMPTY_ELEMENT",
                    "PerFrameFunctionalGroupsSequence",
                    frame 
                );
        
                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                    "FrameContentSequence",
                    0,
                    "FrameReferenceDateTime",
                    "EMPTY_ELEMENT",
                    "PerFrameFunctionalGroupsSequence",
                    frame 
                );
    
                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
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
 *
 */
void svkDdfVolumeReader::InitPlanePositionMacro()
{


    //  Get toplc float array from ddfMap and use that to generate
    //  frame locations.  This position is off by 1/2 voxel, fixed below:
    float toplc[3];
    toplc[0] = this->GetHeaderValueAsFloat(ddfMap, "toplcLPS0"); 
    toplc[1] = this->GetHeaderValueAsFloat(ddfMap, "toplcLPS1"); 
    toplc[2] = this->GetHeaderValueAsFloat(ddfMap, "toplcLPS2"); 
        
    float dcos[3][3];
    dcos[0][0] = this->GetHeaderValueAsFloat(ddfMap, "dcos00"); 
    dcos[0][1] = this->GetHeaderValueAsFloat(ddfMap, "dcos01"); 
    dcos[0][2] = this->GetHeaderValueAsFloat(ddfMap, "dcos02"); 
    dcos[1][0] = this->GetHeaderValueAsFloat(ddfMap, "dcos10"); 
    dcos[1][1] = this->GetHeaderValueAsFloat(ddfMap, "dcos11"); 
    dcos[1][2] = this->GetHeaderValueAsFloat(ddfMap, "dcos12"); 
    dcos[2][0] = this->GetHeaderValueAsFloat(ddfMap, "dcos20"); 
    dcos[2][1] = this->GetHeaderValueAsFloat(ddfMap, "dcos21"); 
    dcos[2][2] = this->GetHeaderValueAsFloat(ddfMap, "dcos22"); 
    
    float pixelSize[3];
    pixelSize[0] = this->GetHeaderValueAsFloat(ddfMap, "pixelSpacing1"); 
    pixelSize[1] = this->GetHeaderValueAsFloat(ddfMap, "pixelSpacing2"); 
    pixelSize[2] = this->GetHeaderValueAsFloat(ddfMap, "pixelSpacing3"); 

    float displacement[3]; 
    float frameLPSPosition[3]; 

    int frame = 0; 

    for (int coilNum = 0; coilNum < this->numCoils; coilNum++) {

        for (int timePt = 0; timePt < this->numTimePts; timePt++) {
    
            for (int sliceNum = 0; sliceNum < this->numSlices; sliceNum++) {
    
                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
                    "PerFrameFunctionalGroupsSequence",
                    sliceNum, 
                    "PlanePositionSequence"
                );
    
                //add displacement along normal vector:
                for (int j = 0; j < 3; j++) {
                    displacement[j] = dcos[2][j] * pixelSize[2] * sliceNum;
                }
                for(int j = 0; j < 3; j++) { //L, P, S
                    frameLPSPosition[j] = toplc[j] +  displacement[j] ;
                }
    
                vtkstd::string imagePositionPatient;
                for (int j = 0; j < 3; j++) {
                    ostringstream oss;
                    oss.precision(8);
                    oss << frameLPSPosition[j];
                    imagePositionPatient += oss.str();
                    if (j < 2) {
                        imagePositionPatient += '\\';
                    }
                }
    
                this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
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
 *
 */
void svkDdfVolumeReader::InitPlaneOrientationMacro()
{
    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
        "SharedFunctionalGroupsSequence",
        0, 
        "PlaneOrientationSequence"
    );

    float dcos[3][3]; 
    dcos[0][0] = this->GetHeaderValueAsFloat(ddfMap, "dcos00"); 
    dcos[0][1] = this->GetHeaderValueAsFloat(ddfMap, "dcos01"); 
    dcos[0][2] = this->GetHeaderValueAsFloat(ddfMap, "dcos02"); 
    dcos[1][0] = this->GetHeaderValueAsFloat(ddfMap, "dcos10"); 
    dcos[1][1] = this->GetHeaderValueAsFloat(ddfMap, "dcos11"); 
    dcos[1][2] = this->GetHeaderValueAsFloat(ddfMap, "dcos12"); 
    dcos[2][0] = this->GetHeaderValueAsFloat(ddfMap, "dcos20"); 
    dcos[2][1] = this->GetHeaderValueAsFloat(ddfMap, "dcos21"); 
    dcos[2][2] = this->GetHeaderValueAsFloat(ddfMap, "dcos22"); 

    ostringstream ossDcos;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            ossDcos << dcos[i][j];
            if (i * j != 2) {
                ossDcos<< "\\";
            }
        }
    }

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "PlaneOrientationSequence",         
        0,                                 
        "ImageOrientationPatient",        
        ossDcos.str(), 
        "SharedFunctionalGroupsSequence",
        0                               
    );


    //  Determine whether the data is ordered with or against the slice normal direction.
    double normal[3]; 
    this->GetOutput()->GetDcmHeader()->GetNormalVector(normal); 

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
    this->GetOutput()->GetDcmHeader()->SetSliceOrder( this->dataSliceOrder );
    math->Delete();

}


/*!
 *
 */
void svkDdfVolumeReader::InitMRTimingAndRelatedParametersMacro()
{
    this->GetOutput()->GetDcmHeader()->InitMRTimingAndRelatedParametersMacro(
        this->GetHeaderValueAsFloat(ddfMap, "repetitionTime"), 
        this->GetHeaderValueAsFloat(ddfMap, "flipAngle")
    ); 
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRSpectroscopyFOVGeometryMacro()
{

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
        "SharedFunctionalGroupsSequence",
        0, 
        "MRSpectroscopyFOVGeometrySequence"
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence", 
        0,                                  
        "SpectroscopyAcquisitionDataColumns", 
        this->GetHeaderValueAsInt( ddfMap, "acqNumberOfDataPoints"),
        "SharedFunctionalGroupsSequence",      
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SpectroscopyAcquisitionPhaseColumns",
        this->GetHeaderValueAsInt( ddfMap, "acqNumberOfPoints0"),
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SpectroscopyAcquisitionPhaseRows",
        this->GetHeaderValueAsInt( ddfMap, "acqNumberOfPoints1"),
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SpectroscopyAcquisitionOutOfPlanePhaseSteps",
        this->GetHeaderValueAsInt( ddfMap, "acqNumberOfPoints2"),
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcquisitionTLC",
        ddfMap["acqToplcLPS0"] + '\\' + ddfMap["acqToplcLPS1"] + '\\' + ddfMap["acqToplcLPS2"],
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcquisitionPixelSpacing",
        ddfMap["acqSpacing0"] + '\\' + ddfMap["acqSpacing1"], 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcquisitionSliceThickness",
        ddfMap["acqSpacing2"], 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcquisitionOrientation",
        ddfMap["acqDcos00"] + '\\' + ddfMap["acqDcos01"] + '\\' + ddfMap["acqDcos02"] + '\\' + 
        ddfMap["acqDcos10"] + '\\' + ddfMap["acqDcos11"] + '\\' + ddfMap["acqDcos12"] + '\\' + 
        ddfMap["acqDcos20"] + '\\' + ddfMap["acqDcos21"] + '\\' + ddfMap["acqDcos22"], 
        "SharedFunctionalGroupsSequence",    
        0
    );


    // ==================================================
    //  Reordered Params
    // ==================================================
    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedPhaseColumns",
        this->GetHeaderValueAsInt( ddfMap, "reorderedNumberOfPoints0" ), 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedPhaseRows",
        this->GetHeaderValueAsInt( ddfMap, "reorderedNumberOfPoints1" ), 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedOutOfPlanePhaseSteps",
        this->GetHeaderValueAsInt( ddfMap, "reorderedNumberOfPoints2"), 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedTLC",
        ddfMap["reorderedToplcLPS0"] + '\\' + ddfMap["reorderedToplcLPS1"] + '\\' + ddfMap["reorderedToplcLPS2"],
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedPixelSpacing",
        ddfMap["reorderedSpacing0"] + '\\' + ddfMap["reorderedSpacing1"],
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedSliceThickness",
        ddfMap["reorderedSpacing2"], 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "SVK_SpectroscopyAcqReorderedOrientation",
        ddfMap["reorderedDcos00"] + '\\' + ddfMap["reorderedDcos01"] + '\\' + ddfMap["reorderedDcos02"] + '\\' + 
        ddfMap["reorderedDcos10"] + '\\' + ddfMap["reorderedDcos11"] + '\\' + ddfMap["reorderedDcos12"] + '\\' + 
        ddfMap["reorderedDcos20"] + '\\' + ddfMap["reorderedDcos21"] + '\\' + ddfMap["reorderedDcos22"], 
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "PercentSampling",
        1,
        "SharedFunctionalGroupsSequence",    
        0
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                      
        "PercentPhaseFieldOfView",
        1,
        "SharedFunctionalGroupsSequence",    
        0
    );

}


/*!
 *
 */
void svkDdfVolumeReader::InitMREchoMacro()
{
    this->GetOutput()->GetDcmHeader()->InitMREchoMacro( this->GetHeaderValueAsFloat(ddfMap, "echoTime") );
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRModifierMacro()
{
    float inversionTime = this->GetHeaderValueAsFloat( ddfMap, "inversionTime");
    this->GetOutput()->GetDcmHeader()->InitMRModifierMacro( inversionTime );
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRReceiveCoilMacro()
{
    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
        "SharedFunctionalGroupsSequence",
        0, 
        "MRReceiveCoilSequence"
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRReceiveCoilSequence",
        0,                        
        "ReceiveCoilName",       
        ddfMap["coilName"], 
        "SharedFunctionalGroupsSequence",    
        0                      
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRReceiveCoilSequence",
        0,                        
        "ReceiveCoilManufacturerName",       
        vtkstd::string("GE"),
        "SharedFunctionalGroupsSequence",    
        0                      
    );

    vtkstd::string coilType("VOLUME"); 
    if ( this->IsMultiCoil() ) {
        coilType.assign("MULTICOIL"); 
    }

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRReceiveCoilSequence",
        0,                        
        "ReceiveCoilType",       
        coilType, 
        "SharedFunctionalGroupsSequence",    
        0                      
    );

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "MRReceiveCoilSequence",
        0,                        
        "QuadratureReceiveCoil",       
        vtkstd::string("YES"),
        "SharedFunctionalGroupsSequence",    
        0                      
    );

    //=====================================================
    //  Multi Coil Sequence
    //=====================================================
    if ( strcmp(coilType.c_str(), "MULTICOIL") == 0)  { 

        this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
            "MRReceiveCoilSequence",
            0, 
            "MultiCoilDefinitionSequence"
        );

        /*
         *  If multi-channel coil, assume each file corresponds to an individual channel:
         */
        for (int coilIndex = 0; coilIndex < this->numCoils; coilIndex++) {

            ostringstream ossIndex;
            ossIndex << coilIndex;
            vtkstd::string indexString(ossIndex.str());

            this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                "MultiCoilDefinitionSequence",
                coilIndex,                        
                "MultiCoilElementName",       
                coilIndex, //"9", //indexString, 
                "MRReceiveCoilSequence",
                0                      
            );


            this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                "MultiCoilDefinitionSequence",
                coilIndex,                        
                "MultiCoilElementUsed",       
                "YES", 
                "MRReceiveCoilSequence",
                0                      
            );
        }

    }

}


/*!
 *
 */
void svkDdfVolumeReader::InitMRTransmitCoilMacro()
{
    this->GetOutput()->GetDcmHeader()->InitMRTransmitCoilMacro("GE", "UNKNOWN", "BODY");
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRAveragesMacro()
{
    int numAverages = 1; 
    this->GetOutput()->GetDcmHeader()->InitMRAveragesMacro(numAverages);
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRSpatialSaturationMacro()
{

    int numSatBands =  this->GetHeaderValueAsInt( ddfMap, "numberOfSatBands" ); 

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement( 
        "SharedFunctionalGroupsSequence",
        0, 
        "MRSpatialSaturationSequence"
    );

    if (numSatBands > 0) {

        for (int i=0; i < numSatBands; i++) {

            ostringstream ossIndex;
            ossIndex << i;
            vtkstd::string indexString(ossIndex.str());

            this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                "MRSpatialSaturationSequence",
                i,                        
                "SlabThickness",       
                this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "Thickness"), 
                "SharedFunctionalGroupsSequence",    
                0
            );
        
            float orientation[3];
            orientation[0] = this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "Orientation0");     
            orientation[1] = this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "Orientation1");     
            orientation[2] = this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "Orientation2");     

            vtkstd::string slabOrientation;
            for (int j = 0; j < 3; j++) {
                ostringstream ossOrientation;
                ossOrientation << orientation[j];
                slabOrientation += ossOrientation.str();
                if (j < 2) {
                    slabOrientation += '\\';
                }
            }
            this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                "MRSpatialSaturationSequence",
                i,                        
                "SlabOrientation",       
                slabOrientation,
                "SharedFunctionalGroupsSequence",    
                0                      
            );
        
            float position[3];
            position[0] = this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "PositionLPS0");     
            position[1] = this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "PositionLPS1");     
            position[2] = this->GetHeaderValueAsFloat(ddfMap, "satBand" + indexString + "PositionLPS2");     

            vtkstd::string slabPosition;
            for (int j = 0; j < 3; j++) {
                ostringstream ossPosition;
                ossPosition << position[j];
                slabPosition += ossPosition.str();
                if (j < 2) {
                    slabPosition += '\\';
                }
            }
            this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
                "MRSpatialSaturationSequence",
                i,                        
                "MidSlabPosition",       
                slabPosition,
                "SharedFunctionalGroupsSequence",    
                0                      
            );

        }
    }
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRSpatialVelocityEncodingMacro()
{
}


/*!
 *
 */
void svkDdfVolumeReader::InitMultiFrameDimensionModule()
{

    int indexCount = 0;

    this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
        "DimensionIndexSequence",
        indexCount,
        "DimensionDescriptionLabel",
        "Slice"
    );

    if (this->numTimePts > 1) {
        indexCount++;
        this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
            "DimensionIndexSequence",
            indexCount,
            "DimensionDescriptionLabel",
            "Time Point"
        );
    }

    if (this->numCoils > 1) {
        indexCount++;
        this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
            "DimensionIndexSequence",
            indexCount,
            "DimensionDescriptionLabel",
            "Coil Number"
        );
    }

/*
    if ( this->IsMultiCoil() ) {
        this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
            "DimensionIndexSequence",
            1,
            "DimensionIndexPointer",
            "18H\\00H\\47H\\90"
        );
        this->GetOutput()->GetDcmHeader()->AddSequenceItemElement(
            "DimensionIndexSequence",
            1,
            "FunctionalGroupPointer",
            //"MultiCoilDefinitionSequence"
            "18H\\00H\\47H\\90"
        );
    }
*/
}


/*!
 *
 */
void svkDdfVolumeReader::InitAcquisitionContextModule()
{
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRSpectroscopyModule()
{

    /*  =======================================
     *  MR Image and Spectroscopy Instance Macro
     *  ======================================= */

    this->GetOutput()->GetDcmHeader()->SetValue(
        "AcquisitionDatetime",
        this->RemoveSlashesFromDate( &(ddfMap["studyDate"]) ) + "000000"
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "AcquisitionDuration",
        0
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "ResonantNucleus", 
        ddfMap["isotope"]
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "KSpaceFiltering", 
        "NONE" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "ApplicableSafetyStandardAgency", 
        "Research" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "MagneticFieldStrength", 
        this->GetHeaderValueAsFloat(ddfMap, "fieldStrength")
    );
    /*  =======================================
     *  END: MR Image and Spectroscopy Instance Macro
     *  ======================================= */

    this->GetOutput()->GetDcmHeader()->SetValue(
        "ImageType", 
        vtkstd::string("ORIGINAL\\PRIMARY\\SPECTROSCOPY\\NONE") 
    );


    /*  =======================================
     *  Spectroscopy Description Macro
     *  ======================================= */
    this->GetOutput()->GetDcmHeader()->SetValue(
        "VolumetricProperties", 
        vtkstd::string("VOLUME")  
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "VolumeBasedCalculationTechnique", 
        vtkstd::string("NONE")
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "ComplexImageComponent", 
        vtkstd::string("COMPLEX")  
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "AcquisitionContrast", 
        "UNKNOWN"  
    );
    /*  =======================================
     *  END: Spectroscopy Description Macro
     *  ======================================= */


    this->GetOutput()->GetDcmHeader()->SetValue(
        "TransmitterFrequency", 
        this->GetHeaderValueAsFloat(ddfMap, "transmitterFrequency")
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SpectralWidth", 
        this->GetHeaderValueAsFloat(ddfMap, "sweepwidth")
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SVK_FrequencyOffset",
        this->GetHeaderValueAsFloat( ddfMap, "frequencyOffset" )
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "ChemicalShiftReference", 
        this->GetHeaderValueAsFloat( ddfMap, "ppmReference" )
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "VolumeLocalizationTechnique", 
        ddfMap["localizationType"]
    );

    if ( strcmp(ddfMap["localizationType"].c_str(), "PRESS") == 0)  { 
        this->InitVolumeLocalizationSeq();
    }

    this->GetOutput()->GetDcmHeader()->SetValue(
        "Decoupling", 
        "NO" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "TimeDomainFiltering", 
        "NONE" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "NumberOfZeroFills", 
        0 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "BaselineCorrection", 
        vtkstd::string("NONE")
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "FrequencyCorrection", 
        "NO"
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "FirstOrderPhaseCorrection", 
        vtkstd::string("NO")
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "WaterReferencedPhaseCorrection", 
        vtkstd::string("NO")
    );
}


/*!
 *
 */
void svkDdfVolumeReader::InitMRSpectroscopyPulseSequenceModule()
{
    this->GetOutput()->GetDcmHeader()->SetValue(
        "PulseSequenceName", 
        ddfMap["pulseSequenceName"]
    );

    int numVoxels[3];
    numVoxels[0] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints1"); 
    numVoxels[1] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints2"); 
    numVoxels[2] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints3"); 

    vtkstd::string acqType = "VOLUME"; 
    if (numVoxels[0] == 1 && numVoxels[1] == 1 &&  numVoxels[2] == 1) {
        acqType = "SINGLE VOXEL";
    }

    this->GetOutput()->GetDcmHeader()->SetValue(
        "MRSpectroscopyAcquisitionType", 
        acqType 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "EchoPulseSequence", 
        "SPIN" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "MultipleSpinEcho", 
        "NO" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "MultiPlanarExcitation", 
        "NO" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SteadyStatePulseSequence", 
        "NONE" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "EchoPlanarPulseSequence", 
        "NO" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SpectrallySelectedSuppression", 
        ddfMap["suppressionTechnique"]
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "GeometryOfKSpaceTraversal", 
        "RECTILINEAR" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue( 
        "RectilinearPhaseEncodeReordering",
        "LINEAR"
    );

    this->GetOutput()->GetDcmHeader()->SetValue( 
        "SegmentedKSpaceTraversal", 
        "SINGLE"
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "CoverageOfKSpace", 
        "FULL" 
    );

    this->GetOutput()->GetDcmHeader()->SetValue( "NumberOfKSpaceTrajectories", 1 );

    //  Assume EVEN, if evenSymmetry is not "yes" then set to ODD
    string kSpaceSymmetry = "EVEN";
    if ( ( this->ddfMap[ "evenSymmetry" ] ).compare("yes") != 0 ) {
        kSpaceSymmetry = "ODD";
    } 
    this->GetOutput()->GetDcmHeader()->SetValue( "SVK_KSpaceSymmetry", kSpaceSymmetry );

    string chop = "no";
    if ( (this->ddfMap[ "chop" ] ).compare("yes") == 0 ) {
        chop = "YES";
    }
    this->GetOutput()->GetDcmHeader()->SetValue( 
        "SVK_AcquisitionChop", 
        chop 
    );

}


/*!
 *
 */
void svkDdfVolumeReader::InitMRSpectroscopyDataModule()
{
    int numVoxels[3];
    numVoxels[0] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints1"); 
    numVoxels[1] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints2"); 
    numVoxels[2] = this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints3"); 

    this->GetOutput()->GetDcmHeader()->SetValue(
        "Rows", 
        numVoxels[1]
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "Columns", 
        numVoxels[0]
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "DataPointRows", 
        1
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "DataPointColumns", 
        this->GetHeaderValueAsInt(ddfMap, "dimensionNumberOfPoints0") 
    );

    int numComponents =  this->GetHeaderValueAsInt( ddfMap, "numberOfComponents" ); 
    vtkstd::string representation; 
    if (numComponents == 1) {
        representation = "REAL";
    } else if (numComponents == 2) {
        representation = "COMPLEX";
    }

    this->GetOutput()->GetDcmHeader()->SetValue(
        "DataRepresentation", 
        representation 
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SignalDomainColumns", 
        this->GetDimensionDomain( ddfMap["dimensionType0"] )
    );


    //  Private Attributes for spatial domain encoding:
    this->GetOutput()->GetDcmHeader()->SetValue(
        "SVK_ColumnsDomain", 
        this->GetDimensionDomain( ddfMap["dimensionType1"] )
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SVK_RowsDomain", 
        this->GetDimensionDomain( ddfMap["dimensionType2"] )
    );

    this->GetOutput()->GetDcmHeader()->SetValue(
        "SVK_SliceDomain", 
        this->GetDimensionDomain( ddfMap["dimensionType3"] )
    );

}


/*!
 *  Returns the file root without extension
 */
svkDcmHeader::DcmPixelDataFormat svkDdfVolumeReader::GetFileType()
{
    return svkDcmHeader::SIGNED_FLOAT_4;
}


/*! 
 *  Converts the ddf dimension type to a string for DICOM domain tags: 
 */
vtkstd::string svkDdfVolumeReader::GetDimensionDomain( vtkstd::string ddfDomainString )
{
    vtkstd::string domain;  
    if ( ddfDomainString.compare("time") == 0 )  { 
        domain.assign("TIME"); 
    } else if ( ddfDomainString.compare("frequency") == 0 )  { 
        domain.assign("FREQUENCY"); 
    } else if ( ddfDomainString.compare("space") == 0 )  { 
        domain.assign("SPACE"); 
    } else if ( ddfDomainString.compare("k-space") == 0 )  { 
        domain.assign("KSPACE"); 
    }
    return domain; 
}


/*!
 *
 */
int svkDdfVolumeReader::FillOutputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "svkMrsImageData");
    return 1;
}


/*!
 *  Prints the key value pairs parsed from the header.
 */
void svkDdfVolumeReader::PrintKeyValuePairs()
{
    if (this->GetDebug()) {
        vtkstd::map< vtkstd::string, vtkstd::string >::iterator mapIter;
        for ( mapIter = ddfMap.begin(); mapIter != ddfMap.end(); ++mapIter ) {
            cout << this->GetClassName() << " " << mapIter->first << " = ";
            cout << ddfMap[mapIter->first] << endl;
        }
    }
}


/*!
 *
 */
int svkDdfVolumeReader::GetHeaderValueAsInt(vtkstd::map <vtkstd::string, vtkstd::string> hdrMap, 
    vtkstd::string keyString, int valueIndex)
{
    istringstream* iss = new istringstream();
    int value = 0;

    iss->str( hdrMap[keyString] );
    *iss >> value;
    delete iss; 
    return value;
}


/*!
 *
 */
float svkDdfVolumeReader::GetHeaderValueAsFloat(vtkstd::map <vtkstd::string, vtkstd::string> hdrMap, 
    vtkstd::string keyString, int valueIndex)
{
    istringstream* iss = new istringstream();
    float value = 0;
    iss->str( hdrMap[keyString] );
    *iss >> value;
    delete iss; 
    return value;
}


/*!
 *  Determine whether the data is multi-coil, based on number of files and coil name:
 */
bool svkDdfVolumeReader::IsMultiCoil()
{
    bool isMultiCoil = false; 

    if (this->GetFileNames()->GetNumberOfValues() > 1 ) { 
        isMultiCoil = true; 
    }
   
    return isMultiCoil; 
}


/*
 *  Globs file names for multi-file data formats and sets them into a vtkStringArray 
 *  accessible via the GetFileNames() method. 
 */
void svkDdfVolumeReader::GlobFileNames()
{
    string fileName( this->GetFileName() );
    string fileExtension( this->GetFileExtension( this->GetFileName() ) );
    string filePath( this->GetFilePath( this->GetFileName() ) );
    vtkGlobFileNames* globFileNames = vtkGlobFileNames::New();
    globFileNames->SetDirectory( filePath.c_str() ); 
    globFileNames->AddFileNames( string( "*." + fileExtension).c_str() );

    vtkSortFileNames* sortFileNames = vtkSortFileNames::New();
    sortFileNames->GroupingOn();
    sortFileNames->SetInputFileNames( globFileNames->GetFileNames() );
    sortFileNames->NumericSortOn();
    sortFileNames->Update();

    //  If globed file names are not similar, use only the 0th group. 
    //  If there is one group that group is used.
    //  If there are multiple groups and the input file cannot be associated with a group then use input filename only.
    //  If readOneInputFile is set, the groupsToUse remains -1 and only the literal input file is read.  
    int groupToUse = -1;     
    if ( this->readOneInputFile == false ) {
        if (sortFileNames->GetNumberOfGroups() > 1 ) {

            //  Get the group the selected file belongs to:
            vtkStringArray* group;
            for (int k = 0; k < sortFileNames->GetNumberOfGroups(); k++ ) {
                group = sortFileNames->GetNthGroup(k); 
                for (int i = 0; i < group->GetNumberOfValues(); i++) {
                    //  The glob was already limited to the directory where the input file was located
                    //  so to avoid any further relative vs absolute path issues just compare the
                    //  returned file names without path:
                    string groupFile ( this->GetFileNameWithoutPath( group->GetValue(i) ) ); 
                    if( this->GetDebug() ) {
                        cout << "Group: " << groupFile << endl;
                    }
                    if ( fileName.compare( groupFile ) == 0 ) {
                        groupToUse = k; 
                        break; 
                    }
                }
            }
        } else {
            groupToUse = 0;     
        }
    }
    
    if( groupToUse != -1 ) {
        this->SetFileNames( sortFileNames->GetNthGroup( groupToUse ) );
    } else {
        vtkStringArray* inputFile = vtkStringArray::New();
        inputFile->InsertNextValue( fileName.c_str() );
        this->SetFileNames( inputFile );
        inputFile->Delete();
    }

    if (this->GetDebug()) {
        for (int i = 0; i < this->GetFileNames()->GetNumberOfValues(); i++) {
            cout << "FN: " << this->GetFileNames()->GetValue(i) << endl;
        }
    }

    globFileNames->Delete(); 
    sortFileNames->Delete(); 
}

