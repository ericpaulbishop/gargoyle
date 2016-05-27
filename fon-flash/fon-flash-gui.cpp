#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 
#endif

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/choice.h>
#include <wx/arrstr.h>
#include <wx/cmdline.h>
#include <wx/init.h>

#define SELECT_FIRMWARE_ID	400
#define FILE1_BROWSE_BUTTON_ID	500
#define FILE2_BROWSE_BUTTON_ID  600
#define FILE3_BROWSE_BUTTON_ID  700
#define FLASH_ROUTER_BUTTON_ID  800


int gui_printf(const char* format, ...);
int gui_fprintf(FILE* stream, const char* format, ...);
void gui_exit(int status);

#define printf gui_printf
#define fprintf gui_fprintf
#define exit gui_exit
#define GUI

#include "fon-flash.cpp"
#include "fon-flash.xpm"



// how to declare a custom event. this can go in a header
DECLARE_EVENT_TYPE(wxEVT_THREAD, -1)
 
// how to define the custom event
DEFINE_EVENT_TYPE(wxEVT_THREAD)


class WorkerThread: public wxThread
{
	protected:
		wxEvtHandler* parentHandler;
		int inProgress;
		char* dev;
		flash_configuration* config;
		char* file1;
		char* file2;
		char* file3;
	public:
    		WorkerThread(wxEvtHandler* pParent, char* d, flash_configuration* c, char* f1, char* f2, char* f3) : wxThread(), parentHandler(pParent)
		{
			inProgress = 0; 
			dev = d;
			config = c;
			file1 = f1;
			file2 = f2;
			file3 = f3;
		}
    		wxThread::ExitCode Entry();
		void DoPrint(wxString s);
		
};

wxThread::ExitCode WorkerThread::Entry()
{

	inProgress = 1;

	if(file_1_buf != NULL)
	{
		free(file_1_buf);
		file_1_buf = NULL;
	}
	if(file_2_buf != NULL)
	{
		free(file_2_buf);
		file_2_buf = NULL;
	}
	if(file_3_buf != NULL)
	{
		free(file_3_buf);
		file_3_buf = NULL;
	}

	initialize_buffers_from_files(file1, file2, file3);
	fon_flash(config, dev);	

	//parameters are dynamicall allocated, free them
	free(dev);
	free(file1);
	free(file2);
	free(file3);
	
	inProgress = 0;
	return EXIT_SUCCESS;
}
void WorkerThread::DoPrint(wxString s)
{
	if(inProgress == 1)
	{
		wxCommandEvent evt(wxEVT_THREAD, wxID_ANY);
		evt.SetString( s );
		parentHandler->AddPendingEvent(evt);
	}
}





class ImagePanel : public wxPanel
{    
	public:
		ImagePanel(wxFrame* parent);
		void paintEvent(wxPaintEvent& evt);
		
		DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(ImagePanel, wxPanel)
	EVT_PAINT(ImagePanel::paintEvent)
END_EVENT_TABLE()

ImagePanel::ImagePanel(wxFrame* parent) : wxPanel(parent) { }

void ImagePanel::paintEvent(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
	wxBitmap fonFlashLogo(fon_flash_xpm);
	dc.DrawBitmap(fonFlashLogo, 215, 0, true);
}




class MainFrame : public wxFrame
{
	public:
		MainFrame(wxApp* a, const wxString& title);

		void SetFirmware(wxCommandEvent& evt);

		void OpenFile1Dialog(wxCommandEvent& evt);
		void OpenFile2Dialog(wxCommandEvent& evt);
		void OpenFile3Dialog(wxCommandEvent& evt);
		
		void FlashRouter(wxCommandEvent& evt);
		void OnThreadMessage(wxCommandEvent& evt);
	
		flash_configuration* gargoyle_conf;
		flash_configuration* fonera_conf;
		flash_configuration* ddwrt_conf;
		flash_configuration* current_conf;

		wxApp*   app;
		wxPanel* panel;

		wxChoice      *firmwareChoice; 
		wxChoice      *interfaceChoice;
		wxArrayString *devNames;

		wxStaticText *file1Label;
		wxTextCtrl   *file1Text;
		wxButton     *file1Button;
		
		wxStaticText *file2Label;
		wxTextCtrl   *file2Text;
		wxButton     *file2Button;

		wxStaticText *file3Label;
		wxTextCtrl   *file3Text;
		wxButton     *file3Button;

		wxButton     *startButton;

		wxTextCtrl *outputText;
		WorkerThread* testThread;
	
