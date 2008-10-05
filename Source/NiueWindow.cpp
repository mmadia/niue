
/*//-----------------------------------------------------------------------------
	Niue Development Environment
	Copyright 2008 T. Lansbergen, All Rights Reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted for non-commercial use only.
	
	This software is provided ``as is'' and any express or implied warranties,
	including, but not limited to, the implied warranties of merchantability and
	fitness for a particular purpose are disclaimed. In no event shall
	authors be liable for any direct, indirect, incidental, special,
	exemplary, or consequential damages (including, but not limited to,
	procurement of substitute goods or services; loss of use, data, or profits;
	or business interruption) however caused and on any theory of liability,
	whether in contract, strict liability, or tort (including negligence or
	otherwise) arising in any way out of the use of this software, even if
	advised of the possibility of such damage. 

*///-----------------------------------------------------------------------------

#include <Menu.h>

#include "Utils.h"
#include "Niue.h"
#include "UBuffer.h"
#include "BMapView.h"
#include "NiueWindow.h"
#include "Hash.h"
#include "Schemes.h"
#include "BSBar.h"
#include "ViewBase.h"
#include "FFont.h"
#include "BufList.h"

#include <MenuBar.h>
#include <MenuItem.h>
#include <Application.h>
#include <Screen.h>
#include <Beep.h>
#include <Directory.h>

#include <stdio.h>
#include <stdlib.h>  //atol for gotoline
#include <string.h>
#include <malloc.h>
#include <Autolock.h>

struct MenIt
{
	char *name;
	int32 msg;
};

#define LEFT_ARROW "\034"
#define RIGHT_ARROW "\035"
#define TAB "\011"
#define BRITE 255
#define NORM 219
#define DARK 152

MenIt AppMenu[] = {	{"Niue",0},
					{"\0About Niue…",B_ABOUT_REQUESTED},
					{"\4Preferences", 0},
					{"\0Documentation…",N_HELP_REQUESTED},
					{"\1\1",0},
					{"QQuit", B_QUIT_REQUESTED},
					{"\0\0",0}	};

MenIt DocumentMenu[] = {	{"File",0},
							{"NNew Window…", A_NEW_WINDOW},
							{"N\1New Project…",A_NEW_PROJECT},
							{"OOpen Project…",A_OPEN_PANEL},
							{"BBrowse Projects…", A_BROWSE_PANEL},
							{"\1\1",0},
							{"SSave",SAVE_FILE},
							{"S\1Save As…", SAVE_AS_FILE},
							{"\0Save All",A_SAVE_ALL},
							{"\0\0",0}	};

MenIt MoreEditMenu[] = {	{"Search",0},
							{"F\1Search Multiple Documents", A_SHOW_GRAPE},
							{"\1\1",0},
							{"FFind",A_FIND_DIALOG},
							{"GFind Forward",A_FIND_FORWARD},
							{"DFind Backward",A_FIND_BACK},
							{"HFind Selection",A_ENTER_FIND_STRING},
							{"JReplace Selection",A_REPLACE_THIS},
							{"\1\1",0},
							{",Go To Line",Y_GOTO_LINE},
							{"\0\0",0}
							};


MenIt EditMenu[] = {	{"Edit",0},
						{"ZUndo",A_UNDO},
						{"\1\1",0},
						{"XCut",B_CUT},
						{"CCopy",B_COPY},
						{"VPaste",B_PASTE},
						{"\1\1",0},
						{"ASelect All",B_SELECT_ALL},
						{"A\1Deselect All",A_SELECT_NONE},
						{"\1\1",0},
						{"]Indent",A_INDENT},
						{"[Undent",A_UNDENT},
						{"]\1Quote",A_QUOTE},
						{"[\1Unquote",A_UNQUOTE},
						{"\0\0",0}	};
							
MenIt ProjectMenu[] = {	{"Project",0},
						{"TAdd Files To Project",A_ADD_FILE},
						{"YBuild Rules", A_GEN_MAKEFILE},
						{"\1\1",0},
						{"MMake", MAKE_MSG},
						{"UMake Clean", MAKECLEAN_MSG},
						{"RRun", RUN_MSG},
						{"\1\1", 0},
						{"E\1Create Backup", A_BACKUP_APP},
						{"EExport Application", A_EXPORT_APP},
						{"\0\0",0}	};
							
MenIt ToolsMenu[] = {	{"Tools",0},
						{"\0Visual Designer", A_SHOW_VD},
						{"\0Snippet Manager", A_SHOW_SM},
						{"\0Grape", A_SHOW_GRAPE},
						{"\1\1",0},
						{"\0The Be Book", A_SHOW_BB},
						{"\0\0",0}	};

							
MenIt PrefsMenu[] = {	{"Preferences", 0},
						{"\0Revert to factory standards", A_REVERT_FACTORY},
						{"\0\0",0}	};

MenIt WinMenu[] = {	{"View",0},
					{"9Switch File - up", A_FILE_UP},
					{"0Switch File - down", A_FILE_DOWN},
					{"\1\1",0},			
					{"\10Font Name",0},
					{"\12Font Size",0},
					{"\1\1",0},
					{"\0Code Completion", A_CHANGE_COMPLETION},
					{"\0\0",0}	};
								

// thread callable
long
MyDraw(void *dummy)
{
	BMapView *vv = (BMapView *)dummy;
	return vv->DrawThread();
}


void
ProcTitle(char *src,BMenuItem *hit,BMenuItem *top,bool set)
{
	if (hit)
	{
		hit->SetMarked(set);
		if (top && set)
		{
//			char sometext[128];
//			sprintf(sometext,src,hit->Label());
			top->SetLabel("Font Type");
		}
	}
}


