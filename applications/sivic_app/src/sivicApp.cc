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
 */

#include <sivicApp.h>

#include <vtkJPEGReader.h>

/*! 
 *  Constructor
 */
sivicApp::sivicApp()
{
    this->model = NULL;

    // Creating our widgetView/Controller
    this->sivicController = NULL;
    this->sivicKWApp = NULL; 

    // For returnting the application status
    this->exitStatus            = 0;
    this->processingWidget      = sivicProcessingWidget::New();
    this->preprocessingWidget   = sivicPreprocessingWidget::New();
    this->quantificationWidget  = sivicQuantificationWidget::New();
    this->imageViewWidget       = sivicImageViewWidget::New();
    this->spectraViewWidget     = sivicSpectraViewWidget::New();
    this->windowLevelWidget     = sivicWindowLevelWidget::New();
    this->windowLevelWidget->SetSliderLabel("Image Window Level");
    this->overlayWindowLevelWidget = sivicWindowLevelWidget::New();
    this->overlayWindowLevelWidget->SetSliderLabel("Overlay Window Level");
    this->preferencesWidget     = sivicPreferencesWidget::New();
    this->spectraRangeWidget    = sivicSpectraRangeWidget::New();
    this->viewRenderingWidget   = sivicViewRenderingWidget::New();
    this->tabbedPanel           = vtkKWNotebook::New();
}


/*! 
 *  Destructor
 */
sivicApp::~sivicApp()
{
    // Deallocate and exit
    if( this->sivicKWApp != NULL ) {
        this->exitStatus = this->sivicKWApp->GetExitStatus();
    }

    if( this->sivicWindow != NULL ) {
        this->sivicWindow->Close();
    }

    if( this->sivicController != NULL ) {
        this->sivicController->Delete();
        this->sivicController = NULL;
    }


    if( this->viewRenderingWidget != NULL ) {
        this->viewRenderingWidget->Delete();
        this->viewRenderingWidget = NULL;
    }

    if( this->processingWidget != NULL ) {
        this->processingWidget->Delete();
        this->processingWidget = NULL;
    }

    if( this->preprocessingWidget != NULL ) {
        this->preprocessingWidget->Delete();
        this->preprocessingWidget = NULL;
    }

    if( this->quantificationWidget != NULL ) {
        this->quantificationWidget->Delete();
        this->quantificationWidget = NULL;
    }

    if( this->imageViewWidget != NULL ) {
        this->imageViewWidget->Delete();
        this->imageViewWidget = NULL;
    }


    if( this->spectraViewWidget != NULL ) {
        this->spectraViewWidget->Delete();
        this->spectraViewWidget = NULL;
    }

    if( this->windowLevelWidget != NULL ) {
        this->windowLevelWidget->Delete();
        this->windowLevelWidget = NULL;
    }

    if( this->preferencesWidget != NULL ) {
        this->preferencesWidget->Delete();
        this->preferencesWidget = NULL;
    }

    if( this->overlayWindowLevelWidget != NULL ) {
        this->overlayWindowLevelWidget->Delete();
        this->overlayWindowLevelWidget = NULL;
    }

    if( this->spectraRangeWidget != NULL ) {
        this->spectraRangeWidget->Delete();
        this->spectraRangeWidget = NULL;
    }

    if( this->model != NULL ) {
        this->model->Delete(); 
        this->model = NULL; 
    }

    if( this->sivicWindow != NULL ) {
        this->sivicWindow->Delete();
        this->sivicWindow = NULL;
    }

    if( this->sivicKWApp != NULL ) {
        this->sivicKWApp->Delete();
        this->sivicKWApp = NULL;
    }
/*
    if( this->uiPanel != NULL ) {
        this->uiPanel->Delete();
        this->uiPanel = NULL;
    }
*/

    if( this->tabbedPanel != NULL ) {
        this->tabbedPanel->Delete();
        this->tabbedPanel = NULL;
    }

}

/*!
 * Returns the View. The intent is to use this for testing purposes.
 */
vtkSivicController* sivicApp::GetView()
{
    return this->sivicController;
}


/*! 
 *  Build the sivicKWApp.
 */
