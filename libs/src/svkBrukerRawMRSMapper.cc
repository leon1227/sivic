/*
 *  Copyright © 2009-2017 The Regents of the University of California.
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


#include <svkBrukerRawMRSMapper.h>
#include <svkVarianReader.h>
#include <svkFreqCorrect.h>

#include <vtkDebugLeaks.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkByteSwap.h>
#include <vtkCallbackCommand.h>


using namespace svk;


vtkStandardNewMacro(svkBrukerRawMRSMapper);



/*!
 *
 */
svkBrukerRawMRSMapper::svkBrukerRawMRSMapper()
{

#if VTK_DEBUG_ON
    this->DebugOn();
    vtkDebugLeaks::ConstructClass("svkBrukerRawMRSMapper");
#endif

    vtkDebugMacro( << this->GetClassName() << "::" << this->GetClassName() << "()" );

    this->specData = NULL;
    this->numFrames = 1;

}


/*!
 *
 */
svkBrukerRawMRSMapper::~svkBrukerRawMRSMapper()
{
    vtkDebugMacro( << this->GetClassName() << "::~" << this->GetClassName() << "()" );

    if ( this->specData != NULL )  {
        delete [] specData;
        this->specData = NULL;
    }

}


/*!
 *  Initializes the svkDcmHeader adapter to a specific IOD type      
 *  and initizlizes the svkDcmHeader member of the svkImageData 
 *  object.    
 */
void svkBrukerRawMRSMapper::InitializeDcmHeader(map <string, vector < string> >  paramMap, 
    svkDcmHeader* header, svkMRSIOD* iod, int swapBytes) 
{
    this->paramMap = paramMap; 
    this->dcmHeader = header; 
    this->iod = iod;   
    this->swapBytes = swapBytes; 

    this->ConvertCmToMm(); 

    this->InitPatientModule();
    this->InitGeneralStudyModule();
    this->InitGeneralSeriesModule();
    this->InitGeneralEquipmentModule();

    this->InitMultiFrameFunctionalGroupsModule();
//    this->InitMultiFrameDimensionModule();
//    this->InitAcquisitionContextModule();
    this->InitMRSpectroscopyModule();
    this->InitMRSpectroscopyPulseSequenceModule();

    this->InitMRSpectroscopyDataModule();

    this->dcmHeader->SetValue( "SVK_PRIVATE_TAG",  "SVK_PRIVATE_CREATOR"); 

}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitPatientModule()
{

    this->dcmHeader->InitPatientModule(
        this->dcmHeader->GetDcmPatientName( this->GetHeaderValueAsString("SUBJECT_name_string") ),
        this->GetHeaderValueAsString("SUBJECT_id"), 
        this->GetHeaderValueAsString("SUBJECT_dbirth"), 
        this->GetHeaderValueAsString("SUBJECT_sex") 
    );

}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitGeneralStudyModule()
{
    string timeDate = this->GetHeaderValueAsString( "SUBJECT_date" ); 
    size_t delim = timeDate.find("T"); 
    string date = timeDate.substr(0, delim); 

    this->dcmHeader->InitGeneralStudyModule(
        date, 
        "",
        "",
        this->GetHeaderValueAsString("SUBJECT_study_instance_uid"), 
        "", 
        ""
    );

}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitGeneralSeriesModule()
{
    this->dcmHeader->InitGeneralSeriesModule(
        "0", 
        "BRUKER SER Data", 
        this->GetDcmPatientPositionString()
    );
}


/*!
 *  DDF is historically the UCSF representation of a GE raw file so
 *  initialize to svkBrukerRawMRSMapper::MFG_STRING.
 */