NiueWindow::NiueWindow(BRect frame,Buff *bb)
	:	BWindow(frame,"Niue",B_DOCUMENT_WINDOW,B_NAVIGABLE),
		cb(NULL),
		fname(NULL),
		fstyle(NULL),
		fsize(NULL),
		mdir(NULL),
		mwin(NULL),
		mproj(NULL),
		mtools(NULL),
		mmime(NULL),
		mcurs(NULL),
		icurs(NULL),
		ipref(NULL),
		ifname(NULL),
		ifstyle(NULL),
		ifsize(NULL),
		mnxt(NULL),
		mprv(NULL)
{
	SetPulseRate(0.0);
	static int32 cnt = 1;wincnt = atomic_add(&cnt,1);

	IsHScroll = true;

	char tempname[128];
	sprintf(tempname,"#%ld DrawPort",wincnt);
	drprt = create_port(32,tempname);

	vsplit = 200; //vertical mover
	hsplit = 515;	//horizontal mover
	ysplit = 400; //doclist|buttons
	xsplit = 600;	//output|hints

	topmenu = new BMenuBar(BRect(0,0,frame.Width(),14),"TopMenu",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,B_ITEMS_IN_ROW,TRUE);

	CreateMenus(topmenu);
	AddChild(topmenu);

	MyMetrics();
	
	scbv = new BSBar(rsbv,"ScrV",drprt,0,100,B_VERTICAL);
	scbh = new BSBar(rsbh,"ScrH",drprt,0,600,B_HORIZONTAL);
	
	mvrVert = new Mover(rmover,B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM,true);
	mvrHorz = new Mover(hmover,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM,false);
	
	myview = new BMapView(this,bb,rmyview,"NIUE:BMView",scbv,scbh,drprt);
	
	vwList = new listview(rlist, "vwList");
	
	vwControl = new ControlView(rcontrol, "vwControl");
	
	vwOutput = new outputview(routput, "vwOuput");
	
	vwHint = new HintView(rhint, "vwHint");
	
	vwFill = new BView(rfill, "vwFill", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	
	//set objects
	vwList->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	vwControl->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	vwHint->SetViewColor(blueish);
	vwFill->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	mycurs = ((wincnt-1)&3)*4 + 140;
	myview->ChangeCursor(mycurs);
	
	//searchview
	sview = new StatusView(rssv,"StView",drprt);
	sview->SetSearch("Search");
	sview->SetReplace("Replace");
	
	ChangeBuff(bb);
	
	//add objects
	AddChild(mvrHorz);
	AddChild(mvrVert);
	AddChild(myview);
	AddChild(vwControl);
	AddChild(scbh);
	AddChild(scbv);
	AddChild(vwList);
	AddChild(sview);
	AddChild(vwOutput);
	AddChild(vwHint);
	AddChild(vwFill);
	
	//ugly hack to bring menu to front
	RemoveChild(topmenu);
	AddChild(topmenu);
	
	FixDisp();
	myview->MakeFocus();
	
	sprintf(tempname,"#%ld Renderer",wincnt);
	drth = spawn_thread(MyDraw,tempname,B_DISPLAY_PRIORITY,myview);
	if (drth>0){resume_thread(drth);}else{drth = 0;}
	SetPulseRate(500000.0);
	
//	mback->FindItem("Light")->SetMarked(true);
	
	hasProjectOpen = false;
	doCodeCompletion = true;
	
	LoadPrefs();
}


NiueWindow::~NiueWindow()
{
	if (drprt)
	{
		delete_port(drprt);
		drprt = 0;
	}
	
	if (drth)
	{
		long temp;
		wait_for_thread(drth,&temp);
		drth = 0;
	}
}


void
NiueWindow::LoadPrefs()
{
	//read the settings file
	BPath prefspath;
	
	//find prefsfile
	find_directory(B_USER_SETTINGS_DIRECTORY, &prefspath, false);
	prefspath.Append("Niue/niue_prefs");
	
	//read file
	BFile prefsfile(prefspath.Path(), B_READ_ONLY);
	if (prefsfile.IsReadable())
	{
		BMessage msg('PREF');
//		int32 backstyle;
		msg.Unflatten(&prefsfile);
			
//		//backstyle
//		if (msg.FindInt32("back_style", &backstyle) == B_OK)
//		{
//			BMessage msg(Y_CHANGE_BACKSTYLE);
//			msg.AddInt32("Option", backstyle);
//			PostMessage(&msg);
//		}
			
		//font
		BFont font;
		if (FindMessageFont(&msg,"font",0,&font) == B_OK)
			HandleFontDrop(&msg);
		
		//codecompletion
		bool icode;
		if (msg.FindBool("code", &icode) == B_OK)
		{
			doCodeCompletion = icode;
			mwin->ItemAt(6)->SetMarked(doCodeCompletion);
			if (doCodeCompletion == false)
				vwHint->SetViewColor(white);
		}
		else
		{
			doCodeCompletion = true;
			mwin->ItemAt(6)->SetMarked(true);
		}
		
		//bounds
		BRect frame;
		if (msg.FindRect("bounds", &frame) == B_OK)
		{
			MoveTo(frame.left, frame.top);
			ResizeTo(frame.Width(), frame.Height());
		}
		
	}
	else
	{
		doCodeCompletion = true;
		mwin->ItemAt(6)->SetMarked(true);
	}
}


void
NiueWindow::SavePrefs()
{
	//now let's save the settigns
	BPath prefspath;
	
	//find prefsfile
	find_directory(B_USER_SETTINGS_DIRECTORY, &prefspath, false);
	prefspath.Append("Niue/niue_prefs");
	
	//create file
	BFile prefsfile(prefspath.Path(), B_READ_WRITE | B_CREATE_FILE);
	if (prefsfile.IsWritable())
	{
		BMessage msg('PREF');
	
//		msg.Unflatten(&prefsfile);		
		//font
		FFont ff(myview->myfont);
		AddMessageFont(&msg,"font",&ff);
		
		//code completion
		msg.AddBool("code", doCodeCompletion);
		
		//bounds
		msg.AddRect("bounds", Frame());
		
		//version
		msg.AddInt8("version", 5);
				
		msg.Flatten(&prefsfile);
	}
}


void
NiueWindow::MyMetrics()
{
	font_height temp;
	be_plain_font2->GetHeight(&temp);
	float   sh = ceil(temp.ascent + 9.0);

	mbh = topmenu->Bounds().Height();

	frame = Bounds();

	float bx = frame.Width()-B_V_SCROLL_BAR_WIDTH;
	float by = frame.Height();
	if (IsHScroll)
		by -= B_H_SCROLL_BAR_HEIGHT;
	
	rmover = BRect(vsplit-6,mbh + 1,vsplit-1,frame.Height());
	hmover = BRect(vsplit-1,ysplit + 1,frame.Width(),ysplit + 6);
	rlist = BRect(0,mbh,vsplit-7,hsplit);
	rcontrol = BRect(0,hsplit + 1,vsplit-7,frame.Height());
	rmyview = BRect(vsplit,mbh + 2.0,bx,ysplit-sh-B_H_SCROLL_BAR_HEIGHT-1);    //BMapView
	routput = BRect(vsplit,ysplit + 7,xsplit,frame.Height());
	rhint = BRect(xsplit,ysplit + 7,frame.Width(),frame.Height());
	rsbv = BRect(bx + 1.0,mbh + 1.0,frame.Width() + 1.0,ysplit-B_H_SCROLL_BAR_HEIGHT-sh-1);
	rsbh = BRect(vsplit,ysplit-B_H_SCROLL_BAR_HEIGHT-sh,bx,ysplit-sh);
	rssv = BRect(vsplit,ysplit + 1.0-sh,frame.Width(),ysplit);
	rfill = BRect(bx,ysplit-B_H_SCROLL_BAR_HEIGHT-sh,bx + B_V_SCROLL_BAR_WIDTH,ysplit-sh);

	SetSizeLimits(vsplit + B_V_SCROLL_BAR_WIDTH,30000,mbh,30000);   //mbh = menu bar height
}


void
NiueWindow::Shove(BView *src,BRect dest)
{
	src->MoveTo(dest.left,dest.top);
	src->ResizeTo(dest.Width(),dest.Height());
}


void
NiueWindow::FixDisp()
{
	Shove(myview,rmyview);
	Shove(scbh,rsbh);
//	sview->RemoveChild(sview->mrv);
	Shove(sview,rssv);
//	if (IsMsg)
//		sview->AddChild(sview->mrv);
//	RemoveChild(scbh);
//	if (IsHScroll)
//		AddChild(scbh);
}


void
NiueWindow::AddFile(entry_ref fileref)
{
	BEntry fileentry;
	fileentry.SetTo(&fileref, true);
	char name[B_FILE_NAME_LENGTH];
	BPath filepath, projpath;
	fileentry.GetName(name);
	fileentry.GetPath(&filepath);
	entry.GetPath(&projpath);
	
	BString addstring;
	addstring << "cp ";
	addstring << filepath.Path();
	addstring << " ";
	addstring << projpath.Path();
	addstring << "/";
	addstring << name;
	
	if (system(addstring.String()) != 0)
	{
		(new BAlert("Niue", "Niue\n\nThere was en error while adding the file(s) "
							"to your project.", "OK", 0, 0, B_WIDTH_AS_USUAL,
							B_WARNING_ALERT))->Go();
	}
}


void
NiueWindow::LoadDir(entry_ref dirref)
{
	//get reference
	entry.SetTo(&dirref);
	entry.GetRef(&ref);
	
	//build filelist and show makefile
	vwList->BuildList(ref);
	
	//start to watch the directory for changes
	node_ref nref;
	entry.GetNodeRef(&nref);
	watch_node(&nref, B_WATCH_DIRECTORY, this);
	
	//get project's path
	BString titlestring;
	BPath projectpath;
	
	entry.GetPath(&projectpath);
	titlestring << "Niue -  ";
	titlestring << projectpath.Path();
	
	//and set the title
	SetTitle(titlestring.String());
	
	//were done loading the project, let's enable some features
	//menu items
//	mdoc->ItemAt(5)->SetEnabled(true);
//	mdoc->ItemAt(4)->SetEnabled(true);
	mdoc->ItemAt(6)->SetEnabled(true);
	mproj->ItemAt(0)->SetEnabled(true);
	mproj->ItemAt(1)->SetEnabled(true);
	mproj->ItemAt(3)->SetEnabled(true);
	mproj->ItemAt(4)->SetEnabled(true);
	mproj->ItemAt(5)->SetEnabled(true);
	mproj->ItemAt(7)->SetEnabled(true);
	mproj->ItemAt(8)->SetEnabled(true);
	mwin->ItemAt(0)->SetEnabled(true);
	mwin->ItemAt(1)->SetEnabled(true);

	//buttons
	vwControl->btnMake->SetEnabled(true);
	vwControl->btnMakeClean->SetEnabled(true);
	vwControl->btnRun->SetEnabled(true);

	//empty outputview
	vwOutput->ClearText();
	
	//and let the world know we have open project
	hasProjectOpen = true;
}


entry_ref
NiueWindow::SaveProject(const char *dest, const char *name)
{
	// first get the resource from our binary
	entry_ref exitref;
	app_info myinfo;
	size_t length;
	const void *data;
	FILE *output_file;
	BString temppath;
	temppath << dest << "/niue_temp.zip";
	
	be_app->GetAppInfo(&myinfo);
	BFile fill(&myinfo.ref,B_READ_ONLY);
	BResources *bres = new BResources(&fill);
	
	if (!(bres->HasResource('SKEL', 9)))
	{
		(new BAlert("Niue", "Can't find the resource", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return exitref;
	}	
	
	data = bres->LoadResource('SKEL', 9, &length);
	if (data)
	{
		output_file = fopen(temppath.String(), "wb");
		if (fwrite(data, 1, length, output_file) != length)
		{
			(new BAlert("Niue", "There were errors while trying to fetch the skeleton project.", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			return exitref;
		}
		fclose(output_file);
	}
	
	//now unzip and rename it
	//unzip -n niue_temp; mv Skeleton name; rm niue_temp.zip 
	BString sysstring;
	sysstring << "cd " << dest << "; unzip -n niue_temp; mv Skeleton " << name << "; rm niue_temp.zip";
	
	if (system(sysstring.String()) != 0)
	{
		(new BAlert("Niue", "There were errors while trying to create your project.", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return exitref;
	}
	
	//at last, open the project
	BString exitpath;
	exitpath << dest << "/" << name;
	
	BEntry exitentry(exitpath.String());
	if (exitentry.GetRef(&exitref) != B_OK)
	{
		(new BAlert("Niue", "There were errors while trying to open your project.", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return exitref;
	}
	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs", &exitref);
	be_app->PostMessage(&msg);
	
	return exitref;
}


void
NiueWindow::Fumble(BMenu *top,BMenuItem **mi,BMenu **mn,MenIt *mit)
{
	if (mit)
		*mn = MenuFrom(mit);
	else
		*mn = new BMenu("HAHA :-)",B_ITEMS_IN_COLUMN);

	*mi = new BMenuItem(*mn);
	top->AddItem(*mi);
}


BMenu *
NiueWindow::MenuFrom(MenIt *mi)
{
	int cc = 0;

	BMenu *mtm = new BMenu(mi->name,B_ITEMS_IN_COLUMN);
	mi++;
	while (true)
	{
		BMenu *sub = NULL;
		char *ct = mi->name;
		if (ct == NULL || ct[1] == 0)
			break;
		switch (ct[0])
		{
			case 1:
			{
				mtm->AddSeparatorItem();
				break;
			}
			case 4:
			{
				Fumble(mtm,&ipref,&mpref,PrefsMenu);
				break;
			}
			case 8:
			{
				Fumble(mtm,&ifname,&fname,NULL);
				break;
			}
			case 9:
			{
				Fumble(mtm,&ifstyle,&fstyle,NULL);
				break;
			}
			case 10:
			{
				Fumble(mtm,&ifsize,&fsize,NULL);
				break;
			}
			default:
			{
				BMessage *mg = new BMessage(mi->msg);
				mg->AddInt32("Option",cc++);
				int vv = ct[0], mod = 0;
				char *dst = ct + 1;
				if (ct[1] == 1)
				{
					mod = B_SHIFT_KEY;
					dst++;
				}
				if (ct[1] == 2)
				{
					mod = B_OPTION_KEY;
					dst++;
				}
				BMenuItem *mm = new BMenuItem(dst,mg,vv,mod);
				mtm->AddItem(mm);
				break;
			}
		}
		if (sub)
			mtm->AddItem(sub);
		mi++;
	}
	return mtm;
}


void
NiueWindow::CreateMenus(BMenu *mbar)
{
	mapp = MenuFrom(AppMenu);
	fApp = new TIconMenu(mapp);
  	mbar->AddItem (fApp);
	mbar->AddItem(mdoc = MenuFrom(DocumentMenu));
	mbar->AddItem(medit1 = MenuFrom(EditMenu));
	mbar->AddItem(medit2 = MenuFrom(MoreEditMenu));
	mbar->AddItem(mproj = MenuFrom(ProjectMenu));
	mbar->AddItem(mtools = MenuFrom(ToolsMenu));
	mbar->AddItem(mwin = MenuFrom(WinMenu));

	//disable some, we will be enabling them later
//	mdoc->ItemAt(5)->SetEnabled(false);
//	mdoc->ItemAt(4)->SetEnabled(false);
	mdoc->ItemAt(6)->SetEnabled(false);
	mproj->ItemAt(0)->SetEnabled(false);
	mproj->ItemAt(1)->SetEnabled(false);
	mproj->ItemAt(3)->SetEnabled(false);
	mproj->ItemAt(4)->SetEnabled(false);
	mproj->ItemAt(5)->SetEnabled(false);
	mproj->ItemAt(7)->SetEnabled(false);
	mproj->ItemAt(8)->SetEnabled(false);
	mwin->ItemAt(0)->SetEnabled(false);
	mwin->ItemAt(1)->SetEnabled(false);
}


void
NiueWindow::MoveCurs(int cx,int cy)
{
	sview->SetPos(cx,cy);
}


long
NiueWindow::KeyDown(BMessage *msg)  //don't forget shift-insert, shift-del
{
	
	int32 mod,raw,key;
	char *str;
	msg->FindInt32("modifiers",&mod);
	msg->FindInt32("key",&key);
	msg->FindInt32("raw_char",&raw);
	msg->FindString("bytes",(const char**)&str);
	
	if (raw > 255)
	{
		int32 event = 0;
		switch (raw)
		{
			case 260:
				event = B_CUT;
				break;
			case 261:
				event = B_COPY;
				break;
			case 262:
				event = B_PASTE;
				break;
			case 263:
				event = A_KILL_SELECTION;
				break;
			case 271:
				event = A_FIND_FORWARD;
				break;
			case 272:
				event = A_ENTER_FIND_STRING;
				break;
			case 280:
				event = SAVE_FILE;
				break;
			case 282:
				event = A_PREV_BUFF;
				break;
			case 283:
				event = A_NEXT_BUFF;
				break;
		}

		if (event)
		{
			if (event != 1)
				PostMessage(event);
			return 0;
		}
	}
	
	return 0;
}


void
NiueWindow::GotoLine(BMessage *msg)
{
	char *ss = NULL;
	status_t rr = msg->FindString("Data",(const char**)&ss);
	if (rr != B_OK)
		return;
	
	long ll = atol(ss);
	if (ll >= 0)
	{
		if (ll)
		{
			BMessage ms(A_GOTO_LINEY);
			ms.AddInt32("Pos",ll);
			ms.AddPointer("YWin",this);

			cb->PostMessage(&ms);
		}
		myview->MakeFocus();
	}
}


void
NiueWindow::ChangeBuff(Buff *bb)
{
	if (!bb)
		return;
	Own;

	if (cb)
	{
		cb->DeRegister(myview->ws);
		cb = NULL;
	}
	
	cb = bb;
//    CheckTitle();
	int32 tn = cb->Register(myview->ws);
	if (myview)
	{
		myview->ChangeBuff(bb);
		myview->targetnum = tn;
	}
	
	if (drprt)
		write_port(drprt,SCROLL,&bb,0);
}


void
NiueWindow::HandleFontDrop(BMessage *msg)
{
//	msg->PrintToStream();
	BFont nufont = myview->myfont;
	FindMessageFont(msg,"font",0,&nufont);
	if (nufont == myview->myfont)
		return;
	
	double oldh,oldv;
	oldh = myview->scbh->Value();
	oldv = myview->scbv->Value()/myview->fh;
	myview->myfont = nufont;
	myview->FrameResized(0,0);
	myview->FlickTo(oldh,oldv*myview->fh);
	myview->Invalidate(myview->Bounds());
	DoMenuFontName();
	DoMenuFontSize();
}


void
NiueWindow::MessageReceived(BMessage *msg)
{
//	long tt;
//	void *vv = NULL;
//	Buff *bb = NULL;
	
//	YAction res = A_NOTHING;
	
	switch (msg->what)
	{
		case MOVER_MESSAGE:
		{
			Mover *mv;
			if (msg->FindPointer(MM_VIEW,(void**)&mv) == B_OK )
			{
				float coord = msg->FindFloat(MM_COORD);
				
				//warning, horrible math below...
				
				if (mv == mvrVert)
				{				
					BPoint p = ConvertFromScreen(BPoint(coord,0));
								
					//left views
					vwControl->ResizeTo(p.x,vwControl->Frame().Height());
					vwList->ResizeTo(p.x,vwList->Frame().Height());
							
					//right views
					myview->ResizeTo(((Frame().Width())-(B_V_SCROLL_BAR_WIDTH + 1))-p.x-mvrVert->Frame().Width(),myview->Frame().Height());
					myview->MoveTo(p.x + mvrVert->Frame().Width() + 1,myview->Frame().top);
					
					sview->ResizeTo(Frame().Width()-p.x-mvrVert->Frame().Width(),rssv.Height());
					sview->MoveTo(p.x + mvrVert->Frame().Width() + 1,sview->Frame().top);
					
					scbh->ResizeTo(Frame().Width()-(B_V_SCROLL_BAR_WIDTH + 1)-p.x-mvrVert->Frame().Width(),rsbh.Height());
					scbh->MoveTo(p.x + mvrVert->Frame().Width() + 1,scbh->Frame().top);
					
					vwOutput->ResizeTo(Frame().Width()-vwHint->Frame().Width()-p.x-mvrVert->Frame().Width()-1,vwOutput->Frame().Height());
					vwOutput->MoveTo(p.x + mvrVert->Frame().Width() + 1,vwOutput->Frame().top);
					
					//moverts
					mvrVert->MoveTo(p.x,mvrVert->Frame().top);
					mvrHorz->ResizeTo((Frame().Width())-p.x-mvrVert->Frame().Width(),hmover.Height());
					mvrHorz->MoveTo(p.x + mvrVert->Frame().Width() + 1,mvrHorz->Frame().top);
					
					//size limits
					SetSizeLimits(p.x + 4 + B_V_SCROLL_BAR_WIDTH,30000,mbh,30000);
				}
				else if (mv == mvrHorz)
				{
					BPoint p = ConvertFromScreen(BPoint(0,coord));
					
					//top views
					scbv->ResizeTo(scbv->Frame().Width(), p.y-(sview->Frame().Height() + B_H_SCROLL_BAR_HEIGHT + mbh + 3));
					
					myview->ResizeTo(myview->Frame().Width(), p.y-(sview->Frame().Height() + B_H_SCROLL_BAR_HEIGHT + mbh + 4));
					
					scbh->MoveTo(scbh->Frame().left, p.y-(B_H_SCROLL_BAR_HEIGHT + sview->Frame().Height() + 1));
					
					sview->MoveTo(sview->Frame().left, p.y-sview->Frame().Height());
					
					vwFill->MoveTo(vwFill->Frame().left, p.y-(vwFill->Frame().Height() + sview->Frame().Height() + 1));
					
					//bottom views
					vwOutput->ResizeTo(vwOutput->Frame().Width(), Frame().Height()-p.y-mvrHorz->Frame().Height());
					vwOutput->MoveTo(vwOutput->Frame().left, p.y + mvrHorz->Frame().Height() + 1);
					
					vwHint->ResizeTo(vwHint->Frame().Width(), Frame().Height()-p.y-mvrHorz->Frame().Height());
					vwHint->MoveTo(vwHint->Frame().left, p.y + mvrHorz->Frame().Height() + 1);
					
					//movert
					mvrHorz->MoveTo(mvrHorz->Frame().left, p.y);					
				}
			}
			break;
		}	
		case B_MOUSE_WHEEL_CHANGED:
		{
			if (myview->IsFocus())
			{
				float x, y, wheel;
				
				msg->FindFloat("be:wheel_delta_y", &wheel);
				
				x = myview->tax;
				y = myview->tay;
				
				myview->FlickTo(x,y + wheel*20);
				
				BString teststring;
				teststring << "x: " << x;
				teststring << "  y: " << y;
				teststring << "  wheel: " << wheel;
				teststring << "  y + wheel: " << y + wheel;
			}
			break;
		}
		case A_NEW_PROJECT:
		{
//			if (!SavePanel)
//			{
				SavePanel = new BFilePanel(B_SAVE_PANEL, 0, 0, 0, false, new BMessage(NEW_PROJECT_MSG), 0, false, true);
				SavePanel->SetTarget(this);
				SavePanel->SetSaveText("project name");
				SavePanel->Window()->SetTitle("Niue: Select a location to save your project folder");
				SavePanel->Show();
//			}
			break;
		}
		case NEW_PROJECT_MSG:
		{
			entry_ref destref;
			BEntry destentry;
			BString projectname;
			BPath destpath;
			
			msg->FindRef("directory", &destref);
			msg->FindString("name", &projectname);
			
			projectname.RemoveAll(" ");  //remove all spaces 'cause SaveProject doesn't like them, too bad for the user...
			
			destentry.SetTo(&destref, false);
			destentry.GetPath(&destpath);
			
			//(new BAlert("Niue", deststring.String(), "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			SaveProject(destpath.Path(), projectname.String());
			break;
		}
		
		case B_SIMPLE_DATA:
//		case A_PREV_WINDOW:
//		case A_NEXT_WINDOW:
		case A_NEW_WINDOW:
		{
			be_roster->Launch("application/x-vnd.Niue4");	
			break;
		}
		case A_OPEN_PANEL:
		{
			msg->AddPointer("YWin",this);
			msg->AddPointer("Buff",cb);
			be_app->PostMessage(msg);
			break;
		}
		case A_BROWSE_PANEL:
		{
			wdwOpen = new OpenWindow;
			wdwOpen->Show();
			break;
		}
		case N_HELP_REQUESTED:
		{
			wdwHelp = new docwindow;
			wdwHelp->Show();
			break;
		}
		case A_SHOW_VD:
		{
			wdwVisual = new VisualWindow(BPoint(Bounds().left + 50,Bounds().top + 50));
			wdwVisual->Show();
			break;
		}
		case A_SHOW_SM:
		{
			wdwSnippet = new snippetwindow();
			wdwSnippet->Show();
			break;
		}
		case A_SHOW_BB:
		{
			status_t result;
			
			result = be_roster->Launch("application/x.vnd-STertois.BeHappy");			//try BeHappy first
			
			if (result != B_ALREADY_RUNNING && result != B_OK)
			{
				char* link = "http://www.tycomsystems.com/beos/BeBook/";				//then try online
				if (be_roster->Launch("text/html", 1, &link) != B_OK)
				{
					(new BAlert("Niue", "Niue: Sorry, couldn't open the BeBook", "OK", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
				}
			}			
			break;
		}
		case A_SHOW_GRAPE:
		{
			if (hasProjectOpen)
			{
				BPath path;
				entry.GetPath(&path);
				
				wdwGrep = new grepwindow(path.Path());
				wdwGrep->Show();	
			}
			else
			{
				wdwGrep = new grepwindow("/boot/home");
				wdwGrep->Show();	
			}
			break;						
		}
		case A_ADD_FILE:
		case B_ABOUT_REQUESTED:
		{
			be_app->PostMessage(msg);
			break;
		}
		case A_DISCARD_BUFFER:
		case A_SAVE_ALL:
		case A_NEXT_BUFF:
		case A_PREV_BUFF:
		case Y_CHANGE_BUFF:
		{
			msg->AddPointer("Buff",cb);
			msg->AddPointer("YWin",this);
			be_app->PostMessage(msg);
			break;
		}
		case A_FIND_DIALOG:
		{
			sview->srv->MakeFocus();
			sview->srv->SelectAll();
			break;
		}
		case B_KEY_DOWN + 1:
		{	
			//from Yak
			KeyDown(msg);
			break;
		}
		case Y_GOTO_LINE:
		{
			sview->yrv->MakeFocus();
			sview->yrv->SelectAll();
			break;
		}
		case Y_HSCROLL_ON:
		{
			IsHScroll = !IsHScroll;
			MyMetrics();
			FixDisp();
			break;
		}
		case Y_SYSFONT_CHANGE:
		{
			sview->xrv->SetFontAndColor(be_plain_font2);
			sview->yrv->SetFontAndColor(be_plain_font2);
			sview->srv->SetFontAndColor(be_plain_font2);
			sview->rrv->SetFontAndColor(be_plain_font2);
			MyMetrics();
			FixDisp();
			break;
		}
		case S_FIND_STRING:
		{
			char *t = NULL;
			msg->FindString("Text",(const char**)&t);
			if (t) sview->SetSearch(t);
			break;
		}
		case S_RESET_FOCUS:
		{
			myview->MakeFocus();
			int32 ll = -1;
			msg->FindInt32("Pos",&ll);
			if (ll>-1){
				myview->CleanScrollers();
				myview->MyScrollTo(ll);
			}
			break;
		}
//		case S_GOTO_LINEX:
		case S_GOTO_LINEY:
		{
			GotoLine(msg);
			break;
		}
		case S_GOTO_PLACE:
		{
			Buff *nb = NULL;
			msg->FindPointer("Buff",(void**)&nb);
			if (nb)
				ChangeBuff(nb);
			GotoLine(msg);
			break;
		}
//		case Y_CHANGE_BACKSTYLE:
//			tt = msg->FindInt32("Option");
//			if (tt<0)tt = 0;
//			if (tt>2)tt = 2;
//			if (tt == 0)
//			{
//				mback->ItemAt(1)->SetMarked(false);				
//				mback->ItemAt(0)->SetMarked(true);
//			}
//			else if (tt == 1)
//			{
//				mback->ItemAt(0)->SetMarked(false);				
//				mback->ItemAt(1)->SetMarked(true);
//			}
//
//			//code view
//			myview->vb->csh = schemetab[tt];
//			myview->Invalidate(myview->Bounds());
//			
//			//outputview
//			vwOutput->ChangeBackstyle(tt);
//			break;
		case '!FNT':
		{
			HandleFontDrop(msg);
			break;
		}
		case A_REPLACE_THIS:
		{
			msg->AddPointer("YWin",this);
			msg->AddString("Text",sview->rrv->Text());
			cb->PostMessage(msg);
			break;
		}
		case A_FIND_BACK:
		case A_FIND_FORWARD:
		{
			if (!sview->srv->Text()[0])
				myview->MakeFocus();
			else
			{
				msg->AddPointer("YWin",this);
				msg->AddString("Text",sview->srv->Text());
				cb->PostMessage(msg);
			}
			break;
		}
		case B_MODIFIERS_CHANGED:
			break;
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		{
			BView *tg = CurrentFocus();
			if (tg == myview)
			{
				msg->AddPointer("YWin",this);
				cb->PostMessage(msg);
			}
			else
			{
				BTextView *tgt = (BTextView*)tg;
				BClipboard *cl = be_clipboard;
				if (msg->what == B_CUT)
					tgt->Cut(cl);
				if (msg->what == B_COPY)
					tgt->Copy(cl);
				if (msg->what == B_PASTE)
					tgt->Paste(cl);
			}
			break;
		}
		case A_UNDO:
		case A_REDO:
		case A_KILL_SELECTION:
		case B_SELECT_ALL:
		case A_SELECT_NONE:
		case A_INDENT:
		case A_UNDENT:
		case A_QUOTE:
		case A_UNQUOTE:
		case A_ENTER_FIND_STRING:
		case A_REVERT_BUFFER:
		case SAVE_FILE:
		case SAVE_AS_FILE:
		{
			msg->AddPointer("YWin",this);
			cb->PostMessage(msg);
			break;
		}
		case B_UNMAPPED_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		{
			BWindow::MessageReceived(msg);
			break;
		}
		case B_NODE_MONITOR:
		{
			//discard all buffers
			be_app->PostMessage(A_DISCARD_ALL_BUFFERS);
			
			//(new BAlert("Niue", "Node Monitor!", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			//reload file list
			vwList->BuildList(ref);
			
			//check out waht has happened
			int32 opcode;
			const char *name;
			if (msg->FindInt32("opcode", &opcode) == B_OK)
			{
				if (opcode == B_ENTRY_CREATED)  //hey, a new file in our project dir
				{
					msg->FindString("name", &name);  //which file?
					vwList->SelectByName(name);
				}
			}
			break;
		}
		case MAKE_MSG:
		{
			vwOutput->ClearText();
			
			BPath path(&entry);
			BString command;
			command << "cd " << path.Path() << "; make 2>&1";
			
			//(new BAlert("Niue", command.String(), "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			
			vwOutput->DisplayShellProcess(command.String());			
			break;
		}
		case MAKECLEAN_MSG:
		{
			vwOutput->ClearText();
			
			BPath path(&entry);
			BString command;
			command << "cd " << path.Path() << "; make clean 2>&1";

			result = vwOutput->DisplayShellProcess(command.String());

			if (result == 0)
			{
				command = "";
				command << "cd " << path.Path() << "; make 2>&1";
				//(new BAlert("Niue", command.String(), "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
				vwOutput->DisplayShellProcess(command.String());				
			}
			break;
		}
		case RUN_MSG:
		{
			BPath path(&entry);
			BString pathstring = path.Path();
			pathstring << "/obj.x86/";

			//first, remove all earlier executables
			BDirectory dir(pathstring.String());
			BEntry file_entry;

			while (dir.GetNextEntry(&file_entry, true) == B_NO_ERROR)
			{
				if (file_entry.IsFile())
				{
					BNode node(&file_entry);
					BNodeInfo nodeInfo(&node);	
					char type[256];
					nodeInfo.GetType(type);
					
					//look for executables
					if (strcmp(type, "application/x-vnd.Be-elfexecutable") == 0)
						file_entry.Remove();
				}
			}
			
			vwOutput->ClearText();
			
			BString command;
			command << "cd " << path.Path() << "; make 2>&1";
			
			result = vwOutput->DisplayShellProcess(command.String());
			
			if (result == 0)
			{
				//find & run app
				BDirectory dir(pathstring.String());
				BEntry file_entry;

				while (dir.GetNextEntry(&file_entry, true) == B_NO_ERROR)
				{
					if (file_entry.IsFile())
					{
						BNode node(&file_entry);
						BNodeInfo nodeInfo(&node);	
						char type[256];
						nodeInfo.GetType(type);
						BPath exepath;
						BString exestring;
						
						//look for executables
						if (strcmp(type, "application/x-vnd.Be-elfexecutable") == 0)
						{
							file_entry.GetPath(&exepath);
							exestring << exepath.Path() << " &";
							system(exestring.String());
						}
					}
				}
			}
			else
			{
				(new BAlert("Niue", "There were errors while making your project,\nare your build rules up to date?", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			}
			break;
		}
		case A_BACKUP_APP:
		{
			char name[B_FILE_NAME_LENGTH];
			entry.GetName(name);
			
//			BDateTime* DateTime = new BDateTime();
//			uint32 nDay = DateTime->Day();
//			uint32 nMonth = DateTime->Month();
//			uint32 nYear = DateTime->Year();
			

//			BString datestring;
//			datestring << "-";
//			if (nDay<10) datestring << "0"; 			//leading zero's
//			datestring << nDay;
//			if (nMonth<10) datestring << "0";		//leading zero's
//			datestring << nMonth << nYear;

			BString backname;
			backname << name << "-backup";  // << datestring;
			
//			if (!BackupPanel)
//			{
				BackupPanel = new BFilePanel(B_SAVE_PANEL, 0, &ref, 0, false, new BMessage(BACKUP_PROJECT_MSG), 0, false, true);
				BackupPanel->SetTarget(this);
				BackupPanel->SetSaveText(backname.String());
				BackupPanel->Window()->SetTitle("Niue: Select a location to save your backup");
				BackupPanel->Show();
//			}	
			break;
		}
		case BACKUP_PROJECT_MSG:
		{
			entry_ref export_ref;
			BString export_name;

			msg->FindRef("directory", &export_ref);
			msg->FindString("name", &export_name);

			BEntry export_entry(&export_ref); 
			BPath export_path(&export_entry);
//			export_path.Append("/");
			export_path.Append(export_name.String());
			BPath proj_path(&entry);

			
			BString backstring;
			backstring << "mkdir " << export_path.Path() << " ; zip -rjD " << export_path.Path() << "/" << export_name << " " << proj_path.Path();
			
//			(new BAlert("Niue", backstring.String(), "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			
			system(backstring.String());
			break;
		}
		case A_EXPORT_APP:
		{
			wdwExport = new exportwindow(entry);
			wdwExport->Show();
			break;
		}
		case A_REVERT_FACTORY:
		{
			//remoe all files (but no dirs) in the prefs directory
			BPath prefspath;
			find_directory(B_USER_SETTINGS_DIRECTORY, &prefspath, false);
			prefspath.Append("Niue");
			
			BEntry prefsentry;
			BDirectory dir;
			dir.SetTo(prefspath.Path());
			
			while (dir.GetNextEntry(&prefsentry, true) == B_NO_ERROR) 
			{
				if (prefsentry.IsFile())
					prefsentry.Remove();
			}
			break;
		}
		case A_CHANGE_COMPLETION:
		{
			if (mwin->ItemAt(6)->IsMarked())
			{
				doCodeCompletion = false;
				vwHint->SetViewColor(white);
				vwHint->Invalidate();
				mwin->ItemAt(6)->SetMarked(false);
			}
			else
			{
				doCodeCompletion = true;
				vwHint->SetViewColor(blueish);
				vwHint->Invalidate();
				mwin->ItemAt(6)->SetMarked(true);
			}
			break;
		}
		case Y_COMPARE_HASH:
		{	
			if (doCodeCompletion == true)
			{
				BString word;
				msg->FindString("word", &word);
				char *lookup = (char*)word.String();
	
				BList result;
				result = ComparePart(lookup, strlen(lookup));
				if (result.CountItems() > 0)
				{	
					vwHint->lstHints->MakeEmpty();
					vwHint->lstHints->AddList(&result);
				}
				else
					vwHint->lstHints->MakeEmpty();	
			}
			break;
		}
		case A_INSERT_HINT:
		{
			msg->AddString("hint", dynamic_cast<BStringItem*>(vwHint->lstHints->ItemAt(vwHint->lstHints->CurrentSelection()))->Text());
			cb->PostMessage(msg);
			
			vwHint->lstHints->MakeEmpty();
			myview->MakeFocus(true);
			break;
		}
		case A_GEN_MAKEFILE:
		{
			wdwMake = new makefilewindow(entry);
			wdwMake->Show();
			break;
		}
		case A_FILE_UP:
		{
//			(new BAlert("Niue", "File up", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			
			if (vwList->fList->CurrentSelection() > 0)
				vwList->fList->Select(vwList->fList->CurrentSelection()-1);
			else
			{
				if (vwList->fList->CountItems() > 1)
					vwList->fList->Select(1);  //item 0 is the header
			}
			break;
		}
		case A_FILE_DOWN:
		{
//			(new BAlert("Niue", "File down", "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
				
			if (vwList->fList->CurrentSelection() > 0)
				vwList->fList->Select(vwList->fList->CurrentSelection() + 1);
			else
			{
				if (vwList->fList->CountItems() > 1)
					vwList->fList->Select(1);  //item 0 is the header
			}
			break;
		}
		default:
		{
//          msg->AddPointer("YWin",this);
//          cb->PostMessage(msg);
			#if defined(_DebugObj)
				printf("unknown message in NiueWindow:\n");
				msg->PrintToStream();
			#endif
			BWindow::MessageReceived(msg);
			break;
		}
	}
}


bool
NiueWindow::QuitRequested()
{
	SavePrefs();
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}


void
NiueWindow::DoMenuFontName()
{
//	if (!fname || !fstyle)
//		return;
	if (!fname)
		return;
	int32 idx,nft;
	uint32  flags;
	
	BFont nufont = myview->myfont;
	
	font_family curfam;
	font_style cursty;
	nufont.GetFamilyAndStyle(&curfam,&cursty);
	
	nft = count_font_families();
	for (idx = 0;idx<nft;idx++)
	{
		font_family myfam;
		font_style mysty;
//		font_style *deststy = &mysty;
		get_font_family(idx,&myfam,&flags);
		int mx = count_font_styles(myfam);
		while (--mx >= 0)
		{
			get_font_style(myfam,mx,&mysty,&flags);
			if (strcmp(mysty,cursty) == 0)
				break;
		}

		BMenuItem *it = (BMenuItem*)fnl.ItemAt(idx);
		if (!it)
		{
			it = new BMenuItem("",new BMessage('!FNT'),0);
			fnl.AddItem(it);
			fname->AddItem(it);
		}
		it->SetLabel(myfam);
		BMessage *msg = it->Message();
		msg->MakeEmpty();
		nufont.SetFamilyAndStyle(myfam,mysty);
		FFont ff(nufont);
		ff.SetMask(B_FONT_FAMILY_AND_STYLE);
		AddMessageFont(msg,"font",&ff);
		ProcTitle("%s Font",it,ifname,strcmp(curfam,myfam) == 0);
	}

	while (true)
	{
		BMenuItem *it = (BMenuItem*)fsl.ItemAt(idx);
		if (!it)
			break;
		fstyle->RemoveItem(it);
		idx++;
	}
}


void
NiueWindow::DoMenuFontSize()
{
	if (!fsize)
		return;
	float myf = myview->myfont.Size();
	float fss[50],temp;
	float dff[] = {6,8,10,12,15,20,24,30,50,0};
	int idx;

	if (ifsize)
	{
//		char sometext[32];
//		sprintf(sometext,"%1.1f Size",myf);
//		ifsize->SetLabel(sometext);
		ifsize->SetLabel("Font Size");
	}

	idx = -1;
	while (dff[++idx] != 0)
		fss[idx] = dff[idx];
	
	for (int diff = -4; diff <= 4; diff++)
		fss[idx++] = myf+diff;
	
	fss[idx++] = myf * 2;
	fss[idx++] = myf / 2;
	fss[idx++] = myf * 3;
	fss[idx++] = myf / 3;
	fss[idx++] = myf + 10;
	fss[idx++] = myf - 10;
	int max = idx;

	int safe = TRUE;
	while (safe)
	{
		safe = FALSE;
		idx = 0;
		for (int i = 0;i<max-1;i++)
		{
			fss[i] = floor(fss[i] + 0.5);
			if (fss[i]<4.0)
				fss[i] = 4.0;
			if (fss[i]>100.0)
				fss[i] = 100.0;
			if (fss[i] == fss[i + 1])
			{
				fss[i] = fss[max-1];
				max--;
			}
			if (fss[i]>fss[i + 1]){
				temp = fss[i + 1];
				fss[i + 1] = fss[i];
				fss[i] = temp;
				safe = TRUE;
			}
		}
	}
	fss[max] = 0.0;

	BFont nufont = myview->myfont;
	for (idx = 0; idx < max; idx++)
	{
		BMenuItem *it = fsize->ItemAt(idx);
		if (!it)
			fsize->AddItem(it = new BMenuItem("",new BMessage('!FNT'),0,0));
		char mystr[8];
		sprintf(mystr,"%d",(int)fss[idx]);
		it->SetLabel(mystr);
		
		BMessage *msg = it->Message();
		msg->MakeEmpty();
		nufont.SetSize(fss[idx]);
		FFont ff(nufont);
		ff.SetMask(B_FONT_SIZE);
		AddMessageFont(msg,"font",&ff);
		it->SetMarked(fss[idx] == myf);
	}
	while (fsize->ItemAt(idx))
		fsize->RemoveItem(idx);
}

//extern char *doctxt;

void
NiueWindow::MenusBeginning()
{
	DoMenuFontSize();
	DoMenuFontName();
}

void
NiueWindow::ScreenChanged(BRect, color_space depth)
{
	//    printf("cspace was %x, now %x\n",myview->cspace,depth);
	myview->cspace = depth;
}


StatusView::StatusView(BRect rect, char *name,port_id pp)
	:	BView(rect, name, B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT,
				B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE)
{
	pp = pp;
	cvx = cvy = -1;
	float   hh = rect.Height(); //*0.5;
	float   xx = 0,ww,mx,fx;
	
	ww = 30;xpos.Set(xx,0,xx + ww,hh);xx += ww + 2;
	ww = 30;ypos.Set(xx,0,xx + ww,hh);xx += ww + 2;
	
	fx = rect.Width();
	mx = (fx + xx)/2.0;
	spos.Set(xx,0,mx-1.0,hh);
	rpos.Set(mx + 1.0,0,fx,hh);
	
	int32 mm = B_FOLLOW_TOP;
	xrv = new STextView(xpos,"XPos",mm|B_FOLLOW_LEFT,0);
	xrv->MakeEditable(false);
	xrv->MakeSelectable(false);
	yrv = new STextView(ypos,"YPos",mm|B_FOLLOW_LEFT,S_GOTO_LINEY);
	srv = new STextView(spos,"Search",mm|B_FOLLOW_LEFT,A_FIND_FORWARD);
	rrv = new STextView(rpos,"Replace",mm|B_FOLLOW_LEFT_RIGHT,A_REPLACE_THIS);
	
	AddChild(xrv);
	AddChild(yrv);
	AddChild(srv);
	AddChild(rrv);
	SetPos(0,0);
	SetViewColor(DARK,DARK,DARK);
}


StatusView::~StatusView()
{
}


void
StatusView::Draw(BRect update)
{
	BRect fr = Bounds();
	SetHighColor(DARK,DARK,DARK);//0,255,0);
	FillRect(fr);
}


void
StatusView::SetPos(int xx,int yy)
{
	char temp[16];
	if (xx != cvx)
	{
		cvx = xx;sprintf(temp,"%d",xx);
		xrv->SetText(temp);
	}
	if (yy != cvy)
	{
		cvy = yy;sprintf(temp,"%d",yy + 1);
		yrv->SetText(temp);
	}
}


void
StatusView::SetSearch(const char *src)
{
	if (src)
		srv->SetText(src);
}


void
StatusView::SetReplace(const char *src)
{
	if (src)
		rrv->SetText(src);
}


void
StatusView::MyNav(STextView *src,int dir)
{
	if (src == NULL)
	{
		((NiueWindow*)Window())->myview->MakeFocus();
		return;
	}
	dir = (dir>0)?1:-1;
	
	STextView *dest = xrv;
	int ss = 0;
	if (src == xrv)
		ss = 0;
	if (src == yrv)
		ss = 1;
	if (src == srv)
		ss = 2;
	if (src == rrv)
		ss = 3;
	ss = (ss + dir)&3;
	
	// TODO: This code will never execute cases 1, 2, or 3. Is this correct?
	switch (ss)
	{
		default:
			dest = xrv;
			break;
		case 1:
			dest = yrv;
			break;
		case 2:
			dest = srv;
			break;
		case 3:
			dest = rrv;
			break;
	}
	
	dest->MakeFocus();
	dest->SelectAll();
}


STextView::STextView(BRect frame, const char *name, uint32 resizeMask, int32 msg)
	:	BTextView(frame,name,frame,resizeMask,B_WILL_DRAW|B_NAVIGABLE)
{
	mymsg = msg;
	BRect in = Bounds();
	in.top += 3.0;
	in.bottom -= 2.0;
	in.left += 2.0;
	in.right -= 3.0;
	SetFontAndColor(be_plain_font2);
	SetViewColor(NORM,NORM,NORM);
	SetTextRect(in);
}


void
STextView::Draw(BRect update)
{
	BRect fr = Bounds();

	SetHighColor(DARK,DARK,DARK);
	StrokeLine(BPoint(fr.left,fr.top),BPoint(fr.right,fr.top));
	if (IsFocus())
	{
		fr.top += 1.0;
		SetLowColor(white);
		FillRect(fr,B_SOLID_LOW);
		SetHighColor(blue);
		StrokeRect(fr,B_SOLID_HIGH);
	}
	else
	{
		SetLowColor(DARK,DARK,DARK);
		SetHighColor(BRITE,BRITE,BRITE);
		StrokeLine(BPoint(fr.left,fr.top + 1.0),BPoint(fr.right,fr.top + 1.0));
		StrokeLine(BPoint(fr.left,fr.top + 2.0),BPoint(fr.left,fr.bottom));
	}
	
	BTextView::Draw(fr);
}


void
STextView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0])
	{
		case 9:	//tab
		{
			BMessage *ms = (Window())->CurrentMessage();
			int32 mod = 0;
			ms->FindInt32("modifiers",&mod);

			((StatusView*)Parent())->MyNav(this,!(mod&B_SHIFT_KEY));
			break;
		}
		case 10:    //cr
		{
			if (mymsg)
			{
				BMessage ms(mymsg);
				ms.AddString("Data",Text());
				Window()->PostMessage(&ms);
			}
			break;
		}
		case B_FUNCTION_KEY:
		{
			int32 key;
			Window()->CurrentMessage()->FindInt32("key",&key);
			if (key == B_F3_KEY)
				Window()->PostMessage(A_FIND_FORWARD);
			break;
		}
		case 27: //esc
		{
			((StatusView*)Parent())->MyNav(NULL,0);
			break;
		}
		default:
		{
			BTextView::KeyDown(bytes,numBytes);
			break;
		}
	}
}


void
STextView::MakeFocus(bool vv)
{
	if (vv)
		SetViewColor(white);
	else
		SetViewColor(NORM,NORM,NORM);
	
	BTextView::MakeFocus(vv);
	Invalidate(Bounds());
}


HintView::HintView(BRect hrect, const char *name)
	:	BView(hrect, name, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW)
{
	split = new Mover(BRect(0,0,5,hrect.Height()),B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM,true);
		
	lstHints = new BListView(BRect(9,0,hrect.Width()-B_V_SCROLL_BAR_WIDTH, hrect.Height()), "lstHints", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
	scrHints = new BScrollView("scrHints", lstHints, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true, B_NO_BORDER);
		
//	lstHints->AddList(&list);
	lstHints->SetHighColor(purple);
	lstHints->SetInvocationMessage(new BMessage(A_INSERT_HINT));
	
	AddChild(scrHints);		
	AddChild(split);
	
}


HintView::~HintView()
{
}

/*

HintWindow::HintWindow(BRect hrect, BList list)
	:BWindow(hrect, "HintWindow", B_BORDERED_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL, B_NAVIGABLE)
{

	lstHints = new BListView(BRect(0,0,hrect.Width()-B_V_SCROLL_BAR_WIDTH, hrect.Height()), "lstHints", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
	scrHints = new BScrollView("scrHints", lstHints, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true, B_NO_BORDER);
		
	lstHints->AddList(&list);
	lstHints->SetHighColor(purple);
//	lstHints->SetSelectionMessage(new BMessage(A_INSERT_HINT));
	lstHints->SetInvocationMessage(new BMessage(A_INSERT_HINT));
//	if (lstHints->CountItems() > 0) lstHints->Select(0);
	
	AddChild(scrHints);		
}


HintWindow::~HintWindow()
{
		
}

void
HintWindow::UpdateList(BList list)
{
	lstHints->MakeEmpty();
	lstHints->AddList(&list);
//	if (lstHints->CountItems() > 0) lstHints->Select(0);
}	


void
HintWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case A_INSERT_HINT:
		{
//			BString test = dynamic_cast<BStringItem*>(lstHints->ItemAt(lstHints->CurrentSelection()))->Text();
//			(new BAlert("Niue", test.String(), "Ok", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		
			BMessage msg(A_INSERT_HINT);
			msg.AddString("hint", dynamic_cast<BStringItem*>(lstHints->ItemAt(lstHints->CurrentSelection()))->Text());
			be_app->PostMessage(&msg);
			this->Quit();

		}
		break;
	}
	
	
}

*/