int sivicApp::Build( int argc, char* argv[] )
{
    // First we have to initialize the tcl interpreter...
    Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, &cerr);
    if (!interp)
        {
        cerr << "Error: InitializeTcl failed" << endl ;
        return 1;
        }
        
    Sivickwcallbackslib_Init(interp);

    if( this->model != NULL ) {
        this->model->Delete();
        this->model = NULL;
    }
    this->model = svkDataModel::New();

    // Creating our widgetView/Controller
    if( this->sivicController != NULL ) {
        this->sivicController->Delete();
        this->sivicController = NULL;
    }
    this->sivicController = vtkSivicController::New();

    if( this->sivicKWApp != NULL ) {
        this->sivicKWApp->Delete();
        this->sivicKWApp = NULL;
    }
    this->sivicKWApp = vtkKWApplication::New();

    this->sivicKWApp->ReleaseModeOn();
    this->sivicController->SetApplication( this->sivicKWApp );
    this->sivicController->SetModel( this->model );

    // Create Application
    this->sivicKWApp->SetName("SIVIC");
    this->sivicKWApp->SetMajorVersion( SVK_MAJOR_VERSION );
    this->sivicKWApp->SetMinorVersion( SVK_MINOR_VERSION );
    this->sivicKWApp->SetVersionName( SVK_RELEASE_VERSION );

    // Add a Window to the application
    this->sivicWindow = vtkKWWindowBase::New();
    this->sivicKWApp->AddWindow(this->sivicWindow);
    this->sivicController->SetMainWindow( this->sivicWindow );
    this->sivicWindow->Create();
    this->sivicWindow->SetSize( WINDOW_SIZE_X, WINDOW_SIZE_Y);

    this->tabbedPanel->SetParent( this->sivicWindow->GetViewFrame() );
    this->tabbedPanel->SetApplication( this->sivicKWApp );
    this->tabbedPanel->Create( );


    this->viewRenderingWidget->SetParent(this->sivicWindow->GetViewFrame());
    this->viewRenderingWidget->SetPlotController(this->sivicController->GetPlotController());
    this->viewRenderingWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->viewRenderingWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->viewRenderingWidget->SetApplication( this->sivicKWApp );
    this->viewRenderingWidget->Create();

    vtkKWText* welcomeText = vtkKWText::New();
    welcomeText->SetParent( tabbedPanel );
    welcomeText->Create();
    this->GetWelcomeMessage( welcomeText );
    welcomeText->ReadOnlyOn(); 

    this->preprocessingWidget->SetParent(tabbedPanel );
    this->preprocessingWidget->SetPlotController(this->sivicController->GetPlotController());
    this->preprocessingWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->preprocessingWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->preprocessingWidget->SetSivicController(this->sivicController);
    this->preprocessingWidget->Create();

    this->processingWidget->SetParent(tabbedPanel );
    this->processingWidget->SetPlotController(this->sivicController->GetPlotController());
    this->processingWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->processingWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->processingWidget->SetSivicController(this->sivicController);
    this->processingWidget->Create();

    this->quantificationWidget->SetParent(tabbedPanel );
    this->quantificationWidget->SetPlotController(this->sivicController->GetPlotController());
    this->quantificationWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->quantificationWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->quantificationWidget->SetSivicController(this->sivicController);
    this->quantificationWidget->Create();

    this->imageViewWidget->SetParent(this->sivicWindow->GetViewFrame());
    this->imageViewWidget->SetPlotController(this->sivicController->GetPlotController());
    this->imageViewWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->imageViewWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->imageViewWidget->SetSivicController(this->sivicController);
    this->imageViewWidget->Create();

    this->spectraViewWidget->SetParent(this->sivicWindow->GetViewFrame());
    this->spectraViewWidget->SetPlotController(this->sivicController->GetPlotController());
    this->spectraViewWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->spectraViewWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->spectraViewWidget->SetSivicController(this->sivicController);
    this->spectraViewWidget->Create();

    this->spectraRangeWidget->SetParent(this->sivicWindow->GetViewFrame());
    this->spectraRangeWidget->SetPlotController(this->sivicController->GetPlotController());
    this->spectraRangeWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->spectraRangeWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->spectraRangeWidget->SetSivicController(this->sivicController);
    this->spectraRangeWidget->Create();

    this->windowLevelWidget->SetPlotController(this->sivicController->GetPlotController());
    this->windowLevelWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->windowLevelWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->windowLevelWidget->SetSivicController(this->sivicController);
    this->windowLevelWidget->SetWindowLevelTarget( svkOverlayViewController::REFERENCE_IMAGE );

    this->preferencesWidget->SetPlotController(this->sivicController->GetPlotController());
    this->preferencesWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->preferencesWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->preferencesWidget->SetSivicController(this->sivicController);

    this->overlayWindowLevelWidget->SetPlotController(this->sivicController->GetPlotController());
    this->overlayWindowLevelWidget->SetOverlayController(this->sivicController->GetOverlayController());
    this->overlayWindowLevelWidget->SetDetailedPlotController(this->sivicController->GetDetailedPlotController());
    this->overlayWindowLevelWidget->SetSivicController(this->sivicController);
    this->overlayWindowLevelWidget->SetWindowLevelTarget( svkOverlayViewController::IMAGE_OVERLAY );


    this->sivicController->SetViewRenderingWidget( viewRenderingWidget );
    this->sivicController->SetProcessingWidget( processingWidget );
    this->sivicController->SetPreprocessingWidget( preprocessingWidget );
    this->sivicController->SetQuantificationWidget( quantificationWidget );
    this->sivicController->SetImageViewWidget( imageViewWidget );
    this->sivicController->SetSpectraViewWidget( spectraViewWidget );
    this->sivicController->SetWindowLevelWidget( windowLevelWidget );
    this->sivicController->SetOverlayWindowLevelWidget( overlayWindowLevelWidget );
    this->sivicController->SetPreferencesWidget( preferencesWidget );
    this->sivicController->SetSpectraRangeWidget( spectraRangeWidget );

    this->tabbedPanel->AddPage("Welcome", "Welcome!", NULL);
    vtkKWWidget* welcomePanel = tabbedPanel->GetFrame("Welcome");

    this->tabbedPanel->AddPage("Preprocess", "Preprocessing.", NULL);
    vtkKWWidget* preprocessingPanel = tabbedPanel->GetFrame("Preprocess");

    this->tabbedPanel->AddPage("MRS Recon", "MRS recon.", NULL);
    vtkKWWidget* processingPanel = tabbedPanel->GetFrame("MRS Recon");

    this->tabbedPanel->AddPage("MRS Quantification", "MRS Quantification.", NULL);
    vtkKWWidget* quantificationPanel = tabbedPanel->GetFrame("MRS Quantification");

    vtkKWSeparator* separator = vtkKWSeparator::New();
    separator->SetParent(this->sivicWindow->GetViewFrame());
    separator->Create();
    separator->SetThickness(5);

    vtkKWSeparator* separator2 = vtkKWSeparator::New();
    separator2->SetParent(this->sivicWindow->GetViewFrame());
    separator2->Create();
    separator2->SetThickness(5);

    vtkKWSeparator* separatorVert = vtkKWSeparator::New();
    separatorVert->SetParent(this->sivicWindow->GetViewFrame());
    separatorVert->Create();
    separatorVert->SetThickness(3);

    vtkKWSeparator* separatorVert2 = vtkKWSeparator::New();
    separatorVert2->SetParent(this->sivicWindow->GetViewFrame());
    separatorVert2->Create();
    separatorVert2->SetThickness(3);

    this->sivicKWApp->Script("grid %s -row 0 -column 0 -columnspan 5 -sticky nsew", viewRenderingWidget->GetWidgetName());
    this->sivicKWApp->Script("grid %s -row 1 -column 0 -columnspan 5 -sticky nsew", separator->GetWidgetName());
    this->sivicKWApp->Script("grid %s -row 2 -column 0 -rowspan 3 -sticky wnse", imageViewWidget->GetWidgetName());
    this->sivicKWApp->Script("grid %s -row 2 -column 1 -sticky nsew -rowspan 3", separatorVert->GetWidgetName());

    this->sivicKWApp->Script("grid %s -row 2 -column 2 -sticky ensw -padx 2", tabbedPanel->GetWidgetName());
    this->sivicKWApp->Script("grid %s -row 3 -column 2 -sticky nsew -padx 2 -pady 2", spectraRangeWidget->GetWidgetName());
    this->sivicKWApp->Script("grid %s -row 4 -column 2 -sticky nsew -padx 2", spectraViewWidget->GetWidgetName());

    this->sivicKWApp->Script("grid %s -row 2 -column 3 -sticky nsew -rowspan 3", separatorVert2->GetWidgetName());

    this->sivicKWApp->Script("pack %s -side top -anchor nw -expand y -fill both -padx 2 -pady 2 -in %s", 
              welcomeText->GetWidgetName(), welcomePanel->GetWidgetName());
    this->sivicKWApp->Script("pack %s -side top -anchor nw -expand y -fill both -padx 4 -pady 2 -in %s", 
              this->preprocessingWidget->GetWidgetName(), preprocessingPanel->GetWidgetName());
    this->sivicKWApp->Script("pack %s -side top -anchor nw -expand y -fill both -padx 4 -pady 2 -in %s", 
              this->processingWidget->GetWidgetName(), processingPanel->GetWidgetName());
    this->sivicKWApp->Script("pack %s -side top -anchor nw -expand y -fill both -padx 4 -pady 2 -in %s", 
              this->quantificationWidget->GetWidgetName(), quantificationPanel->GetWidgetName());

    //  row 0 -> render view
    //  row 1 -> seperator
    //  row 2 -> tab processing    
    //  row 3 -> spec range    
    //  row 4 -> spec view
    this->sivicKWApp->Script("grid rowconfigure    %s 0 -weight 100  -minsize 300 ", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    this->sivicKWApp->Script("grid rowconfigure    %s 1 -weight 0    -minsize 3   ", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    this->sivicKWApp->Script("grid rowconfigure    %s 2 -weight 0    -minsize 137 -maxsize 137", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    this->sivicKWApp->Script("grid rowconfigure    %s 3 -weight 0    -minsize 20  -maxsize 20", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    this->sivicKWApp->Script("grid rowconfigure    %s 4 -weight 0    -minsize 20  -maxsize 20", this->sivicWindow->GetViewFrame()->GetWidgetName() );

    this->sivicKWApp->Script("grid columnconfigure %s 0 -weight 50 -uniform 1 -minsize 350 ", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    this->sivicKWApp->Script("grid columnconfigure %s 1 -weight 0 -minsize 5", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    this->sivicKWApp->Script("grid columnconfigure %s 2 -weight 50 -uniform 1 -minsize 300", this->sivicWindow->GetViewFrame()->GetWidgetName() );
    separator->Delete();
    separatorVert->Delete();
    separatorVert2->Delete();
    welcomeText->Delete();


    // Create Toolbar
    vtkKWToolbar* toolbar = vtkKWToolbar::New();
    toolbar->SetName("Main Toolbar"); 
    toolbar->SetParent(this->sivicWindow->GetMainToolbarSet()->GetToolbarsFrame()); 
    toolbar->Create();
    PopulateMainToolbar( toolbar );

    // Add toolbar to window
    this->sivicWindow->GetMainToolbarSet()->AddToolbar( toolbar );
    this->sivicWindow->GetMainToolbarSet()->ShowToolbar( toolbar );

    toolbar->Delete();

    this->sivicWindow->Display();
#if defined( UCSF_INTERNAL )
    vtkKWMenu* ucsfMenu = vtkKWMenu::New();

    ucsfMenu->SetParent(this->sivicKWApp->GetNthWindow(0)->GetMenu());
    ucsfMenu->Create();
    this->sivicKWApp->GetNthWindow(0)->GetMenu()->AddCascade("UCSF", ucsfMenu);

    //  Add menu of UCSF metabolite maps:
    vtkKWMenu* ucsfMetaboliteMenu = vtkKWMenu::New();
    ucsfMetaboliteMenu->SetParent(ucsfMenu);
    ucsfMetaboliteMenu->Create();
    ucsfMenu->AddCascade("Load Metabolite Map", ucsfMetaboliteMenu);
    ucsfMenu->InsertCommand( 0, "&Send To PACS", this->sivicController, "PushToPACS" );

#endif


    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            0, "&Save Data", this->sivicController, "SaveData");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            1, "&Save Spectra Secondary Capture", this->sivicController, "SaveSecondaryCapture SPECTRA_CAPTURE");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            2, "&Save Image Secondary Capture", this->sivicController, "SaveSecondaryCapture IMAGE_CAPTURE");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            3, "&Save Combined Secondary Capture", this->sivicController, "SaveSecondaryCapture SPECTRA_WITH_OVERVIEW_CAPTURE");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            4, "&Save Metabolite Maps", this->sivicController, "SaveMetaboliteMaps");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            5, "&Open 1 Channel of Spectra", this->sivicController, "OpenFile spectra_one_channel NULL 1");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            6, "&Print Current Slice", this->sivicController, "Print COMBINED_CAPTURE 1");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            7, "&Print All Slices", this->sivicController, "Print COMBINED_CAPTURE 0");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            8, "&Print Images Only", this->sivicController, "Print IMAGE_CAPTURE 0");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            9, "&Save Session", this->sivicController, "SaveSession");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            10, "&Restore Session", this->sivicController, "RestoreSession");
    this->sivicKWApp->GetNthWindow(0)->GetFileMenu()->InsertCommand(
            11, "&Close All", this->sivicController, "ResetApplication");
    this->sivicKWApp->GetNthWindow(0)->GetHelpMenu()->InsertCommand(
            12, "&Sivic Help Resources", this->sivicController, "DisplayInfo");

    // Tools menu
    this->sivicKWApp->GetNthWindow(0)->GetWindowMenu()->InsertCommand(
            0, "&Show Window Level", this->sivicController, "DisplayWindowLevelWindow");
    //this->sivicKWApp->GetNthWindow(0)->GetWindowMenu()->InsertCommand(
    //        1, "&Preferences", this->sivicController, "DisplayPreferencesWindow");
