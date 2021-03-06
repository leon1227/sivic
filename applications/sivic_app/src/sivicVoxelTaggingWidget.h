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
 */

#ifndef SVK_VOXEL_TAGGING_WIDGET_H
#define SVK_VOXEL_TAGGING_WIDGET_H

#include <vtkObjectFactory.h>
#include <vtkKWTkUtilities.h>
#include <vtkKWApplication.h>
#include <vtkKWTopLevel.h>
#include <vtkKWCheckButton.h>
#include <vtkKWSeparator.h>

#include <svkHSVD.h>
#include <svkDataView.h>
#include <svkDataModel.h>
#include <svkPlotGridViewController.h>
#include <svkOverlayViewController.h>
#include <svkPlotGridView.h>
#include <sivicKWCompositeWidget.h>
#include <vtkKWMultiColumnListWithScrollbars.h>
#include <vtkKWMultiColumnListWithLabel.h>
#include <vtkKWPushButton.h>
#include <string.h>
#include <vector>


using namespace svk; 

class sivicVoxelTaggingWidget : public sivicKWCompositeWidget
{

    friend class vtkSivicController;

    public:

        static sivicVoxelTaggingWidget *New();
        vtkTypeMacro(sivicVoxelTaggingWidget,sivicKWCompositeWidget);


    protected:

        sivicVoxelTaggingWidget();
        ~sivicVoxelTaggingWidget();
        
        vtkKWPushButton*                createTagVolumeButton;
        vtkKWPushButton*                addTagButton;
        vtkKWPushButton*                removeTagButton;

        void ReadDefaultTagsFromRegistry();
        void UpdateTagsInRegistry();
        int  GetNumberOfTagsInRegistry();
        void CreateTagVolume();
		void UpdateTagsList( );
        void AddTag();
        void RemoveTag(int tagVolumeNumber);
        void SetTagName(string tagName, int tagVolume );
        void SetTagValue(int tagValue, int tagVolume );
        void GetTagsFromData( svkImageData* voxelTagData );

        // Description:
        // Create the widget.
        virtual void    CreateWidget();
        virtual void    ProcessCallbackCommandEvents( vtkObject*, unsigned long, void* );



    private:

        vector<string> tagNames;
        vector<int> tagValues;
        vtkKWMultiColumnListWithScrollbars* tagsTable;

        sivicVoxelTaggingWidget(const sivicVoxelTaggingWidget&);   // Not implemented.
        void operator=(const sivicVoxelTaggingWidget&);  // Not implemented.
        
};

#endif //SVK_VOXEL_TAGGING_WIDGET_H