		DECLARE_EVENT_TABLE()
};



BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_CHOICE(SELECT_FIRMWARE_ID, MainFrame::SetFirmware)
	EVT_BUTTON(FILE1_BROWSE_BUTTON_ID, MainFrame::OpenFile1Dialog)
	EVT_BUTTON(FILE2_BROWSE_BUTTON_ID, MainFrame::OpenFile2Dialog)
	EVT_BUTTON(FILE3_BROWSE_BUTTON_ID, MainFrame::OpenFile3Dialog)
	EVT_BUTTON(FLASH_ROUTER_BUTTON_ID, MainFrame::FlashRouter)
	EVT_COMMAND(wxID_ANY, wxEVT_THREAD, MainFrame::OnThreadMessage)
END_EVENT_TABLE()


MainFrame *globalAppFrame = NULL;



MainFrame::MainFrame(wxApp* a, const wxString& title) : wxFrame(NULL, -1, title, wxDefaultPosition, wxSize(560, 590))
{
	app = a;
	Centre();

#ifdef WIN32
	SetIcon(wxICON(fon_flash_icon));
#endif
	
	panel = new ImagePanel(this);
	testThread = NULL;	

	gargoyle_conf = get_gargoyle_configuration();
	fonera_conf   = get_fonera_configuration();
	ddwrt_conf    = get_ddwrt_configuration();
	current_conf = NULL;

	wxArrayString* firmwareTypes = new wxArrayString();
	firmwareTypes->Add(wxT("OpenWrt / Gargoyle"));
	firmwareTypes->Add(wxT("Fonera Firmware"));
	firmwareTypes->Add(wxT("DD-WRT"));

	wxStaticText *firmwareLabel = new wxStaticText(panel,-1,wxT("Select Firmware Type:"),wxPoint(3,30));
	firmwareChoice = new wxChoice(panel, SELECT_FIRMWARE_ID, wxPoint(3,50), wxSize(195,25), *firmwareTypes, 0, wxDefaultValidator);
	firmwareChoice->SetSelection(0);
	
	
	wxArrayString *devChoiceNames = new wxArrayString();
	devNames = new wxArrayString();
	pcap_if_t *alldevs;
	pcap_if_t *d;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_findalldevs(&alldevs, errbuf);
	int selectedDevIndex = 0;
	int nextDevIndex = 0;
	for(d= alldevs; d != NULL; d= d->next)
	{
		if(strstr(d->name, "lo") != d->name)
		{
			if (d->description)
			{
				wxString s = wxString::FromAscii(d->description);
				devChoiceNames->Add( wxString::FromAscii(d->description), 1);
				if(strstr(d->description, "VPN") != NULL && selectedDevIndex == nextDevIndex)
				{
					selectedDevIndex++;
				}
			}
			else
			{
				devChoiceNames->Add(wxString::FromAscii(d->name), 1);
			}
			devNames->Add(wxString::FromAscii(d->name), 1);
			nextDevIndex++;
		}
	}
	if(selectedDevIndex == nextDevIndex)
	{
		selectedDevIndex = 0;
	}

	wxStaticText *interfaceLabel = new wxStaticText(panel,-1,wxT("Select Network Interface:"),wxPoint(5,115));

#ifdef WIN32
	interfaceChoice = new wxChoice(panel, -1, wxPoint(5,135), wxSize(450,25), *devChoiceNames, 0, wxDefaultValidator);
#else
	interfaceChoice = new wxChoice(panel, -1, wxPoint(5,135), wxSize(195,25), *devChoiceNames, 0, wxDefaultValidator);
#endif
	interfaceChoice->SetSelection(selectedDevIndex);


		
	file1Label = new wxStaticText(panel,-1,wxT("Select File 1:"),wxPoint(5,185), wxSize(200, 20));
	file1Text = new wxTextCtrl( 
		panel,
		5,
		wxString(wxT("")),
		wxPoint(5, 205),
		wxSize(450, 25),
		0);
	file1Button = new wxButton(panel, FILE1_BROWSE_BUTTON_ID, wxT("Browse"), wxPoint(455,205), wxSize(85, 25));

	file2Label = new wxStaticText(panel,-1,wxT("Select File 2:"),wxPoint(5,250), wxSize(200, 20));
	file2Text = new wxTextCtrl( 
		panel,
		5,
		wxString(wxT("")),
		wxPoint(5, 270),
		wxSize(450, 25),
		0);
	file2Button = new wxButton(panel, FILE2_BROWSE_BUTTON_ID, wxT("Browse"), wxPoint(455,270), wxSize(85, 25));
	
	file3Label = new wxStaticText(panel,-1,wxT("Select File 3:"),wxPoint(5,315), wxSize(200, 20));
	file3Text = new wxTextCtrl( 
		panel,
		5,
		wxString(wxT("")),
		wxPoint(5, 335),
		wxSize(450, 25),
		0);
	file3Button = new wxButton(panel, FILE3_BROWSE_BUTTON_ID, wxT("Browse"), wxPoint(455,335), wxSize(85, 25));


	outputText = new wxTextCtrl( 
		panel,
		5,
		wxString(wxT("")),
		wxPoint(5, 380),
		wxSize(535, 130),
		wxTE_READONLY | wxTE_MULTILINE);

	startButton = new wxButton(panel, FLASH_ROUTER_BUTTON_ID, wxT("Flash Router Now!"), wxPoint(75,520), wxSize(400, 25));

	wxCommandEvent evt;
	SetFirmware(evt);
}


void MainFrame::SetFirmware(wxCommandEvent& evt)
{
	int selection = firmwareChoice->GetCurrentSelection();
	if(selection == 0) //openwrt or gargoyle
	{
		file1Label->SetLabel(wxT("Select Rootfs File:"));
		file2Label->SetLabel(wxT("Select Kernel File:"));
		
		file2Label->Show(true);
		file2Text->Show(true);
		file2Button->Show(true);

		
		file3Label->Show(false);
		file3Text->Show(false);
		file3Button->Show(false);

		current_conf = gargoyle_conf;
	}
	else if(selection==1) //fonera
	{
		file1Label->SetLabel(wxT("Select Loader File:"));
		file2Label->SetLabel(wxT("Select Image File:"));
		file3Label->SetLabel(wxT("Select Image2 File:"));

		file2Label->Show(true);
		file2Text->Show(true);
		file2Button->Show(true);

		file3Label->Show(true);
		file3Text->Show(true);
		file3Button->Show(true);
		
		current_conf = fonera_conf;
	}
	else if(selection == 2)
	{
		file1Label->SetLabel(wxT("Select Firmware File:"));
		
		file2Label->Show(false);
		file2Text->Show(false);
		file2Button->Show(false);

		file3Label->Show(false);
		file3Text->Show(false);
		file3Button->Show(false);

		current_conf = ddwrt_conf;
	}
}

void MainFrame::OpenFile1Dialog(wxCommandEvent& evt)
{
	wxFileDialog* selectFile = new wxFileDialog(NULL, wxT("Select File"), wxT(""), wxT(""), wxT("*"), wxOPEN, wxDefaultPosition);
	if( selectFile->ShowModal() == wxID_OK )
	{
		wxString path;
		path.append( selectFile->GetDirectory() );
		path.append( wxFileName::GetPathSeparator() );
		path.append( selectFile->GetFilename() );
		file1Text->SetValue(path);
	}
}
void MainFrame::OpenFile2Dialog(wxCommandEvent& evt)
{
	wxFileDialog* selectFile = new wxFileDialog(NULL, wxT("Select File"), wxT(""), wxT(""), wxT("*"), wxOPEN, wxDefaultPosition);
	if( selectFile->ShowModal() == wxID_OK  )
	{
		wxString path;
		path.append( selectFile->GetDirectory() );
		path.append( wxFileName::GetPathSeparator() );
		path.append( selectFile->GetFilename() );
		file2Text->SetValue(path);
	}
}
void MainFrame::OpenFile3Dialog(wxCommandEvent& evt)
{
	wxFileDialog* selectFile = new wxFileDialog(NULL, wxT("Open Kernel File"), wxT(""), wxT(""), wxT("*"), wxOPEN, wxDefaultPosition);
	if( selectFile->ShowModal() == wxID_OK  )
	{
		wxString path;
		path.append( selectFile->GetDirectory() );
		path.append( wxFileName::GetPathSeparator() );
		path.append( selectFile->GetFilename() );
		file3Text->SetValue(path);
	}
}


void MainFrame::FlashRouter(wxCommandEvent& evt)
{
	int firmwareSelection = firmwareChoice->GetCurrentSelection();
	int is_valid = 1;

	//test that files have been selected and that they exist
	wxString file1Path = file1Text->GetValue();
	wxString file2Path = file2Text->GetValue();
	wxString file3Path = file3Text->GetValue();
	
	FILE *test1 = fopen(file1Path.ToAscii(), "r");
	wxString errorString;
	if(test1 == NULL)
	{
		is_valid = 0;
		errorString = firmwareSelection == 1 ? 
				wxT("ERROR: The specified loader file does not exist.\n\nPlease indicate the correct location of the necessary files and try again.") :
				wxT("ERROR: The specified rootfs file does not exist.\n\nPlease indicate the correct location of the necessary files and try again.");
		errorString = firmwareSelection == 2 ?
				wxT("ERROR: The specified firmware file does not exist.\n\nPlease indicate the correct location of the necessary files and try again.") :
				errorString;

	
		//print error
		wxMessageDialog *errorDialog = new wxMessageDialog(NULL, errorString, wxT("Error"), wxOK | wxICON_ERROR);
		errorDialog->ShowModal();

	}
	else
	{
		fclose(test1);
	}


	FILE *test2 = firmwareSelection != 2 ? fopen(file2Path.ToAscii(), "r") : NULL;
	if(test2 == NULL && is_valid && firmwareSelection != 2)
	{
		is_valid = 0;
		errorString = firmwareSelection == 1 ? 
				wxT("ERROR: The specified image file does not exist.\n\nPlease indicate the correct location of the necessary files and try again.") :
				wxT("ERROR: The specified kernel file does not exist.\n\nPlease indicate the correct location of the necessary files and try again.");
	
		//print error
		wxMessageDialog *errorDialog = new wxMessageDialog(NULL, errorString, wxT("Error"), wxOK | wxICON_ERROR);
		errorDialog->ShowModal();

	}
	else if(test2 != NULL)
	{
		fclose(test2);
	}

	
	
	FILE* test3 = firmwareSelection == 1 ? fopen(file3Path.ToAscii(), "r") : NULL;
	if(test3 == NULL && firmwareSelection == 1 && is_valid)
	{
		is_valid = 0;
		errorString = 	wxT("ERROR: The specified image2 file does not exist.\n\nPlease indicate the correct location of the necessary files and try again.");
	
		//print error
		wxMessageDialog *errorDialog = new wxMessageDialog(NULL, errorString, wxT("Error"), wxOK | wxICON_ERROR);
		errorDialog->ShowModal();

	}
	else if(test3 != NULL)
	{
		fclose(test3);
	}


	if(is_valid)
	{
		//disable controls, so everything is now fixed
		firmwareChoice->Enable(false);
		interfaceChoice->Enable(false);
		file1Button->Enable(false);
		file2Button->Enable(false);
		file3Button->Enable(false);
		startButton->Enable(false);
	
		//set text as readonly
		wxString old1 = file1Text->GetValue();
		delete(file1Text);
		file1Text = new wxTextCtrl( 
			this->panel,
			5,
			old1,
			wxPoint(5, 205),
			wxSize(450, 30),
			wxTE_READONLY);
	
		if(firmwareSelection != 2)
		{	
			wxString old2 = file2Text->GetValue();
			delete(file2Text);
			file2Text = new wxTextCtrl( 
				this->panel,
				5,
				old2,
				wxPoint(5, 270),
				wxSize(450, 30),
				wxTE_READONLY);
		}
		if(firmwareSelection == 1)
		{
			wxString old3 = file3Text->GetValue();
			delete(file3Text);
			file3Text = new wxTextCtrl( 
				this->panel,
				5,
				old3,
				wxPoint(5, 335),
				wxSize(450, 30),
				wxTE_READONLY);
		}

		//actually flash
		outputText->SetValue(wxT(""));
		char* dev = strdup( (devNames->Item(interfaceChoice->GetCurrentSelection())).ToAscii() );
		
		
		char* file1 = strdup(file1Path.ToAscii());
		char* file2 = firmwareSelection != 2 ? strdup(file2Path.ToAscii()) : NULL;
		char* file3 = firmwareSelection == 1 ? strdup(file3Path.ToAscii()) : NULL;

		int is_gz = ends_with(file1, ".gz") == 1 || ends_with(file2, ".gz") == 1 || ends_with(file3, ".gz") == 1 ? 1 : 0;
		if(is_gz == 1)
		{
			char** boot = current_conf->bootloader_lines;
			int boot_index=0;
			for(boot_index=0; boot[boot_index] != NULL; boot_index++)
			{
				if(strstr(boot[boot_index], "fis load -l") != NULL)
				{
					char* old_boot = boot[boot_index];
					char* boot_end = old_boot + strlen("fis load -l");
					char new_boot[100];
					sprintf(new_boot, "fis load -d%s", boot_end);
					boot[boot_index] = strdup(new_boot);
					free(old_boot);
				}
			}
		}


		testThread =new WorkerThread(this, dev, current_conf, file1, file2, file3);
		testThread->Create();
		testThread->Run();
	}
	
}

void MainFrame::OnThreadMessage(wxCommandEvent& evt)
{
	if(globalAppFrame != NULL)
	{
		wxString msg = evt.GetString();
		if(msg.compare(wxT("FON_FLASH_APPLICATION_END_SUCCESS")) == 0)
		{
			//report success and exit
			wxMessageDialog* success = new wxMessageDialog(NULL, wxT("Device Flashed Successfully"), wxT("Success"), wxOK );
			success->ShowModal();
			app->ExitMainLoop();
		}
		else if(msg.compare(wxT("FON_FLASH_APPLICATION_END_FAILURE")) == 0)
		{
			//report failure and exit
			wxMessageDialog* failure = new wxMessageDialog(NULL, wxT("Flashing Failed"), wxT("Failure"), wxOK | wxICON_ERROR);
			failure->ShowModal();
			app->ExitMainLoop();
		}
		else
		{
			//print message to output box
			globalAppFrame->outputText->SetInsertionPointEnd();	
			globalAppFrame->outputText->WriteText( msg  );
		}
	}
}















class FonFlash : public wxApp
{
	public:
		virtual bool OnInit();
		char* programName;
};
IMPLEMENT_APP(FonFlash)


bool FonFlash::OnInit()
{
	pcap_if_t *alldevs;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_findalldevs(&alldevs, errbuf);
	if(alldevs == NULL )
	{
#ifdef WIN32

		wxMessageDialog *errorDialog = new wxMessageDialog(NULL, wxT("Error: Could not detect any network devices!\n\nMake sure you have administrator privelidges and try again."), wxT("Error"), wxOK | wxICON_ERROR);
		errorDialog->ShowModal();

		




#else
		wxPasswordEntryDialog* passwordDialog = new wxPasswordEntryDialog(NULL, wxT("No Network devices were detected.\n\nThis may be due to lack of administrative priviledges.\nEnter your password below to attempt to authenticate (via sudo), or press cancel to quit."), wxT("Enter a password"), wxT(""), wxOK | wxCANCEL | wxCENTRE);
		if(passwordDialog->ShowModal() == wxID_OK)
		{
			char *password = strdup( passwordDialog->GetValue().ToAscii() );

			char openbuf[500];
			programName = strdup(wxString(wxTheApp->argv[0]).ToAscii());
			sprintf(openbuf, "sudo -S %s >/dev/null 2>&1", programName);


			FILE* stest = popen("sudo -S touch passtest >/dev/null 2>&1", "w");
			fprintf(stest, "%s\n\n\n\n\n\n", password);
			pclose(stest);

			stest = fopen("passtest", "r");
			if(stest != NULL)
			{
				fclose(stest);
				stest = popen("sudo -S rm -f passtest >/dev/null 2>&1", "w");
				fprintf(stest, "%s\n\n\n\n\n\n", password);
				pclose(stest);


				stest =  popen(openbuf, "w");
				fprintf(stest, "%s\n\n\n\n\n\n", password);
			}
			else
			{
				wxMessageDialog *errorDialog = new wxMessageDialog(NULL, wxT("Authentication Failed"), wxT("Authentication Failed"), wxOK | wxICON_ERROR);
				errorDialog->ShowModal();
			}
		}
#endif
		
		return false;
	}
	else
	{
		globalAppFrame = new MainFrame(this,wxT("Fon Flash"));
		globalAppFrame->Show(true);
		SetTopWindow(globalAppFrame);


		return true;
	}
	return false;
}


int gui_printf(const char* format, ...)
{
	if(globalAppFrame != NULL)
	{
		if(globalAppFrame->testThread != NULL)
		{
			va_list args;
			char buf[5000];
			va_start(args, format);
			vsprintf(buf, format, args);
			globalAppFrame->testThread->DoPrint( wxString::FromAscii(buf) );
			va_end(args);
		}
	}
	return 1;
}

int gui_fprintf(FILE* stream, const char* format, ...)
{
	int ret_val = 1;
	va_list args;
	va_start(args, format);
	if(globalAppFrame != NULL && stream == stderr)
	{
		
		if(globalAppFrame->testThread != NULL)
		{
			va_list args;
			char buf[5000];
			va_start(args, format);
			vsprintf(buf, format, args);
			globalAppFrame->testThread->DoPrint( wxString::FromAscii(buf) );
			va_end(args);
		}
	}
	else
	{
		ret_val = vfprintf(stream, format, args);
	}
	va_end(args);
	return ret_val;
}

void gui_exit(int status)
{
	if(status == 0)
	{
		globalAppFrame->testThread->DoPrint(wxT("FON_FLASH_APPLICATION_END_SUCCESS"));
	}
	else
	{
		globalAppFrame->testThread->DoPrint(wxT("FON_FLASH_APPLICATION_END_FAILURE"));
	}
}