#if defined(DEBUG_BUILD)
    this->sivicKWApp->GetNthWindow(0)->GetHelpMenu()->InsertCommand(
            1, "&Run Tests", this->sivicController, "RunTestingSuite");
#endif
    this->sivicKWApp->SetHelpDialogStartingPage("http://sivic.sourceforge.net");
    this->sivicWindow->Display();

	string pathName = svkUtils::GetCurrentWorkingDirectory();
    this->sivicKWApp->SetRegistryValue( 0, "RunTime", "lastPath", pathName.c_str() );


}

//! Populates the welcome message. 
void sivicApp::GetWelcomeMessage(vtkKWText* text)
{
    ostringstream* oss = new ostringstream();
    *oss << "**Welcome to ";
    *oss << this->sivicKWApp->GetName() << " " << this->sivicKWApp->GetVersionName() << "!**" << endl; 
    *oss << endl << "Home Page:" << endl << "  http://sivic.sourceforge.net" << endl;; 
    *oss << endl << "User's Guide:" << endl << "  http://sourceforge.net/apps/trac/sivic/wiki/users_guide" << endl;; 
    text->QuickFormattingOn(); 
    text->SetText( oss->str().c_str()); 
    delete oss;
}


//! Populates the main toolbar with buttons
void sivicApp::PopulateMainToolbar(vtkKWToolbar* toolbar)
{

    // Create Open image Button
    vtkKWPushButton* openExamButton = vtkKWPushButton::New();
    openExamButton->SetParent( toolbar->GetFrame() );
    openExamButton->Create();
    openExamButton->SetText("exam");
    openExamButton->SetCompoundModeToLeft();
    openExamButton->SetImageToPredefinedIcon( vtkKWIcon::IconFileOpen );
    openExamButton->SetCommand( this->sivicController, "OpenExam");
    openExamButton->SetBalloonHelpString( "Open an exam." );
    toolbar->AddWidget( openExamButton );

    // Create Open image Button
    vtkKWPushButton* openIdfButton = vtkKWPushButton::New();
    openIdfButton->SetParent( toolbar->GetFrame() );
    openIdfButton->Create();
    openIdfButton->SetText("image");
    openIdfButton->SetCompoundModeToLeft();
    openIdfButton->SetImageToPredefinedIcon( vtkKWIcon::IconFileOpen );
    openIdfButton->SetCommand( this->sivicController, "OpenFile image NULL 0");
    openIdfButton->SetBalloonHelpString( "Open a image file." );
    toolbar->AddWidget( openIdfButton );

    // Create Open spectra Button
    vtkKWPushButton* openDdfButton = vtkKWPushButton::New();
    openDdfButton->SetParent( toolbar->GetFrame() );
    openDdfButton->Create();
    openDdfButton->SetReliefToGroove();
    openDdfButton->SetText("spectra");
    openDdfButton->SetCompoundModeToLeft();
    openDdfButton->SetImageToPredefinedIcon( vtkKWIcon::IconFileOpen ); 
    openDdfButton->SetCommand( this->sivicController, "OpenFile spectra NULL 0");
    openDdfButton->SetBalloonHelpString( "Open a spectra file." );
    toolbar->AddWidget( openDdfButton );

    // Create Open metabolite/overlay Button
    vtkKWPushButton* openMetButton = vtkKWPushButton::New();
    openMetButton->SetParent( toolbar->GetFrame() );
    openMetButton->Create();
    openMetButton->SetReliefToGroove();
    openMetButton->SetText("overlay");
    openMetButton->SetCompoundModeToLeft();
    openMetButton->SetImageToPredefinedIcon( vtkKWIcon::IconFileOpen );
    openMetButton->SetCommand( this->sivicController, "OpenFile overlay NULL 0");
    openMetButton->SetBalloonHelpString( "Open an image to overlay or a metabolite file." );
    toolbar->AddWidget( openMetButton );

    // Create secondary capture Button
    vtkKWPushButton* scButton = vtkKWPushButton::New();
    scButton->SetParent( toolbar->GetFrame() );
    scButton->Create();
    scButton->SetImageToPredefinedIcon( vtkKWIcon::IconCamera ); 
    scButton->SetCommand( this->sivicController, "SaveSecondaryCapture COMBINED_CAPTURE");
    scButton->SetBalloonHelpString( "Take a secondary capture." );
    toolbar->AddWidget( scButton );

    // Create Selection Style 
    vtkKWPushButton* selectionButton = vtkKWPushButton::New();
    selectionButton->SetParent( toolbar->GetFrame() );
    selectionButton->Create();
    selectionButton->SetImageToPredefinedIcon( vtkKWIcon::IconGridLinear ); 
    selectionButton->SetCommand( this->sivicController, "UseSelectionStyle");
    selectionButton->SetBalloonHelpString( "Switch to voxel selection interactor." );
    toolbar->AddWidget( selectionButton );

    // Create Window Level Style 
    vtkKWPushButton* wlButton = vtkKWPushButton::New();
    wlButton->SetParent( toolbar->GetFrame() );
    wlButton->Create();
    wlButton->SetImageToPredefinedIcon( vtkKWIcon::IconWindowLevel ); 
    wlButton->SetCommand( this->sivicController, "UseWindowLevelStyle");
    wlButton->SetBalloonHelpString( "Switch to window level interactor." );
    toolbar->AddWidget( wlButton );

    // Create color overlay Style 
    vtkKWPushButton* colorOverlayButton = vtkKWPushButton::New();
    colorOverlayButton->SetParent( toolbar->GetFrame() );
    colorOverlayButton->Create();
    colorOverlayButton->SetImageToPredefinedIcon( vtkKWIcon::IconColorSquares ); 
    colorOverlayButton->SetCommand( this->sivicController, "UseColorOverlayStyle");
    colorOverlayButton->SetBalloonHelpString( "Switch to color overlay window level interactor." );
    toolbar->AddWidget( colorOverlayButton );

    // Create Window Level Reset 
    vtkKWPushButton* wlResetButton = vtkKWPushButton::New();
    wlResetButton->SetParent( toolbar->GetFrame() );
    wlResetButton->Create();
    wlResetButton->SetImageToPredefinedIcon( vtkKWIcon::IconDocumentWindowLevel ); 
    wlResetButton->SetCommand( this->sivicController, "ResetWindowLevel");
    wlResetButton->SetBalloonHelpString( "Reset window level." );
    toolbar->AddWidget( wlResetButton );

    // Create Window Voxel Selection Reset 
    vtkKWPushButton* vsResetButton = vtkKWPushButton::New();
    vsResetButton->SetParent( toolbar->GetFrame() );
    vsResetButton->Create();
    //vsResetButton->SetImageToPredefinedIcon( vtkKWIcon::IconCameraMini ); 
    vsResetButton->SetImageToPredefinedIcon( vtkKWIcon::IconResetCamera ); 
    vsResetButton->SetCommand( this->sivicController, "HighlightSelectionBoxVoxels");
    vsResetButton->SetBalloonHelpString( "Highlight the voxels within the selection box of the current slice." );
    toolbar->AddWidget( vsResetButton );

    // Create Rotation Style 
    vtkKWPushButton* rotateButton = vtkKWPushButton::New();
    rotateButton->SetParent( toolbar->GetFrame() );
    rotateButton->Create();
    //rotateButton->SetImageToPredefinedIcon( vtkKWIcon::IconRotate ); 
    rotateButton->SetImageToPredefinedIcon( vtkKWIcon::IconCrystalProject16x16ActionsRotate );
    rotateButton->SetCommand( this->sivicController, "UseRotationStyle");
    rotateButton->SetBalloonHelpString( "Switch to 3D rotation interactor." );
    toolbar->AddWidget( rotateButton );

    // Create Open image Button
    vtkKWPushButton* reloadPreferences = vtkKWPushButton::New();
    reloadPreferences->SetParent( toolbar->GetFrame() );
    reloadPreferences->Create();
    reloadPreferences->SetCompoundModeToLeft();
    reloadPreferences->SetImageToPredefinedIcon( vtkKWIcon::IconHSVDiagram );
    reloadPreferences->SetCommand( this->sivicController, "SetPreferencesFromRegistry");
    reloadPreferences->SetBalloonHelpString( "Reload Preferences." );
    //toolbar->AddWidget( reloadPreferences );

    // Create Orientation Selector Menu
    vtkKWPushButtonWithMenu* orientationButton = vtkKWPushButtonWithMenu::New();
    orientationButton->SetParent( toolbar->GetFrame() );
    orientationButton->Create();
    orientationButton->GetMenuButton()->SetImageToPredefinedIcon( vtkKWIcon::IconAngleTool);
    orientationButton->SetBalloonHelpString( "Select the orientation to view."); 
    vtkKWMenu* orientationMenu = orientationButton->GetMenu();
    orientationMenu->AddRadioButton("Axial", this->sivicController, "SetOrientation AXIAL 1"); 
    orientationMenu->AddRadioButton("Coronal", this->sivicController, "SetOrientation CORONAL 1"); 
    orientationMenu->AddRadioButton("Sagittal", this->sivicController, "SetOrientation SAGITTAL 1"); 
    toolbar->AddWidget( orientationButton );


#if defined(Darwin)
    //  Create OsiriX buttons
    vtkKWPushButton* osirixSCButton = vtkKWPushButton::New();
    osirixSCButton->SetParent( toolbar->GetFrame() );
    osirixSCButton->Create();
    osirixSCButton->SetText( "OsiriX SC");
    osirixSCButton->SetCommand( this->sivicController, "SaveSecondaryCaptureOsiriX"); 
    osirixSCButton->SetBalloonHelpString( "Save DICOM Secondary Capture data to OsiriX" );
    toolbar->AddWidget( osirixSCButton );

    vtkKWPushButton* osirixMRSButton = vtkKWPushButton::New();
    osirixMRSButton->SetParent( toolbar->GetFrame() );
    osirixMRSButton->Create();
    osirixMRSButton->SetText( "OsiriX MRS");
    osirixMRSButton->SetCommand( this->sivicController, "SaveDataOsiriX"); 
    osirixMRSButton->SetBalloonHelpString( "Save DICOM MRS data to OsiriX" );
    toolbar->AddWidget( osirixMRSButton );

    vtkKWPushButton* osirixMetMapButton = vtkKWPushButton::New();
    osirixMetMapButton->SetParent( toolbar->GetFrame() );
    osirixMetMapButton->Create();
    osirixMetMapButton->SetText( "OsiriX Met Maps");
    osirixMetMapButton->SetCommand( this->sivicController, "SaveMetMapDataOsiriX"); 
    osirixMetMapButton->SetBalloonHelpString( "Save DICOM metabolite map data to OsiriX" );
    toolbar->AddWidget( osirixMetMapButton );
#endif


}


