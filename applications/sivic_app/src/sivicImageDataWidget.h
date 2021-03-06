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

#ifndef SVK_IMAGE_DATA_WIDGET_H
#define SVK_IMAGE_DATA_WIDGET_H

#include <vtkKWCompositeWidget.h>
#include <vtkObjectFactory.h>

#include <svkHSVD.h>
#include <svkDataModel.h>
#include <svkPlotGridViewController.h>
#include <svkOverlayViewController.h>
#include <sivicKWCompositeWidget.h>
#include <vtkKWMultiColumnListWithScrollbars.h>
#include <vtkKWMultiColumnList.h>

#include <string.h>

using namespace svk; 

class sivicImageDataWidget : public sivicKWCompositeWidget
{

    friend class vtkSivicController;

    public:

        static sivicImageDataWidget *New();
        vtkTypeMacro(sivicImageDataWidget,sivicKWCompositeWidget);
        void UpdateReferenceImageList();
        void SetFilename( int row, string filename );
        void SetModel( svkDataModel* model );

    protected:


        sivicImageDataWidget();
        ~sivicImageDataWidget();

        // Description:
        // Create the widget.
        virtual void    CreateWidget();
        virtual void    CreateListWidget();
        virtual void    ProcessCallbackCommandEvents( vtkObject*, unsigned long, void* );

    private:

        vtkCallbackCommand*                 modelModifiedCB;
        vtkCallbackCommand*                 progressCallback;
        vtkKWMultiColumnListWithScrollbars* imageList;
        int                                 activeIndex;

        static void                 UpdateProgress(vtkObject* subject, unsigned long, void* thisObject, void* callData);
        static void                 ModelModifiedCallback(vtkObject* subject, unsigned long eid, void* thisObject, void *calldata);


        sivicImageDataWidget(const sivicImageDataWidget&);   // Not implemented.
        void operator=(const sivicImageDataWidget&);  // Not implemented.
        
};

#endif //SVK_IMAGE_DATA_WIDGET_H