void svkBrukerRawMRSMapper::InitGeneralEquipmentModule()
{
    this->dcmHeader->SetValue(
        "Manufacturer",
        "Bruker"
    );
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMultiFrameFunctionalGroupsModule()
{

    this->InitSharedFunctionalGroupMacros();

    this->dcmHeader->SetValue(
        "InstanceNumber",
        1
    );

    string timeDate = this->GetHeaderValueAsString( "SUBJECT_date" );
    size_t delim = timeDate.find("T");
    string date = timeDate.substr(0, delim);

    this->dcmHeader->SetValue(
        "ContentDate",
        date
    );

    this->numSlices = 1; 
    int numEchoes = this->GetHeaderValueAsInt("NECHOES");

    this->numFrames = this->numSlices * numEchoes;
    this->dcmHeader->SetValue(
        "NumberOfFrames",
        this->numFrames
    );

    this->InitPerFrameFunctionalGroupMacros();

}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitSharedFunctionalGroupMacros()
{

    this->InitPixelMeasuresMacro();
    this->InitPlaneOrientationMacro();

    this->InitMRTimingAndRelatedParametersMacro();
    this->InitMRSpectroscopyFOVGeometryMacro();
    this->InitMREchoMacro();

    this->InitMRModifierMacro();

    this->InitMRReceiveCoilMacro();

    //this->InitMRTransmitCoilMacro();
    this->InitMRAveragesMacro();
    //this->InitMRSpatialSaturationMacro();
    //this->InitMRSpatialVelocityEncodingMacro();

}


/*!
 *  The FID toplc is the center of the first voxel.
 */
void svkBrukerRawMRSMapper::InitPerFrameFunctionalGroupMacros()
{

    double dcos[3][3];
    this->dcmHeader->SetSliceOrder( this->dataSliceOrder );
    this->dcmHeader->GetDataDcos( dcos );

    double pixelSpacing[3];
    this->dcmHeader->GetPixelSize(pixelSpacing);

    int numPixels[3];
    numPixels[0] = this->GetHeaderValueAsInt("ACQ_spatial_size_0");
    numPixels[1] = this->GetHeaderValueAsInt("ACQ_spatial_size_1");
    numPixels[2] = 1; 

    //  Get center coordinate float array from fidMap and use that to generate
    //  Displace from that coordinate by 1/2 fov - 1/2voxel to get to the center of the
    //  toplc from which the individual frame locations are calculated

    //  If volumetric 3D (not 2D), get the center of the TLC voxel in LPS coords:
    double* volumeTlcLPSFrame = new double[3];
    //  if more than 1 slice:
    if ( numPixels[2] > 1 ) {

        //  Get the volumetric center in acquisition frame coords:
        double volumeCenterAcqFrame[3];
        for (int i = 0; i < 3; i++) {
            //volumeCenterAcqFrame[i] = this->GetHeaderValueAsFloat("location[]", i);
            volumeCenterAcqFrame[i] = 0.; 
        }

        double* volumeTlcAcqFrame = new double[3];
        for (int i = 0; i < 3; i++) {
            volumeTlcAcqFrame[i] = volumeCenterAcqFrame[i]
                                 + (100 - pixelSpacing[i] )/2;
                                 //+ ( this->GetHeaderValueAsFloat("span[]", i) - pixelSpacing[i] )/2;
        }
        svkVarianReader::UserToMagnet(volumeTlcAcqFrame, volumeTlcLPSFrame, dcos);
        delete [] volumeTlcAcqFrame;

    }


    //  Center of toplc (LPS) pixel in frame:
    double toplc[3];

    //
    //  If 3D vol, calculate slice position, otherwise use value encoded
    //  into slice header
    //

     //  If 2D (single slice)
     if ( numPixels[2] == 1 ) {

        //  Location is the center of the image frame in user (acquisition frame).
        double centerAcqFrame[3];
        for ( int j = 0; j < 3; j++) {
            centerAcqFrame[j] = 0.0;
        }

        //  Now get the center of the tlc voxel in the acq frame:
        double* tlcAcqFrame = new double[3];
        for (int j = 0; j < 2; j++) {
            tlcAcqFrame[j] = centerAcqFrame[j]
                - ( ( numPixels[j] * pixelSpacing[j] ) - pixelSpacing[j] )/2;
        }
        tlcAcqFrame[2] = centerAcqFrame[2];

        //  and convert to LPS (magnet) frame:
        svkVarianReader::UserToMagnet(tlcAcqFrame, toplc, dcos);
    
        delete [] tlcAcqFrame;
    
    } else {

        for(int j = 0; j < 3; j++) { //L, P, S
            toplc[j] = volumeTlcLPSFrame[j]; 
        }
    
   }


    svkDcmHeader::DimensionVector dimensionVector = this->dcmHeader->GetDimensionIndexVector();
    svkDcmHeader::SetDimensionVectorValue(&dimensionVector, svkDcmHeader::SLICE_INDEX, this->numFrames-1);

    this->dcmHeader->InitPerFrameFunctionalGroupSequence(
        toplc, pixelSpacing, dcos, &dimensionVector
    );

    delete volumeTlcLPSFrame;
}


/*!
 *  The DICOM PlaneOrientationSequence is set from orientational params defined in the 
 *  Varian procpar file.  According to the VNMR User Programming documentation available at
 *  http://www.varianinc.com/cgi-bin/nav?varinc/docs/products/nmr/apps/pubs/sys_vn&cid=975JIOILOKPQNGMKIINKNGK&zsb=1060363007.usergroup 
 *  (VNMR 6.1C, Pub No. 01-999165-00, Rev B0802, page 155), the Euler angles are 
 *  defined in  the "User Guide Imaging.  The Varian User Guide: Imaging 
 *  (Pub. No. 01-999163-00, Rev. A0201, page 272) provides the following definition
 *  of the procpar euler angles: 
 * 
 *  "Arguments: phi, psi, theta are the coordinates of a point in the logical imaging
 *   reference frame (the coordinate system deﬁned by the readout, phase encode,
 *   and slice select axes) and the Euler angles that deﬁne the orientation of the
 *   logical frame:
 *      • phi is the angular rotation of the image plane about a line normal to the
 *        image plane.
 *      • psi is formed by the projection of a line normal to the imaging plane onto
 *        the magnet XY plane, and the magnet Y axis.
 *      • theta is formed by the line normal to the imaging plane, and the magnet
 *        Z axis."
 *
 *  i.e. 
 *      axial(phi, psi, theta) => 0,  0,  0
 *      cor  (phi, psi, theta) => 0,  0, 90
 *      sag  (phi, psi, theta) => 0, 90, 90
 *
 */
void svkBrukerRawMRSMapper::InitPlaneOrientationMacro()
{

    this->dcmHeader->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "PlaneOrientationSequence"
    );

    //  Get the euler angles for the acquisition coordinate system:
    float psi = 0; 
    float phi = 0; 
    float theta = 0; 
    //float psi = this->GetHeaderValueAsFloat("psi", 0);
    //float phi = this->GetHeaderValueAsFloat("phi", 0);
    //float theta = this->GetHeaderValueAsFloat("theta", 0);

    vtkTransform* eulerTransform = vtkTransform::New();
    eulerTransform->RotateX( theta);
    eulerTransform->RotateY( phi );
    eulerTransform->RotateZ( psi );
    vtkMatrix4x4* dcos = vtkMatrix4x4::New();
    eulerTransform->GetMatrix(dcos);

    if (this->GetDebug()) {
        cout << *dcos << endl;
    }

    //  and analagous to the fdf reader, convert from LAI to LPS: 
    //dcos->SetElement(0, 0, dcos->GetElement(0, 0)  * 1 );
    //dcos->SetElement(0, 1, dcos->GetElement(0, 1)  * -1);
    //dcos->SetElement(0, 2, dcos->GetElement(0, 2)  * -1);

    //dcos->SetElement(1, 0, dcos->GetElement(1, 0)  * 1 );
    //dcos->SetElement(1, 1, dcos->GetElement(1, 1)  * -1);
    //dcos->SetElement(1, 2, dcos->GetElement(1, 2)  * -1);

    //dcos->SetElement(2, 0, dcos->GetElement(2, 0)  * 1 );
    //dcos->SetElement(2, 1, dcos->GetElement(2, 1)  * -1);
    //dcos->SetElement(2, 2, dcos->GetElement(2, 2)  * -1);
    

    string orientationString;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            ostringstream dcosOss;
            dcosOss.setf(ios::fixed);
            dcosOss << dcos->GetElement(i, j);
            orientationString.append( dcosOss.str() );
            if (i != 1 || j != 2  ) {
                orientationString.append( "\\");
            }
        }
    }

    this->dcmHeader->AddSequenceItemElement(
        "PlaneOrientationSequence",
        0,
        "ImageOrientationPatient",
        orientationString,
        "SharedFunctionalGroupsSequence",
        0
    );

    //  Determine whether the data is ordered with or against the slice normal direction.
    double normal[3];
    this->dcmHeader->GetNormalVector(normal);

    double dcosSliceOrder[3];
    for (int j = 0; j < 3; j++) {
        dcosSliceOrder[j] = dcos->GetElement(2, j);
    }

    //  Use the scalar product to determine whether the data in the .cmplx
    //  file is ordered along the slice normal or antiparalle to it.
    vtkMath* math = vtkMath::New();
    if (math->Dot(normal, dcosSliceOrder) > 0 ) {
        this->dataSliceOrder = svkDcmHeader::INCREMENT_ALONG_POS_NORMAL;
    } else {
        this->dataSliceOrder = svkDcmHeader::INCREMENT_ALONG_NEG_NORMAL;
    }
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMRTimingAndRelatedParametersMacro()
{
    this->dcmHeader->InitMRTimingAndRelatedParametersMacro(
        1000,
        0
        //this->GetHeaderValueAsFloat( "tr" ),
        //this->GetHeaderValueAsFloat("fliplist", 0)
    ); 
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMRSpectroscopyFOVGeometryMacro()
{
    this->dcmHeader->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRSpectroscopyFOVGeometrySequence"
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SpectroscopyAcquisitionDataColumns",
        this->GetHeaderValueAsInt("PVM_DigNp"), 
        "SharedFunctionalGroupsSequence",
        0
    );


    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SpectroscopyAcquisitionPhaseColumns",
        this->GetHeaderValueAsInt("ACQ_spatial_size_0"),
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SpectroscopyAcquisitionPhaseRows",
        this->GetHeaderValueAsInt("ACQ_spatial_size_1"),
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SpectroscopyAcquisitionOutOfPlanePhaseSteps",
        1,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SVK_SpectroscopyAcquisitionTLC",
        "0\\0\\0",
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SVK_SpectroscopyAcquisitionPixelSpacing",
        //this->GetHeaderValueAsString("vox1", 0) + '\\' + this->GetHeaderValueAsString("vox2", 0),
        "10\\10",
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SVK_SpectroscopyAcquisitionSliceThickness",
        //this->GetHeaderValueAsFloat("vox3", 0),
        "10",
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SVK_SpectroscopyAcquisitionOrientation",
        "0\\0\\0\\0\\0\\0\\0\\0\\0",
        "SharedFunctionalGroupsSequence",
        0
    );


    // ==================================================
    //  Reordered Params
    // ==================================================
    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SVK_SpectroscopyAcqReorderedPhaseColumns",
        this->GetHeaderValueAsInt("ACQ_spatial_size_0"),
        "SharedFunctionalGroupsSequence",
        0
    );
    
    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                               
        "SVK_SpectroscopyAcqReorderedPhaseRows",
        this->GetHeaderValueAsInt("ACQ_spatial_size_1"),
        "SharedFunctionalGroupsSequence",
        0
    );
    
    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                               
        "SVK_SpectroscopyAcqReorderedOutOfPlanePhaseSteps",
        1,
        "SharedFunctionalGroupsSequence",
        0
    );
    
    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,                               
        "SVK_SpectroscopyAcqReorderedTLC",
        "0\\0\\0",
        "SharedFunctionalGroupsSequence",
        0
    );
    
    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",   
        0,                                  
        "SVK_SpectroscopyAcqReorderedPixelSpacing",
        "10\\10",
        "SharedFunctionalGroupsSequence",
        0
    );
    
    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,                                      
        "SVK_SpectroscopyAcqReorderedSliceThickness",
        "10", 
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "SVK_SpectroscopyAcqReorderedOrientation",
        "0\\0\\0\\0\\0\\0\\0\\0\\0",
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRSpectroscopyFOVGeometrySequence",
        0,
        "PercentSampling",
        1,
        "SharedFunctionalGroupsSequence",
        0
    );

    this->dcmHeader->AddSequenceItemElement(
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
void svkBrukerRawMRSMapper::InitMREchoMacro()
{

    this->dcmHeader->InitMREchoMacro( 1 * 1000.); 
}


/*!
 *  Override in concrete mapper for specific acquisitino
 */
void svkBrukerRawMRSMapper::InitMRModifierMacro()
{
    float inversionTime = 0; 
    this->dcmHeader->InitMRModifierMacro( inversionTime );
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMRTransmitCoilMacro()
{
    this->dcmHeader->InitMRTransmitCoilMacro("Bruker", "UNKNOWN", "BODY");
}


/*! 
 *  Receive Coil:
 */
void svkBrukerRawMRSMapper::InitMRReceiveCoilMacro()
{

    this->dcmHeader->AddSequenceItemElement(
        "SharedFunctionalGroupsSequence",
        0,
        "MRReceiveCoilSequence"
    );

    this->dcmHeader->AddSequenceItemElement(
        "MRReceiveCoilSequence",
        0,
        "ReceiveCoilName",
        "Bruker Coil",
        "SharedFunctionalGroupsSequence",
        0
    );

}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMRAveragesMacro()
{
    int numAverages = 1; 
    this->dcmHeader->InitMRAveragesMacro(numAverages);
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMultiFrameDimensionModule()
{
    int indexCount = 0; 
    this->dcmHeader->AddSequenceItemElement(
        "DimensionIndexSequence",
        indexCount,
        "DimensionDescriptionLabel",
        "Slice"
    );

/*
    if (this->GetNumTimePoints() > 1) {
        indexCount++; 
        this->dcmHeader->AddSequenceItemElement(
            "DimensionIndexSequence",
            indexCount,
            "DimensionDescriptionLabel",
            "Time Point"
        );
    }
*/

//      if (this->GetNumCoils() > 1) {
//          indexCount++; 
//          this->dcmHeader->AddSequenceItemElement(
//          "DimensionIndexSequence",
//          indexCount,
//          "DimensionDescriptionLabel",
//          "Coil Number"
//      );

//
//        this->dmHeader()->AddSequenceItemElement(
//            "DimensionIndexSequence",
//            1,
//            "DimensionIndexPointer",
//            "18H\\00H\\47H\\90"
//        );
//        this->dcmHeader->AddSequenceItemElement(
//            "DimensionIndexSequence",
//            1,
//            "FunctionalGroupPointer",
//            //"MultiCoilDefinitionSequence"
//            "18H\\00H\\47H\\90"
//        );
//  }

}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitAcquisitionContextModule()
{
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMRSpectroscopyModule()
{

    /*  =======================================
     *  MR Image and Spectroscopy Instance Macro
     *  ======================================= */
    string timeDate = this->GetHeaderValueAsString( "SUBJECT_date" );

    this->dcmHeader->SetValue(
        "AcquisitionDateTime",
        timeDate
    );
    
    this->dcmHeader->SetValue(
        "AcquisitionDuration",
        0
    );


    string nucleus = this->GetHeaderValueAsString("NUC1"); 
    this->dcmHeader->SetValue(
        "ResonantNucleus",
        nucleus 
    );

    this->dcmHeader->SetValue(
        "KSpaceFiltering",
        "NONE"
    );

    this->dcmHeader->SetValue(
        "ApplicableSafetyStandardAgency",
        "Research"
    );

    //  B0 in Gauss?
    this->dcmHeader->SetValue(
        "MagneticFieldStrength",
        3
        //static_cast< int > ( this->GetHeaderValueAsFloat("B0") / 10000 )
    );
    /*  =======================================
     *  END: MR Image and Spectroscopy Instance Macro
     *  ======================================= */

    this->dcmHeader->SetValue(
        "ImageType",
        string("ORIGINAL\\PRIMARY\\SPECTROSCOPY\\NONE")
    );


    /*  =======================================
     *  Spectroscopy Description Macro
     *  ======================================= */
    this->dcmHeader->SetValue(
        "VolumetricProperties",
        string("VOLUME")
    );

    this->dcmHeader->SetValue(
        "VolumeBasedCalculationTechnique",
        string("NONE")
    );

    this->dcmHeader->SetValue(
        "ComplexImageComponent",
        string("COMPLEX")
    );

    this->dcmHeader->SetValue(
        "AcquisitionContrast",
        "UNKNOWN"
    );  
    /*  =======================================
     *  END: Spectroscopy Description Macro
     *  ======================================= */


    this->dcmHeader->SetValue(
        "TransmitterFrequency",
        //this->GetHeaderValueAsFloat( "sfrq" )
        100
    );

    this->dcmHeader->SetValue(
        "SpectralWidth",
        //this->GetHeaderValueAsFloat( "sw" )
        100    
    );

    this->dcmHeader->SetValue(
        "SVK_FrequencyOffset",
        0
    );

    //  sp is the frequency in Hz at left side (downfield/High freq) 
    //  side of spectrum: 
    //
    //float ppmRef = this->GetHeaderValueAsFloat( "sp" ) + this->GetHeaderValueAsFloat( "sw" )/2.;
    //ppmRef /= this->GetHeaderValueAsFloat( "sfrq" ); 
    float ppmRef = 0; 
    this->dcmHeader->SetValue(
        "ChemicalShiftReference",
        ppmRef 
    );

    this->dcmHeader->SetValue(
        "VolumeLocalizationTechnique",
        ""
    );


    this->dcmHeader->SetValue(
        "Decoupling",
        "NO"
    );

    this->dcmHeader->SetValue(
        "TimeDomainFiltering",
        "NONE"
    );

    this->dcmHeader->SetValue(
        "NumberOfZeroFills",
        0
    );

    this->dcmHeader->SetValue(
        "BaselineCorrection",
        string("NONE")
    );

    this->dcmHeader->SetValue(
        "FrequencyCorrection",
        "NO"
    );

    this->dcmHeader->SetValue(
        "FirstOrderPhaseCorrection",
        string("NO")
    );


    this->dcmHeader->SetValue(
        "WaterReferencedPhaseCorrection",
        string("NO")
    );
}


/*!
 *
 */
void svkBrukerRawMRSMapper::InitMRSpectroscopyDataModule()
{
    this->dcmHeader->SetValue( "Columns", this->GetHeaderValueAsInt("ACQ_spatial_size_0") );
    this->dcmHeader->SetValue( "Rows", this->GetHeaderValueAsInt("ACQ_spatial_size_1") );
    this->dcmHeader->SetValue( "DataPointRows", 0 );
    this->dcmHeader->SetValue( "DataPointColumns", this->GetHeaderValueAsInt("PVM_DigNp", 0) );
    this->dcmHeader->SetValue( "DataRepresentation", "COMPLEX" );
    this->dcmHeader->SetValue( "SignalDomainColumns", "TIME" );
    this->dcmHeader->SetValue( "SVK_ColumnsDomain", "KSPACE" );
    this->dcmHeader->SetValue( "SVK_RowsDomain", "KSPACE" );
    this->dcmHeader->SetValue( "SVK_SliceDomain", "KSPACE" );
}


/*!
 *  Reads spec data from fid file.
 */
void svkBrukerRawMRSMapper::ReadSerFile( string serFileName, svkImageData* data )
{
    
    vtkDebugMacro( << this->GetClassName() << "::ReadSerFile()" );

    ifstream* serDataIn = new ifstream();
    serDataIn->exceptions( ifstream::eofbit | ifstream::failbit | ifstream::badbit );

    string rawFormat = this->GetHeaderValueAsString("GO_raw_data_format"); 
    int pixelWordSize = 4;
    if ( rawFormat.find("32_BIT") != string::npos ) {
        pixelWordSize = 4;
    }

    int numComponents = 2;
    int numSpecPoints = this->dcmHeader->GetIntValue( "DataPointColumns" );

    int numVoxels[3]; 
    numVoxels[0] = this->dcmHeader->GetIntValue( "Columns" ); 
    numVoxels[1] = this->dcmHeader->GetIntValue( "Rows" ); 
    numVoxels[2] = this->dcmHeader->GetNumberOfSlices( ); 

    int numPixInVolume = numVoxels[0] * numVoxels[1] * numVoxels[2]; 

    int numBytesInVol = ( numPixInVolume * pixelWordSize * numComponents * numSpecPoints );

    serDataIn->open( serFileName.c_str(), ios::binary );

    /*
     *   Flatten the data volume into one dimension
     */
    if (this->specData == NULL) {
        this->specData = new char[ numBytesInVol ];
    }

    serDataIn->seekg(0, ios::beg);
    serDataIn->read( (char *)(this->specData), numBytesInVol);

    /*
     *  FID files are bigendian.
     */
    if ( this->swapBytes ) {
        vtkByteSwap::SwapVoidRange((void *)this->specData, numBytesInVol/pixelWordSize, pixelWordSize);
    }

    svkDcmHeader* hdr = this->dcmHeader;

    double progress = 0;

    int numTimePts = hdr->GetNumberOfTimePoints(); 
    int numCoils = hdr->GetNumberOfCoils(); 
    for (int coilNum = 0; coilNum < numCoils; coilNum++) {
        for (int timePt = 0; timePt < numTimePts; timePt++) {

            for (int z = 0; z < numVoxels[2] ; z++) {
                for (int y = 0; y < numVoxels[1]; y++) {
                    for (int x = 0; x < numVoxels[0]; x++) {
                        SetCellSpectrum(data, x, y, z, timePt, coilNum);

                        if( timePt % 2 == 0 ) { // only update every other index
				            progress = (timePt * coilNum)/((double)(numTimePts*numCoils));
				            this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void *>(&progress));
        	            }
                    }
                }
            }
        }
    }

    //  Bruker FIDs are shifted by a group delay number of points defined by PVM_DigShiftDbl.  
    //  Apply this global shift to correct the data here: 
    svkFreqCorrect* freqShift = svkFreqCorrect::New(); 
    freqShift->SetInputData( data ); 
    freqShift->SetCircularShift(); 
    freqShift->SetGlobalFrequencyShift( 
        -1 * this->GetHeaderValueAsInt("PVM_DigShiftDbl") 
    ); 
    freqShift->Update(); 
    freqShift->Delete(); 

    progress = 1;
	this->InvokeEvent(vtkCommand::ProgressEvent,static_cast<void *>(&progress));

    serDataIn->close();
    delete serDataIn;

}


/*!
 *
 */
void svkBrukerRawMRSMapper::SetCellSpectrum(vtkImageData* data, int x, int y, int z, int timePt, int coilNum)
{

    int numComponents = 1;
    string representation =  this->dcmHeader->GetStringValue( "DataRepresentation" );
    if (representation.compare( "COMPLEX" ) == 0 ) {
        numComponents = 2;
    }
    vtkDataArray* dataArray = vtkDataArray::CreateDataArray(VTK_FLOAT);
    dataArray->SetNumberOfComponents( numComponents );

    int numPts = this->dcmHeader->GetIntValue( "DataPointColumns" );
    dataArray->SetNumberOfTuples(numPts);

    char arrayName[30];
    sprintf(arrayName, "%d %d %d %d %d", x, y, z, timePt, coilNum);
    dataArray->SetName(arrayName);

    int numVoxels[3];
    numVoxels[0] = this->dcmHeader->GetIntValue( "Columns" );
    numVoxels[1] = this->dcmHeader->GetIntValue( "Rows" );
    numVoxels[2] = this->dcmHeader->GetNumberOfSlices();

    //  if cornoal, swap z and x:
    //int xTmp = x; 
    //x = y; 
    //y = xTmp; 
    x = numVoxels[0] - x - 1; 
    y = numVoxels[1] - y - 1; 

    int offset = (numPts * numComponents) *  (
                     ( numVoxels[0] * numVoxels[1] * numVoxels[2] ) * timePt
                    +( numVoxels[0] * numVoxels[1] ) * z
                    +  numVoxels[0] * y
                    +  x
                 );


    string rawFormat = this->GetHeaderValueAsString("GO_raw_data_format"); 
    if ( rawFormat.find("SGN_INT") != string::npos ) {
        for (int i = 0; i < numPts; i++) {
            int intValRe     = static_cast<int*>(  this->specData)[offset + (i * 2)];
            int intValIm     = static_cast<int*>(  this->specData)[offset + (i * 2 + 1)];
            float floatVal[2]; 
            floatVal[0] = intValRe; 
            floatVal[1] = intValIm; 
            dataArray->SetTuple( i,  floatVal ); 
        }
    } else { 
        for (int i = 0; i < numPts; i++) {
            dataArray->SetTuple(i, &(static_cast<float*>(this->specData)[offset + (i * 2)]));
        }
    }

    //  Add the spectrum's dataArray to the CellData:
    //  vtkCellData is a subclass of vtkFieldData
    data->GetCellData()->AddArray(dataArray);

    dataArray->Delete();
    
    return;
}


/*!
 *  Convert FID (Procpar) spatial values from cm to mm: FOV, Center, etc. 
 */
void svkBrukerRawMRSMapper::ConvertCmToMm()
{
/*
    float cmToMm = 10.;
    float tmp;
    ostringstream oss;

    // FOV 
    tmp = cmToMm * this->GetHeaderValueAsFloat("lpe", 0);
    oss << tmp;
    ( this->paramMap["lpe"] )[0][0] = oss.str();

    oss.str("");
    tmp = cmToMm * this->GetHeaderValueAsFloat("lpe2", 0);
    oss << tmp;
    ( this->paramMap["lpe2"] )[0][0] = oss.str();

    oss.str("");
    tmp = cmToMm * this->GetHeaderValueAsFloat("lro", 0);
    oss << tmp;
    ( this->paramMap["lro"] )[0][0] = oss.str(); 


    //  Center 
    oss.str("");
    tmp = cmToMm * this->GetHeaderValueAsFloat("ppe", 0);
    oss << tmp;
    ( this->paramMap["ppe"] )[0][0] = oss.str();

    oss.str("");
    tmp = cmToMm * this->GetHeaderValueAsFloat("ppe2", 0);
    oss << tmp;
    ( this->paramMap["ppe2"] )[0][0] = oss.str();

    oss.str("");
    tmp = cmToMm * this->GetHeaderValueAsFloat("pro", 0);
    oss << tmp;
    ( this->paramMap["pro"] )[0][0] = oss.str();
*/

}


/*!
 *  Use the Procpar patient position string to set the DCM_PatientPosition data element.
 */
string svkBrukerRawMRSMapper::GetDcmPatientPositionString()
{
    string dcmPatientPosition;

    string position1 = this->GetHeaderValueAsString("SUBJECT_entry");
    if( position1.find("HeadFirst") != string::npos ) {
        dcmPatientPosition.assign("HF");
    } else if( position1.find("FeetFirst") != string::npos ) {
        dcmPatientPosition.assign("FF");
    } else {
        dcmPatientPosition.assign("UNKNOWN");
    }

    string position2 = this->GetHeaderValueAsString("ACQ_patient_pos");
    if( position2.find("Supine") != string::npos ) {
        dcmPatientPosition += "S";
    } else if( position2.find("Prone") != string::npos ) {
        dcmPatientPosition += "P";
    } else if( position2.find("Decubitus left") != string::npos ) {
        dcmPatientPosition += "DL";
    } else if( position2.find("Decubitus right") != string::npos ) {
        dcmPatientPosition += "DR";
    } else {
        dcmPatientPosition += "UNKNOWN";
    }

    return dcmPatientPosition;
}


/*!
 *
 */
int svkBrukerRawMRSMapper::GetHeaderValueAsInt(string keyString, int valueIndex )
{

    istringstream* iss = new istringstream();
    int value;

    iss->str( (this->paramMap[keyString])[valueIndex]);
    *iss >> value;

    delete iss; 

    return value;
}


/*!
 *
 */
float svkBrukerRawMRSMapper::GetHeaderValueAsFloat(string keyString, int valueIndex )
{

    istringstream* iss = new istringstream();
    float value;
    iss->str( (this->paramMap[keyString])[valueIndex]);
    *iss >> value;

    delete iss; 

    return value;
}


/*!
 *
 */
string svkBrukerRawMRSMapper::GetHeaderValueAsString(string keyString, int valueIndex )
{

    map< string, vector < string > >::iterator it;
    it = this->paramMap.find(keyString);
    if ( it != this->paramMap.end() ) {
        return (this->paramMap[keyString])[valueIndex];
    } else {
        return ""; 
    }
}



/*!
 *  Pixel Spacing:
 */
void svkBrukerRawMRSMapper::InitPixelMeasuresMacro()
{
    float numPixels[3];
    numPixels[0] = this->GetHeaderValueAsInt("ACQ_spatial_size_0");
    numPixels[1] = this->GetHeaderValueAsInt("ACQ_spatial_size_1");
    numPixels[2] = 1;


    //  Not sure if this is best, also see lpe (phase encode resolution in cm)
    float pixelSize[3];
    pixelSize[0] = 10; 
    pixelSize[1] = 10; 
    pixelSize[2] = 10; 
    //pixelSize[0] = this->GetHeaderValueAsFloat("vox1", 0);
    //pixelSize[1] = this->GetHeaderValueAsFloat("vox2", 0);
    //pixelSize[2] = this->GetHeaderValueAsFloat("vox3", 0);

    string pixelSizeString[3];

    for (int i = 0; i < 3; i++) {
        ostringstream oss;
        oss << pixelSize[i];
        pixelSizeString[i].assign( oss.str() );
    }

    this->dcmHeader->InitPixelMeasuresMacro(
        pixelSizeString[0] + "\\" + pixelSizeString[1],
        pixelSizeString[2]
    );
}


void svkBrukerRawMRSMapper::InitMRSpectroscopyPulseSequenceModule() 
{
} 