/*! 
 *  Start the application.
 */
int sivicApp::Start( int argc, char* argv[] )
{

    //  Preparse the MR Image objects to try and detect which is an overlay:
    //  Assume that the reference image is higher res than the overlay
    int refImageIndex = -1; 
    int overlayImageIndex = -1; 
    int refRows = 0; 
    int refColumns = 0; 
    int overlayRows = 10000000; 
    int overlayColumns = 10000000; 
    for (int i=1 ; i < argc; i++) {

        svkImageData* tmp = svkMriImageData::New();
        tmp->GetDcmHeader()->ReadDcmFile( argv[i] );
        string SOPClassUID = tmp->GetDcmHeader()->GetStringValue( "SOPClassUID" ) ;
        //  Check MRImage Storage and Enhanced MRImage Storage
        if ( SOPClassUID == "1.2.840.10008.5.1.4.1.1.4" || SOPClassUID == "1.2.840.10008.5.1.4.1.1.4.1" ) {
            int rows = tmp->GetDcmHeader()->GetIntValue( "Rows" ) ;
            int columns = tmp->GetDcmHeader()->GetIntValue( "Columns" ) ;
            //cout << "check : " << rows << " vs " << refRows << " " << columns << " vs " << refColumns << endl;
            if ( rows > refRows && columns > refColumns) {
                refRows = rows; 
                refColumns = columns; 
                refImageIndex = i; 
            } 
            if ( rows < overlayRows && columns < overlayColumns) {
                overlayRows = rows; 
                overlayColumns = columns; 
                overlayImageIndex = i; 
            }
        }
        tmp->Delete();
    }

    vtkstd::vector< int >   loadOrder; 
    for (int i = 1; i < argc; i++) {
        if ( i != overlayImageIndex ) {
            loadOrder.push_back(i);
        }
    }
    //  Load overlay last:
    loadOrder.push_back(overlayImageIndex);

    for (int i=1 ; i < argc; i++) {
        //cout << "loading order: " << loadOrder[i-1] << endl;
    }

    //  Check each argv and set the load file type 
    for (int i=1 ; i < argc; i++) {

        //cout << " load order: " << i << " " << loadOrder[i] <<  argv[ loadOrder[i-1] ] << endl;
        svkImageData* tmp = svkMriImageData::New();
        tmp->GetDcmHeader()->ReadDcmFile( argv[ loadOrder[i-1] ] );
        string SOPClassUID = tmp->GetDcmHeader()->GetStringValue( "SOPClassUID" ) ;
        tmp->Delete();

        if ( SOPClassUID == "1.2.840.10008.5.1.4.1.1.4" || SOPClassUID == "1.2.840.10008.5.1.4.1.1.4.1" ) {

            if ( loadOrder[i-1] == refImageIndex ) {
                this->GetView()->OpenFile("command_line_image", argv[ loadOrder[i-1] ]); 
            } else {
                this->GetView()->OpenFile("command_line_overlay", argv[ loadOrder[i-1] ]); 
            }

        } else if ( SOPClassUID == "1.2.840.10008.5.1.4.1.1.4.2" ) {
            this->GetView()->OpenFile("command_line_spectra", argv[ loadOrder[i-1] ]); 
        }
    }
    
    // model is used to manage data that has been loaded 
    this->sivicKWApp->Start(argc, argv);
    return this->exitStatus;
}
