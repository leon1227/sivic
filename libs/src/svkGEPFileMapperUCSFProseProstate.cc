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
 *  $URL: svn+ssh://agentmess@svn.code.sf.net/p/sivic/code/trunk/libs/src/svkGEPFileMapper.cc $
 *  $Rev: 2119 $
 *  $Author: jccrane $
 *  $Date: 2014-12-19 13:13:17 -0800 (Fri, 19 Dec 2014) $
 *
 *  Authors:
 *      Jason C. Crane, Ph.D.
 *      Beck Olson
 */


#include <svkGEPFileMapperUCSFProseProstate.h>
#include <vtkDebugLeaks.h>

using namespace svk;


//vtkCxxRevisionMacro(svkGEPFileMapperUCSFProseProstate, "$Rev: 2119 $");
vtkStandardNewMacro(svkGEPFileMapperUCSFProseProstate);


/*!
 *
 */
svkGEPFileMapperUCSFProseProstate::svkGEPFileMapperUCSFProseProstate()
{

#if VTK_DEBUG_ON
    this->DebugOn();
    vtkDebugLeaks::ConstructClass("svkGEPFileMapperUCSFProseProstate");
#endif

    vtkDebugMacro( << this->GetClassName() << "::" << this->GetClassName() << "()" );

}


/*!
 *
 */
svkGEPFileMapperUCSFProseProstate::~svkGEPFileMapperUCSFProseProstate()
{
    vtkDebugMacro( << this->GetClassName() << "::~" << this->GetClassName() << "()" );
}



/*!
 *  Get the voxel spacing in 3D, with option for anisotropic FOV with rhuser19. Note that the slice spacing may 
 *  include a skip. 
 */
void svkGEPFileMapperUCSFProseProstate::GetVoxelSpacing( double voxelSpacing[3] )
{


    float user19 =  this->GetHeaderValueAsFloat( "rhi.user19" ); 


    if ( user19 > 0  && this->pfileVersion >= 9 ) {

        voxelSpacing[0] = user19; 
        voxelSpacing[1] = user19; 
        voxelSpacing[2] = this->GetHeaderValueAsFloat( "rhi.scanspacing" );

    } else {
       
        float fov[3];  
        this->GetFOV( fov ); 

        int numVoxels[3]; 
        this->GetNumVoxels( numVoxels ); 

        voxelSpacing[0] = fov[0]/numVoxels[0];
        voxelSpacing[1] = fov[1]/numVoxels[1];
        voxelSpacing[2] = fov[2]/numVoxels[2];
    }
}


