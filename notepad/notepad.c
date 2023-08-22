/*
 *   Notepad application
 *   Copyright (C) 1984-2001 Microsoft Inc.
 */

// Without this the GUI elements will be ugly and look like win95
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "precomp.h"
//#include <HtmlHelp.h>

#define DeepTrouble() MessageBox(hwndNP, szErrSpace, szNN, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
BOOL MergeStrings(TCHAR*, TCHAR*, TCHAR*);

UINT     lGotoLine;                  /* line number to goto to */

TCHAR    chMerge;
HWND     hwndNP = 0;                 /* handle to notepad parent window   */
HWND     hwndStatus = 0;             /* handle to notepad status window   */
HWND     hwndEdit = 0;               /* handle to main text control item  */
HANDLE   hEdit;                      /* Handle to storage for edit item   */
HWND     hDlgFind = NULL;            /* handle to modeless FindText window */
HANDLE   hStdCursor;                 /* handle to arrow or beam cursor    */
HANDLE   hWaitCursor;                /* handle to hour glass cursor       */
HANDLE   hInstanceNP;                /* Module instance handle            */
HANDLE   hFont;                      /* handle to Unicode font            */
LOGFONT  FontStruct;                 /* font dialog structure             */
INT      iPointSize=120;             /* current point size unit=1/10 pts  */
TCHAR    szFileName[MAX_PATH];       /* Current notepad filename          */
TCHAR    szSearch[CCHKEYMAX];        /* Search string                     */
TCHAR    szReplace[CCHKEYMAX];       /* replace string                    */

BOOL     fUntitled = TRUE;           /* TRUE iff notepad has no title                  */
BOOL     fStatus = FALSE;            /* status bar shown?                              */
BOOL     fLastStatus = FALSE;        /* status bar status when wordwrap was turned off */
INT      dyStatus;                   /* height of status bar                           */

INT		iTabStops = 32;				/* width of tabs in the edit control, multiplied by 4 */

HMENU    hSysMenuSetup;              /* Save Away for disabled Minimize   */

DWORD    dwEmSetHandle = 0;          /* Is EM_SETHANDLE in process?       */
HANDLE   hAccel;                     /* Handle to accelerator table       */
BOOL     fRunBySetup = FALSE;        /* Did SlipUp WinExec us??           */
BOOL     fWrap = 0;                  /* Flag for word wrap                */
TCHAR    szNotepad[] = TEXT("Notepad");/* Name of notepad window class    */

BOOL     fInSaveAsDlg = FALSE;

// Edit control used to AV is EM_FMTLINES was turned off when cursor was near the end
// To get around this, notepad moved to cursor to 0,0 when it turned off wordwrap.
// Users were not happy, so we will put up with possible AVs.  Note: as of June 27, 2001
// we could not repro the AV behavior, so perhaps it is not there anymore.

BOOL     fMLE_is_broken= FALSE; 

BOOL	 fWindowsOnlyEOL = FALSE;

/* variables for the new File/Open, File/Saveas,Find Text and Print dialogs */
OPENFILENAME OFN;                     /* passed to the File Open/save APIs */
TCHAR szOpenFilterSpec[CCHFILTERMAX]; /* default open filter spec          */
TCHAR szSaveFilterSpec[CCHFILTERMAX]; /* default save filter spec          */
NP_FILETYPE g_ftOpenedAs = FT_UNKNOWN;/* current file was opened           */
NP_FILETYPE g_ftSaveAs;               /* current file was opened           */
// these two are automatically handled and aren't offered as options in the dialogs
NP_LINETYPE g_ltOpenedAs = LT_WINDOWS;
NP_LINETYPE g_ltSaveAs; // unused for now

BOOL g_bDirty = FALSE;

FINDREPLACE FR;                       /* Passed to FindText()              */
PAGESETUPDLG g_PageSetupDlg;
UINT wFRMsg;                          /* message used in communicating     */
                                      /* with Find/Replace dialog          */

DWORD dwCurrentSelectionStart = 0L;   /* WM_ACTIVATEAPP selection pos      */
DWORD dwCurrentSelectionEnd   = 0L;   /* WM_ACTIVATEAPP selection pos      */
UINT wHlpMsg;                         /* message used in invoking help     */

/* Strings loaded from resource file passed to LoadString at initialization time */
/* To add resource string:
 * 1) create IDS_ macro definition in notepad.h
 * 2) create string in resource file
 * 3) create 'TCHAR*' variable directly below and in notepad.h file
 * 4) add &variable to rgsz
 * 5) increment CSTRINGS
 */
TCHAR *szDiskError =(TCHAR *)IDS_DISKERROR;  /* Can't open File, check disk  */
TCHAR *szFNF       =(TCHAR *)IDS_FNF;        /* File not found               */
TCHAR *szSCBC      =(TCHAR *)IDS_SCBC;       /* Save changes before closing? */
TCHAR *szUntitled  =(TCHAR *)IDS_UNTITLED;   /* Untitled                     */
TCHAR *szNpTitle   =(TCHAR *)IDS_NOTEPAD;    /* Notepad -                    */
TCHAR *szCFS       =(TCHAR *)IDS_CFS;        /* Can't find string            */
TCHAR *szErrSpace  =(TCHAR *)IDS_ERRSPACE;   /* Memory space exhausted       */
TCHAR *szFTL       =(TCHAR *)IDS_FTL;        /* File too large for notepad   */
TCHAR *szNN        =(TCHAR *)IDS_NN;         /* Notepad name                 */

TCHAR *szCommDlgInitErr = (TCHAR*)IDS_COMMDLGINIT; /* common dialog error %x */
TCHAR *szPDIE      =(TCHAR*) IDS_PRINTDLGINIT; /* Print dialog init error    */
TCHAR *szCP        =(TCHAR*) IDS_CANTPRINT;  /* Can't print                  */
TCHAR *szNVF       =(TCHAR*) IDS_NVF;        /* Not a valid filename.        */
TCHAR *szCREATEERR =(TCHAR*) IDS_CREATEERR;  /* cannot create file           */
TCHAR *szNoWW      =(TCHAR*) IDS_NOWW;       /* Too much text to word wrap   */
TCHAR *szMerge     =(TCHAR*) IDS_MERGE1;     /* search string for merge      */
TCHAR *szHelpFile  =(TCHAR*) IDS_HELPFILE;   /* Name of helpfile.            */
TCHAR *szHeader    =(TCHAR*) IDS_HEADER;
TCHAR *szFooter    =(TCHAR*) IDS_FOOTER;
TCHAR *szLetters   =(TCHAR*) IDS_LETTERS;    /* formatting letters in pagesetup */

TCHAR *szAnsiText    = (TCHAR*)IDS_ANSITEXT; /* File/Open ANSI filter spec. string */
TCHAR *szAllFiles    = (TCHAR*)IDS_ALLFILES;   /* File/Open Filter spec. string */
TCHAR *szOpenCaption = (TCHAR*)IDS_OPENCAPTION; /* caption for File/Open dlg */
TCHAR *szSaveCaption = (TCHAR*)IDS_SAVECAPTION; /* caption for File/Save dlg */
TCHAR *szCannotQuit  = (TCHAR*)IDS_CANNOTQUIT;  /* cannot quit during a WM_QUERYENDSESSION */
TCHAR *szLoadDrvFail = (TCHAR*)IDS_LOADDRVFAIL;  /* LOADDRVFAIL from PrintDlg */
TCHAR *szACCESSDENY  = (TCHAR*)IDS_ACCESSDENY; /* Access denied on Open */
TCHAR *szErrUnicode  = (TCHAR*)IDS_ERRUNICODE;  /* Unicode character existence error */
TCHAR *szFontTooBig  = (TCHAR*)IDS_FONTTOOBIG; /* font too big or page too small */

TCHAR *szCommDlgErr   = (TCHAR*) IDS_COMMDLGERR; /* common dialog error %x */
TCHAR *szLineError    = (TCHAR*) IDS_LINEERROR;   /* line number error        */
TCHAR *szLineTooLarge = (TCHAR*) IDS_LINETOOLARGE;/* line number out of range */
TCHAR *szFtAnsi       = (TCHAR*) IDS_FT_ANSI;
TCHAR *szFtUnicode    = (TCHAR*) IDS_FT_UNICODE;
TCHAR *szFtUnicodeBe  = (TCHAR*) IDS_FT_UNICODEBE;
TCHAR *szFtUtf8       = (TCHAR*) IDS_FT_UTF8;
TCHAR *szCurrentPage  = (TCHAR*) IDS_CURRENT_PAGE;

// strings for the status bar
TCHAR *szLineCol        = (TCHAR*) IDS_LINECOL;
TCHAR *szCompressedFile = (TCHAR*) IDS_COMPRESSED_FILE;  
TCHAR *szEncryptedFile  = (TCHAR*) IDS_ENCRYPTED_FILE;   
TCHAR *szHiddenFile     = (TCHAR*) IDS_HIDDEN_FILE;      
TCHAR *szOfflineFile    = (TCHAR*) IDS_OFFLINE_FILE;     
TCHAR *szReadOnlyFile   = (TCHAR*) IDS_READONLY_FILE;    
TCHAR *szSystemFile     = (TCHAR*) IDS_SYSTEM_FILE;      
TCHAR *szFile           = (TCHAR*) IDS_FILE;
TCHAR *szWindowsFile	= (TCHAR*) IDS_LT_WINDOWS;
TCHAR *szUnixFile		= (TCHAR*) IDS_LT_UNIX;


TCHAR **rgsz[CSTRINGS] = {
        &szDiskError,
        &szFNF,
        &szSCBC,
        &szUntitled,
        &szErrSpace,
        &szCFS,
        &szNpTitle,
        &szFTL,
        &szNN,
        &szCommDlgInitErr,
        &szPDIE,
        &szCP,
        &szNVF,
        &szCREATEERR,
        &szNoWW,
        &szMerge,
        &szHelpFile,
        &szAnsiText,
        &szAllFiles,
        &szOpenCaption,
        &szSaveCaption,
        &szCannotQuit,
        &szLoadDrvFail,
        &szACCESSDENY,
        &szErrUnicode,
        &szCommDlgErr,
        &szFontTooBig,
        &szLineError,
        &szLineTooLarge,
        &szFtAnsi,
        &szFtUnicode,
        &szFtUnicodeBe,
        &szFtUtf8,
        &szCurrentPage,
        &szHeader,
        &szFooter,
        &szLineCol,
        &szCompressedFile,
        &szEncryptedFile,
        &szHiddenFile,
        &szOfflineFile,
        &szReadOnlyFile,
        &szSystemFile,
        &szFile,
        &szLetters,
		&szWindowsFile,
		&szUnixFile,
};


HANDLE   fp;          /* file pointer */

WNDPROC	DefEditWindowProc;


#if 0
VOID DisplayFont( LOGFONT* pf )
{
    TCHAR dbuf[100];

    ODS(TEXT("-----------------------\n"));
    wsprintf(dbuf,TEXT("lfHeight          %d\n"),pf->lfHeight); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWidth           %d\n"),pf->lfWidth ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfEscapement      %d\n"),pf->lfEscapement); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOrientation     %d\n"),pf->lfOrientation); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWeight          %d\n"),pf->lfWeight); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfItalic          %d\n"),pf->lfItalic); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfUnderLine       %d\n"),pf->lfUnderline); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfStrikeOut       %d\n"),pf->lfStrikeOut); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfCharSet         %d\n"),pf->lfCharSet); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOutPrecision    %d\n"),pf->lfOutPrecision); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfClipPrecision   %d\n"),pf->lfClipPrecision); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfQuality         %d\n"),pf->lfQuality); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfPitchAndFamily  %d\n"),pf->lfPitchAndFamily); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfFaceName        %s\n"),pf->lfFaceName); ODS(dbuf);

}
#endif

static TCHAR  szPath[MAX_PATH];

void FileDragOpen(void);
VOID NpResetMenu(HWND hWnd);
BOOL SignalCommDlgError(VOID);
VOID ReplaceSel( BOOL bView );

/* FreeGlobal, frees  all global memory allocated. */

void NEAR PASCAL FreeGlobal()
{
    if(g_PageSetupDlg.hDevMode)
    {
        GlobalFree(g_PageSetupDlg.hDevMode);
    }

    if(g_PageSetupDlg.hDevNames)
    {
        GlobalFree(g_PageSetupDlg.hDevNames);
    }

    g_PageSetupDlg.hDevMode=  NULL; // make sure they are zero for PrintDlg
    g_PageSetupDlg.hDevNames= NULL;
}

VOID PASCAL SetPageSetupDefaults( VOID )
{
    TCHAR szIMeasure[ 2 ];

    g_PageSetupDlg.lpfnPageSetupHook= PageSetupHookProc;
    g_PageSetupDlg.lpPageSetupTemplateName= MAKEINTRESOURCE(IDD_PAGESETUP);

    GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szIMeasure, 2 );

    g_PageSetupDlg.Flags= PSD_MARGINS  |
            PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGESETUPTEMPLATE;

    if (szIMeasure[ 0 ] == TEXT( '1' ))
    {
        //  English measure (in thousandths of inches).
        g_PageSetupDlg.Flags |= PSD_INTHOUSANDTHSOFINCHES;
        g_PageSetupDlg.rtMargin.top    = 1000;
        g_PageSetupDlg.rtMargin.bottom = 1000;
        g_PageSetupDlg.rtMargin.left   = 750;
        g_PageSetupDlg.rtMargin.right  = 750;
    }
    else
    {
        //  Metric measure (in hundreths of millimeters).
        g_PageSetupDlg.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
        g_PageSetupDlg.rtMargin.top    = 2500;
        g_PageSetupDlg.rtMargin.bottom = 2500;
        g_PageSetupDlg.rtMargin.left   = 2000;
        g_PageSetupDlg.rtMargin.right  = 2000;
    }

}

/* Standard window size proc */
void NPSize (int cxNew, int cyNew)
{
    /* Invalidate the edit control window so that it is redrawn with the new
     * margins. Needed when comming up from iconic and when doing word wrap so
     * the new margins are accounted for.
     */

	// NotepadEx: This InvalidateRect call causes flickering on window resize.
	// It also doesn't seem to be necessary at all for the stated reasons in
	// the original comment, above.
	// Furthermore, this call seems to have been removed on Windows 7.
	// After testing on XP and 2000 it doesn't seem to be needed there either.
    //InvalidateRect(hwndEdit, (LPRECT)NULL, TRUE);

    // the height of the edit window depends on whether the status bar is
    // displayed.
    MoveWindow(hwndEdit, 0, 0, cxNew, cyNew - (fStatus ? dyStatus : 0), TRUE);
}

// NpSaveDialogHookProc
//
// Common dialog hook procedure for handling
// the file type while saving.
//

const DWORD s_SaveAsHelpIDs[]=
    {
        IDC_FILETYPE, IDH_FILETYPE,
        IDC_ENCODING, IDH_FILETYPE,
        0, 0
    };

UINT_PTR APIENTRY NpSaveDialogHookProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    INT id;
    POINT pt;
    TCHAR* szSelect;        // selected type

    switch( msg )
    {
        case WM_INITDIALOG:
            // Warning: the order here must be the same as NP_FILETYPE

            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtAnsi );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicode );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicodeBe );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUtf8 );

            szSelect= szFtAnsi;         // default
            g_ftSaveAs= g_ftOpenedAs;   // default: save as same type as opened
            switch( g_ftSaveAs )
            {
                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                case FT_UTF8:      szSelect= szFtUtf8;      break;
                default: break;
            }

            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );
            break;

        case WM_COMMAND:
            g_ftSaveAs= (NP_FILETYPE) SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_GETCURSEL, 0, 0 );
            break;

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle,
                      szHelpFile,
                      HELP_WM_HELP,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  If the user clicks on any of our labels, then the wParam will
            //  be the hwnd of the dialog, not the static control.  WinHelp()
            //  handles this, but because we hook the dialog, we must catch it
            //  first.
            //
            if( hWnd == (HWND) wParam )
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                wParam = (WPARAM) ChildWindowFromPoint(hWnd, pt);
            }

            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID((HWND) wParam);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp( (HWND)   wParam,
                              szHelpFile,
                              HELP_CONTEXTMENU,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;
    }
    return( FALSE );
}


// NpOpenDialogHookProc
//
// Common dialog hook procedure for handling
// the file type while opening.
//

UINT_PTR APIENTRY NpOpenDialogHookProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    INT id;
    POINT pt;
    TCHAR* szSelect;        // selected type
    static TCHAR  szPrevFileName[MAX_PATH] = TEXT(""); 

    switch( msg )
    {
        case WM_INITDIALOG:
            // Warning: the order here must be the same as NP_FILETYPE

            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtAnsi );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicode );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicodeBe );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUtf8 );

            szSelect= szFtAnsi;         // default
            switch( g_ftOpenedAs )
            {
                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                case FT_UTF8:      szSelect= szFtUtf8;      break;
                default: break;
            }

            // set the current filetype.
            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );
            break;

        case WM_COMMAND:
            g_ftOpenedAs= (NP_FILETYPE) SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_GETCURSEL, 0, 0 );
            break;

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle,
                      szHelpFile,
                      HELP_WM_HELP,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  If the user clicks on any of our labels, then the wParam will
            //  be the hwnd of the dialog, not the static control.  WinHelp()
            //  handles this, but because we hook the dialog, we must catch it
            //  first.
            //
            if( hWnd == (HWND) wParam )
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                wParam = (WPARAM) ChildWindowFromPoint(hWnd, pt);
            }

            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID((HWND) wParam);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp( (HWND)   wParam,
                              szHelpFile,
                              HELP_CONTEXTMENU,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

            
         case WM_NOTIFY:
         {
            LPOFNOTIFY pofn;
            TCHAR      szFileName[MAX_PATH]; 
            BYTE       szFileBuffer[BUFFER_TEST_SIZE];
            HANDLE     hFile;
            DWORD      dwBytesRead;
            
            // process the message when the file selection changes.
            pofn = (LPOFNOTIFY)lParam;
            switch (pofn->hdr.code)
            {
                case CDN_SELCHANGE:
                {                        
                    // get the filename.
                    if (CommDlg_OpenSave_GetFilePath(GetParent(hWnd), szFileName, sizeof(szFileName)/sizeof(TCHAR)) > 0)
                    {
                        // if same file as the previous file, don't do anything.
                        if (lstrcmpi(szFileName, szPrevFileName) == 0)
                            break;

                        // open the file.
                        hFile = CreateFile(szFileName,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE)
                        {
                            // if the file read fails, just quit.
                            if ( (ReadFile(hFile, szFileBuffer, BUFFER_TEST_SIZE, &dwBytesRead, NULL) <= 0) || dwBytesRead == 0)
                            {
                                CloseHandle(hFile);
                                break;
                            }

                            // determine the file type based on dwBytesRead bytes of the file.
                            g_ftOpenedAs = fDetermineFileType(szFileBuffer, dwBytesRead);

                            // set the dropdown filetype to the filetype we think based on the initial part of the file.
                            szSelect = szFtAnsi;         // default
                            switch( g_ftOpenedAs )
                            {
                                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                                case FT_UTF8:      szSelect= szFtUtf8;      break;
                                default: break;
                            }

                            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );            

                            // cleanup.
                            lstrcpy(szPrevFileName, szFileName);
                            CloseHandle(hFile);                           

                        }
                    }                                    

                    break;
                }
            }

         }
                    
    }
    return( FALSE );
}

// Added for NotepadEx.
// Get the 0-based index of the character at the start of a given line.
// The line number is 0-based. Returns -1 if no given line is present.
ULONG GetEditControlRealCharacterIndexFromLineNumber(
	IN	HWND	Window,
	IN	ULONG	LineNumber)
{
	HLOCAL EditControlBufferHandle;
	PWSTR EditControlBuffer;
	ULONG Index;
	UINT LineNumberTemp;

	if (LineNumber == 0) {
		return 0;
	}

	Index = 0;
	LineNumberTemp = 0;

	EditControlBufferHandle = Edit_GetHandle(Window);

	if (!EditControlBufferHandle) {
		return -1;
	}

	EditControlBuffer = (PWSTR) LocalLock(EditControlBufferHandle);

	if (!EditControlBuffer) {
		return -1;
	}

	while (TRUE) {
		if (EditControlBuffer[Index] == '\0') {
			Index = -2; // we're gonna add 1 to it
			break;
		}

		if (EditControlBuffer[Index] == '\n') {
			++LineNumberTemp;

			if (LineNumberTemp == LineNumber) {
				break;
			}
		}

		++Index;
	}

	LocalUnlock(EditControlBufferHandle);

	return Index + 1;
}

// GotoAndScrollInView
//
// Put the cursor at the begining of a line, and scroll the
// editbox so the user can see it.
//
// If there is a failure, it just leaves the cursor where it is.
// Return TRUE on success and FALSE on failure.
//
BOOLEAN GotoAndScrollInView( INT OneBasedLineNumber )
{
    UINT CharIndex;

	if (OneBasedLineNumber == 0) {
		OneBasedLineNumber = 1;
	}

	if (fWrap) {
		// NotepadEx: Different algorithm is required when we go to a
		// specific line when word wrap is enabled. The edit control
		// thinks in terms of on-screen lines, whereas we want to use
		// real line numbers (as in, # of newline characters).

		CharIndex = GetEditControlRealCharacterIndexFromLineNumber(
			hwndEdit,
			OneBasedLineNumber - 1);
	} else {
		CharIndex = (UINT) SendMessage(	hwndEdit,
										EM_LINEINDEX,
										OneBasedLineNumber-1,
										0 );
	}

	if (CharIndex == -1) {
		return FALSE;
	}
    
    SendMessage( hwndEdit, EM_SETSEL, CharIndex, CharIndex);
    SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
	return TRUE;
}

// NotepadEx: added for Vista+ file open/save dialog
// {9BEB83A7-0F24-4afe-B04E-D22A0DAF41FD}
DEFINE_GUID(IID_IFileDialogEvents, 
0x9beb83a7, 0xf24, 0x4afe, 0xb0, 0x4e, 0xd2, 0x2a, 0xd, 0xaf, 0x41, 0xfd);
HRESULT STDMETHODCALLTYPE fdeQueryInterface(IFileDialogEvents *pfde, REFIID riid, void **ppv) {
	if (!IsEqualIID(riid, &IID_IFileDialogEvents)) {
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	*ppv = pfde;
	pfde->lpVtbl->AddRef(pfde);
	return S_OK;
}

ULONG STDMETHODCALLTYPE fdeAddRef(IFileDialogEvents *pfde) {
	return 1;
}

ULONG STDMETHODCALLTYPE fdeRelease(IFileDialogEvents *pfde) {
	GlobalFree(pfde);
	return 0;
}

HRESULT STDMETHODCALLTYPE OnFileOk(IFileDialogEvents *pfde, IFileDialog *pfd) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE OnFolderChange(IFileDialogEvents *pfde, IFileDialog *pfd) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE OnFolderChanging(IFileDialogEvents *pfde, IFileDialog *pfd, IShellItem *psi) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE OnOverwrite(IFileDialogEvents *pfde, IFileDialog *pfd, IShellItem *psi, FDE_OVERWRITE_RESPONSE *pfor) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE OnShareViolation(IFileDialogEvents *pfde, IFileDialog *pfd, IShellItem *psi, FDE_SHAREVIOLATION_RESPONSE *pfsr) {
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE OnTypeChange(IFileDialogEvents *pfde, IFileDialog *pfd) {
	return E_NOTIMPL;
}

#define pfdCall(func, ...) if (!SUCCEEDED(pfd->lpVtbl->func(pfd, __VA_ARGS__))) return E_FAIL;
#define pfdCallNA(func) if (!SUCCEEDED(pfd->lpVtbl->func(pfd))) return E_FAIL;
#define pfdcCall(func, ...) if (!SUCCEEDED(pfdc->lpVtbl->func(pfdc, __VA_ARGS__))) return E_FAIL;
#define pfdcCallNA(func) if (!SUCCEEDED(pfdc->lpVtbl->func(pfdc))) return E_FAIL;
#define psiCall(func, ...) if (!SUCCEEDED(psi->lpVtbl->func(psi, __VA_ARGS__))) return E_FAIL;

IFileDialogCustomize *pfdc = NULL;
const DWORD dwIDEncodingLabel = 0;
const DWORD dwIDEncodingComboBox = 1;

// Dialog hook for the NEW STYLE open dialog, run whenever the user selects a new item
HRESULT STDMETHODCALLTYPE OnSelectionChange(IFileDialogEvents *pfde, IFileDialog *pfd) {
	TCHAR szFileName[MAX_PATH];
	LPWSTR szTempName;
	static TCHAR szPrevFileName[MAX_PATH] = TEXT(""); 
	HANDLE hFile;
	DWORD dwBytesRead;
	IShellItem *psi;

	// get the filename.
	pfdCall(GetCurrentSelection, &psi);
	psiCall(GetDisplayName, SIGDN_FILESYSPATH, &szTempName);
	_tcsncpy(szFileName, szTempName, sizeof(szFileName)/sizeof(TCHAR));
	CoTaskMemFree(szTempName); szTempName = NULL;

	if (_tcslen(szFileName) > 0)
	{
		// if same file as the previous file, don't do anything.
		if (lstrcmpi(szFileName, szPrevFileName) == 0)
			return S_OK;

		if (!fInSaveAsDlg) { // File open dialog
			// open the file.
			hFile = CreateFile(szFileName,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				BYTE szFileBuffer[BUFFER_TEST_SIZE];

				if ( (ReadFile(hFile, szFileBuffer, BUFFER_TEST_SIZE, &dwBytesRead, NULL) <= 0) || dwBytesRead == 0) {
					// if the file read fails, just quit.
					CloseHandle(hFile);
					return E_FAIL;
				}

				// determine the file type based on dwBytesRead bytes of the file.
				g_ftOpenedAs = fDetermineFileType(szFileBuffer, dwBytesRead);

				switch (g_ftOpenedAs) {
				case FT_ANSI:
				case FT_UNICODE:
				case FT_UNICODEBE:
				case FT_UTF8: pfdcCall(SetSelectedControlItem, dwIDEncodingComboBox, g_ftOpenedAs); break;
				default: break;
				}

				// cleanup.
				lstrcpy(szPrevFileName, szFileName);
				CloseHandle(hFile);
			}
		}
	}

	psi->lpVtbl->Release(psi);
	return S_OK;
}

static IFileDialogEventsVtbl IFileDialogEvents_Vtbl = {
	fdeQueryInterface,
	fdeAddRef,
	fdeRelease,
	OnFileOk,
	OnFolderChanging,
	OnFolderChange,
	OnSelectionChange,
	OnShareViolation,
	OnTypeChange,
	OnOverwrite
};

#undef pfdCall
#undef pfdCallNA
#undef pfdcCall
#undef pfdcCallNA
#define pfdCall(func, ...) if (!SUCCEEDED(pfd->lpVtbl->func(pfd, __VA_ARGS__))) return FALSE;
#define pfdCallNA(func) if (!SUCCEEDED(pfd->lpVtbl->func(pfd))) return FALSE;
#define pfdcCall(func, ...) if (!SUCCEEDED(pfdc->lpVtbl->func(pfdc, __VA_ARGS__))) return FALSE;
#define pfdcCallNA(func) if (!SUCCEEDED(pfdc->lpVtbl->func(pfdc))) return FALSE;

// show a new-style or old-style file open/save dialog
// return true on success, false on failure (incl. user canceled)
BOOL NPOpenSave(
	_Out_ LPTSTR szNewName,
	_In_ SIZE_T BufferSize)
{
	IFileDialog *pfd = NULL;

	if (SUCCEEDED(CoCreateInstance(fInSaveAsDlg ? &CLSID_FileSaveDialog : &CLSID_FileOpenDialog,
								   NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, &pfd))) {
		// use new style dialog (vista or win7)
		IShellItem *psiResult = NULL;
		IFileDialogEvents *pfde = NULL;
		DWORD dwCookie;
		DWORD dwSelectedEncoding;
		FILEOPENDIALOGOPTIONS dwFlags = 0;
		LPWSTR szName = NULL;
		COMDLG_FILTERSPEC filterFileExt[] = {
//			{L"Text", L"*.ahk;*.asc;*.asm;*.bas;*.bat;*.c;*.c++;*.cpp;*.cc;*.cfg;*.cmd;*.cs;*.css;*.csv;*.def;*.dlg;*.f03;*.f08;*.f18;*.f4;*.f77;*.f90;*.f95;*.for;*.go;*.h;*.h++;*.hh;*.hpp;*.hta;*.htm;*.html;*.hxx;*.idl;*.ini;*.jl;*.java;*.js;*.json;*.lisp;*.lua;*.m4;*.md;*.nfo;*.nim;*.pas;*.pem;*.php;*.php3;*.php4;*.ps;*.rb;*.rc;*.reg;*.rgs;*.rs;*.rst;*.s;*.scala;*.scm;*.sh;*.shar;*.shtm;*.shtml;*.sl;*.svg;*.txt;*vb;*.vbox;*.vbs;*.y;*.yml;*.yaml"},
			{L"Text Documents", L"*.txt"},
			{L"All Files", L"*.*"}
		};

		// event handler is only necessary for the open dialog
		if (!fInSaveAsDlg) {
			// set up the event handler (see OnSelectionChange)
			if (!(pfde = GlobalAlloc(GMEM_FIXED, sizeof(*pfde)))) return FALSE;
			pfde->lpVtbl = &IFileDialogEvents_Vtbl;
			pfdCall(Advise, pfde, &dwCookie);
		}

		// setup the encoding selector combo box
		pfdCall(QueryInterface, &IID_IFileDialogCustomize, &pfdc);
		pfdcCall(StartVisualGroup, dwIDEncodingLabel, TEXT("&Encoding:"));
		pfdcCall(AddComboBox, dwIDEncodingComboBox);
		pfdcCall(AddControlItem, dwIDEncodingComboBox, FT_ANSI, szFtAnsi);
		pfdcCall(AddControlItem, dwIDEncodingComboBox, FT_UNICODE, szFtUnicode); // aka UTF-16 LE
		pfdcCall(AddControlItem, dwIDEncodingComboBox, FT_UNICODEBE, szFtUnicodeBe); // aka UTF-16 BE
		pfdcCall(AddControlItem, dwIDEncodingComboBox, FT_UTF8, szFtUtf8);
		
		switch (g_ftOpenedAs) {
		case FT_UNICODE:  //fallthrough
		case FT_UNICODEBE://fallthrough
		case FT_UTF8:		pfdcCall(SetSelectedControlItem, dwIDEncodingComboBox, g_ftOpenedAs); break;
		default:			pfdcCall(SetSelectedControlItem, dwIDEncodingComboBox, 0); break;
		}
		
		pfdcCall(MakeProminent, dwIDEncodingLabel);
		pfdcCallNA(EndVisualGroup);

		pfdCall(GetOptions, &dwFlags);
		pfdCall(SetOptions, dwFlags | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);

		if (fInSaveAsDlg) {
			// Win7 notepad automatically populates the field with the last used filename
			// (not including the full file path) or "*.txt" if no filename exists
			const TCHAR *BareFileName = PathFindFileName(szFileName);

			if (_tcslen(BareFileName) == 0)
				BareFileName = TEXT("*.txt");

			pfdCall(SetFileName, BareFileName);
		}

		pfdCall(SetFileTypes, ARRAYSIZE(filterFileExt), filterFileExt);
		pfdCall(SetFileTypeIndex, 1); // 1-based - so this will set the first entry in filterFileExt table
		pfdCall(SetDefaultExtension, L"txt");
		pfdCall(Show, hwndNP);
		pfdCall(GetResult, &psiResult);
		pfdcCall(GetSelectedControlItem, dwIDEncodingComboBox, &dwSelectedEncoding);
		
		if (fInSaveAsDlg) {
			g_ftSaveAs = (NP_FILETYPE) dwSelectedEncoding;
		} else {
			g_ftOpenedAs = (NP_FILETYPE) dwSelectedEncoding;
		}

		if (SUCCEEDED(psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, &szName))) {
			_tcsncpy(szNewName, szName, (MAX_PATH-1)/sizeof(TCHAR));
			CoTaskMemFree(szName);
			psiResult->lpVtbl->Release(psiResult);
			pfdc->lpVtbl->Release(pfdc);
			if (!fInSaveAsDlg) pfdCall(Unadvise, dwCookie);
			pfdCallNA(Release);
			return TRUE;
		}

		return FALSE;
	} else {
		// use old style dialog (windows xp style)

		if (fInSaveAsDlg) {
			OFN.lpstrFile       = szNewName;
            OFN.lpstrTitle      = szSaveCaption;
            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             * 12 February 1991    clarkc
             */
            OFN.Flags = OFN_HIDEREADONLY     | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST   |
                        OFN_EXPLORER         |
                        OFN_ENABLESIZING     |
                        OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

            OFN.lpTemplateName= TEXT("NpEncodingDialog");
            OFN.lpfnHook= NpSaveDialogHookProc;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             * 12 February 1991    clarkc
             */
            OFN.lpstrFilter       = szSaveFilterSpec;
            OFN.lpstrDefExt       = TEXT("txt");
			OFN.nFilterIndex= FILE_TEXT;
			return GetSaveFileName(&OFN);
		} else {
			/* set up the variable fields of the OPENFILENAME struct.
			 * (the constant fields have been set in NPInit()
			 */
	        OFN.lpstrFile         = szNewName;
	        lstrcpy(szNewName, TEXT("*.txt") ); /* set default selection */
	        OFN.lpstrTitle        = szOpenCaption;

	        /* ALL non-zero long pointers must be defined immediately
	         * before the call, as the DS might move otherwise.
	         * 12 February 1991    clarkc
	         */
	        OFN.lpstrFilter       = szOpenFilterSpec;
	        OFN.lpstrDefExt       = TEXT("txt");
	        /* Added OFN_FILEMUSTEXIST to eliminate problems in LoadFile.
	         * 12 February 1991    clarkc
	         */
	        OFN.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
	                             OFN_EXPLORER         |
	                             OFN_ENABLESIZING     |
	                             OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

	        OFN.nFilterIndex   = FILE_TEXT;
              
	        // show encoding listbox
	        OFN.lpTemplateName= TEXT("NpEncodingDialog");
	        OFN.lpfnHook= NpOpenDialogHookProc;
			return GetOpenFileName(&OFN);
		}
	}
}

// NotepadEx addition.
// Copy the selected text out of an edit control into a buffer.
// If there is nothing selected OR an error occured, an empty string
// is placed into the buffer and 0 is returned. Otherwise, the length
// of the selection is returned.
// If the size of the selection is larger than the size of the buffer,
// nothing will be copied and 0 is returned.
// The length returned is in characters, not bytes.
ULONG CopyEditControlSelectionText(
	HWND	Window,
	PWCHAR	Buffer,
	ULONG	BufferCch)
{
	ULONG SelectionStart;
	ULONG SelectionEnd;
	ULONG SelectionLength;
	HLOCAL EditControlBufferHandle;
	PWCHAR EditControlBuffer;

	if (BufferCch == 0) {
		// 0 is an invalid parameter.
		return 0;
	}

	// Initialize buffer with empty string
	Buffer[0] = '\0';

	// Retrieve the length of the selected text.
	SendMessage(Window, EM_GETSEL, (WPARAM) &SelectionStart, (LPARAM) &SelectionEnd);
	SelectionLength = SelectionEnd - SelectionStart;

	if (SelectionLength == 0) {
		// There is no selection.
		return 0;
	} else if (SelectionLength > (BufferCch - 1)) {
		// The selection length is longer than what would fit into the buffer.
		return 0;
	}

	// Lock the edit control's buffer so we can get a direct pointer to
	// the text inside.
	
	EditControlBufferHandle = Edit_GetHandle(Window);

	if (!EditControlBufferHandle) {
		return 0;
	}

	EditControlBuffer = (PWCHAR) LocalLock(EditControlBufferHandle);

	if (!EditControlBuffer) {
		return 0;
	}

	RtlCopyMemory(Buffer, &EditControlBuffer[SelectionStart], SelectionLength * sizeof(WCHAR));
	Buffer[SelectionLength] = '\0';
	LocalUnlock(EditControlBufferHandle);

	return SelectionLength;
}

UINT_PTR CALLBACK FindReplaceHookProc(
	IN	HWND	Dialog,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	STATIC LPFINDREPLACE FindReplaceInfo;

	switch (Message) {
	case WM_INITDIALOG:
		FindReplaceInfo = (LPFINDREPLACE) LParam;
		CheckDlgButton(Dialog, chx1, fWholeWord);
		CheckDlgButton(Dialog, chx2, fCase);
		CheckDlgButton(Dialog, chx3, fWrapAround);
		return TRUE;
	case WM_COMMAND:
		switch (WParam) {
		case chx1: // Whole word only
			fWholeWord = IsDlgButtonChecked(Dialog, (int) WParam);
			return FALSE;
		case chx2: // Match case
			fCase = IsDlgButtonChecked(Dialog, (int) WParam);
			return FALSE;
		case chx3: // Wrap around
			fWrapAround = IsDlgButtonChecked(Dialog, (int) WParam);
			return TRUE;
		}

		break;
	}

	return FALSE;
}

/* ** Notepad command proc - called whenever notepad gets WM_COMMAND
      message.  wParam passed as cmd */
INT NPCommand(
    HWND     hwnd,
    WPARAM   wParam,
    LPARAM   lParam )
{
    HWND     hwndFocus;
    LONG     lSel;
    TCHAR    szNewName[MAX_PATH] = TEXT("");      /* New file name */
    LONG     style;
    DWORD    rc;
    RECT     rcClient;
	IHxHelpPane *php = NULL;
	BSTR helpURI = NULL;
	CLSID CLSID_HxHelpPane;
	IID IID_IHxHelpPane;

    switch (LOWORD(wParam))
    {
        case M_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case M_NEW:
            New(TRUE);
            break;

		case M_NEWWIN:

			NewWindow();
			break;

        case M_OPEN:
            if (CheckSave(FALSE))
            {
                NP_FILETYPE  g_ftOldOpenedAs = g_ftOpenedAs;

                if( NPOpenSave(szNewName, MAX_PATH) )
                {
                   HANDLE oldfp= fp;                  

                   fp= CreateFile( szNewName,            // filename
                                   GENERIC_READ,         // access mode
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   NULL,                 // security descriptor
                                   OPEN_EXISTING,        // how to create
                                   FILE_ATTRIBUTE_NORMAL,// file attributes
                                   NULL);                // hnd to file attrs
                   /* Try to load the file and reset fp if failed */
                   if( !LoadFile( szNewName, g_ftOpenedAs ) )
                   {
                      fp= oldfp;
                      g_ftOpenedAs = g_ftOldOpenedAs;
                   }
                }
                else
                {
                    g_ftOpenedAs = g_ftOldOpenedAs;
                    SignalCommDlgError();
                }
            }
            break;

        case M_SAVE:
            /* set up the variable fields of the OPENFILENAME struct.
             * (the constant fields have been sel in NPInit()
             */
            g_ftSaveAs = g_ftOpenedAs;
            if( !fUntitled && SaveFile( hwndNP, szFileName, FALSE ) )
               break;

            /* fall through if there is no save name, or if the file
			 * save fails (e.g. no write permission) */

        case M_SAVEAS:
            if (!fUntitled)
            {
                lstrcpyn(szNewName, szFileName, MAX_PATH); /* set default selection */
            }
            else
            {
                lstrcpy (szNewName, TEXT("*.txt") );
            }

            fInSaveAsDlg = TRUE;

            //
            // Do common dialog to save file
            //

            if (NPOpenSave(szNewName, MAX_PATH))
            {

               if( SaveFile(hwnd, szNewName, TRUE) )
               {
                  lstrcpyn( szFileName, szNewName, MAX_PATH);
                  g_ftOpenedAs= g_ftSaveAs;
               }
            }
            else
            {
                SignalCommDlgError();
            }

            fInSaveAsDlg = FALSE;
            break;

        case M_SELECTALL:
            {
                HMENU    hMenu;

                hMenu = GetMenu(hwndNP);
                lSel = (LONG) SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, 0L);
                SendMessage (hwndEdit, EM_SETSEL, 0, lSel );
                SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
                EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_GRAYED);
                break;
            }

        case M_REPLACE:
			CopyEditControlSelectionText(hwndEdit, szSearch, ARRAYSIZE(szSearch));

            if (hDlgFind)
            {
				SetDlgItemText(hDlgFind, edt1, szSearch);
				SetFocus(hDlgFind);
            }
            else
            {
				FR.Flags			= FR_REPLACE | FR_ENABLETEMPLATE | FR_ENABLEHOOK;
				FR.lpTemplateName	= MAKEINTRESOURCE(IDD_REPLACEDIALOG);
				FR.lpfnHook			= FindReplaceHookProc;
				FR.lpstrReplaceWith	= szReplace;
				FR.wReplaceWithLen	= CCHKEYMAX;
				FR.lpstrFindWhat	= szSearch;
				FR.wFindWhatLen		= CCHKEYMAX;
				hDlgFind = ReplaceText( &FR );
            }
            break;

        case M_FINDNEXT:
            if (szSearch[0])
            {
				fReverse = FALSE;
				Search(szSearch);
				break;
            }
            /* else fall through and bring up "find" dialog */

		case M_FINDPREVIOUS:
			// notepadEx addition
			if (szSearch[0])
			{
				fReverse = TRUE;
				Search(szSearch);
				break;
			}

        case M_FIND:
			CopyEditControlSelectionText(hwndEdit, szSearch, ARRAYSIZE(szSearch));

            if (hDlgFind)
            {
				SetDlgItemText(hDlgFind, edt1, szSearch);
				SetFocus(hDlgFind);
            }
            else
            {
				FR.Flags			= FR_DOWN | FR_ENABLETEMPLATE | FR_ENABLEHOOK;
				FR.lpTemplateName	= MAKEINTRESOURCE(IDD_FINDDIALOG);
				FR.lpfnHook			= FindReplaceHookProc;
				FR.lpstrReplaceWith	= NULL;
				FR.wReplaceWithLen	= 0;
				FR.lpstrFindWhat	= szSearch;
				FR.wFindWhatLen		= CCHKEYMAX;
				hDlgFind = FindText((LPFINDREPLACE)&FR);
            }
            break;

        case M_GOTO:
            DialogBox(	(HINSTANCE) hInstanceNP,
						MAKEINTRESOURCE(IDD_GOTODIALOG),
						hwndNP,
						GotoDlgProc);
            break;

        case M_ABOUT:
            ShellAbout(hwndNP,
                       szNN,
                       TEXT(""),
                       LoadIcon((HINSTANCE) hInstanceNP,
                                (LPTSTR)MAKEINTRESOURCE(ID_ICON)));

            break;

        case M_HELP:
			CLSIDFromString(L"{8cec58e7-07a1-11d9-b15e-000d56bfe6ee}", &CLSID_HxHelpPane);
			IIDFromString(L"{8cec5884-07a1-11d9-b15e-000d56bfe6ee}", &IID_IHxHelpPane);

			// NotepadEx
			if (SUCCEEDED(CoCreateInstance(&CLSID_HxHelpPane, NULL, CLSCTX_INPROC_SERVER, &IID_IHxHelpPane, &php))) {
				// Vista/7+ windows help
				if (!(helpURI = SysAllocString(L"mshelp://Windows/?id=5d18d5fb-e737-4a73-b6cc-dccc63720231"))) break;
				php->lpVtbl->DisplayTask(php, helpURI);
				php->lpVtbl->Release(php); php = NULL;
				SysFreeString(helpURI); helpURI = NULL;
			} else {
				// NT5 style - on XP and 2k, the required CHM file is located at %WINDIR%\Help\notepad.chm
				//HtmlHelpA(GetDesktopWindow(), "notepad.chm", HH_DISPLAY_TOPIC, 0L);
				TCHAR szNpHelpPath[MAX_PATH];
				ExpandEnvironmentStrings(TEXT("%WINDIR%\\Help\\notepad.chm"), szNpHelpPath, ARRAYSIZE(szNpHelpPath));
				ShellExecute(GetDesktopWindow(), NULL, szNpHelpPath, NULL, NULL, TRUE);
			}
            break;

        case M_CUT:
        case M_COPY:
        case M_CLEAR:
            {
                DWORD dwSelStart;
                DWORD dwSelEnd;

                // if nothing selected, then nothing to do.

                SendMessage( hwndEdit, 
                             EM_GETSEL, 
                             (WPARAM) &dwSelStart, 
                             (LPARAM) &dwSelEnd);
                if( dwSelStart == dwSelEnd ) {
                   break;
                }
            }

        case M_PASTE:
            /* If notepad parent or edit window has the focus,
               pass command to edit window.
               make sure line resulting from paste will not be too long. */
            hwndFocus = GetFocus();
            if (hwndFocus == hwndEdit || hwndFocus == hwndNP)
            {
                PostMessage(hwndEdit, LOWORD(wParam), 0, 0);
            }
            break;

        case M_DATETIME:
            InsertDateTime(FALSE);
            break;

        case M_UNDO:
            SendMessage (hwndEdit, EM_UNDO, 0, 0L);
            break;

        case M_WW: // word wrap
            style= (!fWrap) ? ES_STD : (ES_STD | WS_HSCROLL);
            if( NpReCreate( style ) )
            {
                fWrap= !fWrap;
            }
            else
            {
                MessageBox(hwndNP, szNoWW, szNN,
                           MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            }

            break;

        case M_STATUSBAR:

            // hide/show the statusbar and also redraw the edit window accordingly.
            GetClientRect(hwndNP, &rcClient);

            if ( fStatus )
            {
                fStatus = FALSE;
                ShowWindow ( hwndStatus, SW_HIDE );
                NPSize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
            }
            else
            {
                fStatus = TRUE;
                NPSize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
                UpdateStatusBar(TRUE);
                ShowWindow( hwndStatus, SW_SHOW );
            }
            break;

        case ID_EDIT:
			// 18-08-2022 notepadEx - add a handler here which updates the titlebar status
			if (g_bDirty == FALSE && HIWORD(wParam) == EN_CHANGE && SendMessage(hwndEdit, EM_GETMODIFY, 0, 0)) {
				TCHAR lpNewTitle[MAX_PATH + 11];

				if (GetWindowText(hwndNP, lpNewTitle + 1, ARRAYSIZE(lpNewTitle))) {
					lpNewTitle[0] = '*';
					SetWindowText(hwndNP, lpNewTitle);
					g_bDirty = TRUE;
				}
			}
            break;

        case M_PRINT:
            PrintIt( UseDialog );
            break;

        case M_PAGESETUP:
            TryPrintDlgAgain:
            
            if( PageSetupDlg(&g_PageSetupDlg) )
            {
                //  We know it's okay to copy these strings over...
                lstrcpy(chPageText[HEADER], chPageTextTemp[HEADER]);
                lstrcpy(chPageText[FOOTER], chPageTextTemp[FOOTER]);
            }
            else
            {
                rc= CommDlgExtendedError();

                if( rc == PDERR_PRINTERNOTFOUND ||
                    rc == PDERR_DNDMMISMATCH    ||
                    rc == PDERR_DEFAULTDIFFERENT )
                  {
                      FreeGlobal();
                      g_PageSetupDlg.hDevMode= g_PageSetupDlg.hDevNames= 0;
                      goto TryPrintDlgAgain;
                  }

                // Check for Dialog Failure

                SignalCommDlgError( );

            }
            break;

        case M_SETFONT:
        {
            CHOOSEFONT  cf;
            HFONT       hFontNew;
            HDC         hDisplayDC;     // display DC

            hDisplayDC= GetDC(NULL);    // try to get display DC
            if( !hDisplayDC )
                break;

            // calls the font chooser (in commdlg)
            // We set lfHeight; choosefont returns ipointsize
            //

            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &FontStruct;         // filled in by init
            FontStruct.lfHeight= -MulDiv(iPointSize,GetDeviceCaps(hDisplayDC,LOGPIXELSY),720);
 
            // We filter out useless stuff here
            // We tried CF_NOSCRIPTSEL, but the FE had fits.
            //    Even though it looks useless, it changes the font that gets mapped on FE builds.
            //    Even though we ignore the lfCharSet that gets returned, we have the "right"
            //    font according to the FE guys.  It might make sense to use lfCharSet to
            //    convert the ansi file when it is converted to Unicode, but this might be
            //    confusing.
 
            cf.Flags = CF_INITTOLOGFONTSTRUCT |
                       CF_SCREENFONTS         | 
                       CF_NOVERTFONTS         |
                       // CF_NOSCRIPTSEL         |  // windows bug# 7770 (April 10,2001)
                       0;
            cf.rgbColors = 0;                   // only if cf_effects
            cf.lCustData = 0;                   // for hook function
            cf.lpfnHook = (LPCFHOOKPROC) NULL;
            cf.lpTemplateName = (LPTSTR) NULL;
            cf.hInstance = NULL;
            cf.lpszStyle = NULL;                // iff cf_usestyle
            cf.nFontType = SCREEN_FONTTYPE;
            cf.nSizeMin  = 0;  // iff cf_limitsize
            cf.nSizeMax  = 0;  // iff cf_limitsize
            ReleaseDC( NULL, hDisplayDC );

            if( ChooseFont(&cf) )
            {
                SetCursor( hWaitCursor );        // may take some time
                
                hFontNew= CreateFontIndirect(&FontStruct);
                if( hFontNew )
                {
                   DeleteObject( hFont );
                   hFont= hFontNew;
                   SendMessage( hwndEdit, WM_SETFONT,
                               (WPARAM)hFont, MAKELPARAM(TRUE, 0));
                   iPointSize= cf.iPointSize;  // remember for printer
                }
                SetCursor( hStdCursor );
            }
            break;
        }
		
		case M_TW2:
		{
			iTabStops = 8;
			SendMessage(hwndEdit, EM_SETTABSTOPS, 1, (LPARAM) &iTabStops);
			break;
		}

		case M_TW4:
		{
			iTabStops = 16;
			SendMessage(hwndEdit, EM_SETTABSTOPS, 1, (LPARAM) &iTabStops);
			break;
		}
		
		case M_TW8:
		{
			iTabStops = 32;
			SendMessage(hwndEdit, EM_SETTABSTOPS, 1, (LPARAM) &iTabStops);
			break;
		}

        default:
            return FALSE;
    }
    return TRUE;
}


// for some reason, this procedure tries to maintain
// a valid 'fp' even though I believe it does not need
// to be.
void FileDragOpen(void)
{
    HANDLE oldfp;

    oldfp= fp;       // remember in case of error

    if( CheckSave(FALSE) )
    {

         fp= CreateFile( szPath,               // filename
                         GENERIC_READ,         // access mode
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL,                 // security descriptor
                         OPEN_EXISTING,        // how to create
                         FILE_ATTRIBUTE_NORMAL,// file attributes
                         NULL);                // hnd to file attrs

       if( fp == INVALID_HANDLE_VALUE )
       {
          AlertUser_FileFail( szPath );

          // Restore fp to original file.
          fp= oldfp;
       }
       /* Try to load the file and reset fp if failed */
       else if( !LoadFile( szPath, FT_UNKNOWN ) )
       {
           fp= oldfp;
       }
    }
}


/* Proccess file drop/drag options. */
void doDrop (WPARAM wParam, HWND hwnd)
{
   /* If user dragged/dropped a file regardless of keys pressed
    * at the time, open the first selected file from file manager. */

    if (DragQueryFile ((HANDLE)wParam, 0xFFFFFFFF, NULL, 0)) /* # of files dropped */
    {
       DragQueryFile ((HANDLE)wParam, 0, szPath, CharSizeOf(szPath));
       SetActiveWindow (hwnd);
       FileDragOpen();
    }
    DragFinish ((HANDLE)wParam);  /* Delete structure alocated for WM_DROPFILES*/
}

typedef HRESULT (WINAPI *TDI)(const TASKDIALOGCONFIG *, int *, int *, BOOL *);

/* ** if notepad is dirty, check to see if user wants to save contents */
BOOL FAR CheckSave (BOOL fSysModal)
{
    INT    mdResult = IDOK;
    TCHAR  szNewName[MAX_PATH] = TEXT("");      /* New file name */
    TCHAR *pszFileName;
	INT iAllocSize;                   // size needed for message
    TCHAR*  pszMessage;               // combined message
	TDI __TaskDialogIndirect;
	TASKDIALOGCONFIG tdc;
	
	TASKDIALOG_BUTTON tdButtons[] = {
		{IDYES, TEXT("&Save")},
		{IDNO, TEXT("Do&n't Save")},
	};
	ZeroMemory(&tdc, sizeof(tdc));

/* If it's untitled and there's no text, don't worry about it */
    if (fUntitled && !SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0))
        return (TRUE);

    if (SendMessage (hwndEdit, EM_GETMODIFY, 0, 0L))
    {
       if( fUntitled )
           pszFileName= szUntitled;
       else
           pszFileName= szFileName;

       // put up message box - XP style or Vista style depending on whether it is available
	   if ((__TaskDialogIndirect = (TDI) GetProcAddress(GetModuleHandleA("COMCTL32.DLL"), "TaskDialogIndirect")) == NULL) {
		   // use XP style MessageBox
		   fInSaveAsDlg= TRUE;     // inform wm_queryendsession that we are trying to save
		   mdResult= AlertBox(hwndNP, szNN, szSCBC, pszFileName,
                              (WORD)((fSysModal ? MB_SYSTEMMODAL :
                              MB_APPLMODAL)|MB_YESNOCANCEL|MB_ICONEXCLAMATION));
	   } else {
		   // use Task Dialog like in Vista and up
		   iAllocSize= (lstrlen(szSCBC) + (pszFileName ? lstrlen(pszFileName) : 0) + 1 ) * sizeof(TCHAR);
		   pszMessage= (TCHAR*) LocalAlloc( LPTR, iAllocSize );

	       if( pszMessage ) {
		       MergeStrings( szSCBC, pszFileName, pszMessage );
           } else {
		       pszMessage = szSCBC;
           }
	       tdc.cbSize = sizeof(tdc);
	       tdc.hwndParent = hwndNP;
	       tdc.hInstance = (HINSTANCE) hInstanceNP;
	       tdc.pszWindowTitle = szNN;
	       tdc.pszMainInstruction = pszMessage;
	       tdc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	       tdc.cButtons = 2;
	       tdc.pButtons = tdButtons;
	       __TaskDialogIndirect(&tdc, &mdResult, NULL, NULL);
	       if (pszMessage != szSCBC) LocalFree( (HLOCAL) pszMessage );
	   }

       fInSaveAsDlg= FALSE;

       if( mdResult == IDYES )
       {
          if( fUntitled )
          {
             lstrcpy( szNewName, TEXT("*.txt") );
SaveFilePrompt:
             OFN.lpstrFile        = szNewName;
             OFN.lpstrTitle       = szSaveCaption;

            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             * 12 February 1991    clarkc
             */

            OFN.Flags = OFN_HIDEREADONLY     | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST   |
                        OFN_EXPLORER         |
                        OFN_ENABLESIZING     |
                        OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

            OFN.lpTemplateName= TEXT("NpEncodingDialog");
            OFN.lpfnHook= NpSaveDialogHookProc;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             * 12 February 1991    clarkc
             */
             OFN.lpstrFilter       = szSaveFilterSpec;
             OFN.lpstrDefExt       = TEXT("txt");

             OFN.nFilterIndex= FILE_TEXT;

             //
             // Set dialog checkmark by current file type
             //

             fInSaveAsDlg = TRUE;
             if (NPOpenSave(szNewName, MAX_PATH))
             {
                // since SaveFile() uses near ptr to name (obs.)
                lstrcpy(szNewName, OFN.lpstrFile);
                if( SaveFile(hwndNP, szNewName, TRUE) )
                {
                   lstrcpy(szFileName, szNewName);
                   g_ftOpenedAs= g_ftSaveAs;
                }
                else
                {      // Fixing close without saving file when disk-full
                   lstrcpy(szNewName, szFileName);
                   goto SaveFilePrompt;
                }
             }
             else
             {
                mdResult= IDCANCEL;       /* Don't exit Program */
                if( CommDlgExtendedError() )/* Dialog box failed, Lo-mem*/
                   DeepTrouble();
             }

             fInSaveAsDlg = FALSE;
          }
          else
          {
            // initialize the save type.
             g_ftSaveAs = g_ftOpenedAs;
             if( SaveFile(hwndNP, szFileName, FALSE) )
                return(TRUE);
             lstrcpy(szNewName, szFileName);
             goto SaveFilePrompt;
          }
       }
    }
    return (mdResult != IDCANCEL);
}

// new in notepadex
// ripped from some shell code somewhere from the edit control
// forgot exactly where
BOOL IsBreakChar(
	WCHAR	wch)
{
	CONST WCHAR szBreakChars[] = {
		0x0009, // TAB
		0x000A, // NEWLINE
		0x000D, // CARRIAGE RETURN
		0x0020, // SPACE
		0x0021, // IE: !
		0x0022, // IE: "
		0x0023, // IE: #
		0x0024, // IE: $
		0x0025, // IE: %
		0x0026, // IE: &
		0x0027, // IE: '
		0x0028, // LEFT PARENTHESIS
		0x0029, // RIGHT PARENTHESIS
		0x002A, // IE: *
		0x002B, // IE: +
		0x002C, // IE: ,
		0x002D, // HYPHEN
		0x002E, // IE: .
		0x002F, // IE: /
		0x003A, // IE: :
		0x003B, // IE: ;
		0x003C, // IE: <
		0x003D, // IE: =
		0x003E, // IE: >
		0x003F, // IE: ?
		0x0040, // IE: @
		0x005B, // LEFT SQUARE BRACKET
		0x005C, // IE: '\'
		0x005D, // RIGHT SQUARE BRACKET
		0x005E, // IE: ^
		0x005F, // IE: _
		0x0060, // IE:`
		0x007B, // LEFT CURLY BRACKET
		0x007C, // IE: |
		0x007D, // RIGHT CURLY BRACKET
		0x007E, // IE: ~
		0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		0x00AD, // OPTIONAL HYPHEN
		0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		0x02C7, // CARON
		0x02C9, // MODIFIER LETTER MACRON
		0x055D, // ARMENIAN COMMA
		0x060C, // ARABIC COMMA
		0x2002, // EN SPACE
		0x2003, // EM SPACE
		0x2004, // THREE-PER-EM SPACE
		0x2005, // FOUR-PER-EM SPACE
		0x2006, // SIX-PER-EM SPACE
		0x2007, // FIGURE SPACE
		0x2008, // PUNCTUATION SPACE
		0x2009, // THIN SPACE
		0x200A, // HAIR SPACE
		0x200B, // ZERO WIDTH SPACE
		0x2013, // EN DASH
		0x2014, // EM DASH
		0x2016, // DOUBLE VERTICAL LINE
		0x2018, // LEFT SINGLE QUOTATION MARK
		0x201C, // LEFT DOUBLE QUOTATION MARK
		0x201D, // RIGHT DOUBLE QUOTATION MARK
		0x2022, // BULLET
		0x2025, // TWO DOT LEADER
		0x2026, // HORIZONTAL ELLIPSIS
		0x2027, // HYPHENATION POINT
		0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
		0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
		0x2045, // LEFT SQUARE BRACKET WITH QUILL
		0x2046, // RIGHT SQUARE BRACKET WITH QUILL
		0x207D, // SUPERSCRIPT LEFT PARENTHESIS
		0x207E, // SUPERSCRIPT RIGHT PARENTHESIS
		0x208D, // SUBSCRIPT LEFT PARENTHESIS
		0x208E, // SUBSCRIPT RIGHT PARENTHESIS
		0x226A, // MUCH LESS THAN
		0x226B, // MUCH GREATER THAN
		0x2574, // BOX DRAWINGS LIGHT LEFT
		0x3001, // IDEOGRAPHIC COMMA
		0x3002, // IDEOGRAPHIC FULL STOP
		0x3003, // DITTO MARK
		0x3005, // IDEOGRAPHIC ITERATION MARK
		0x3008, // LEFT ANGLE BRACKET
		0x3009, // RIGHT ANGLE BRACKET
		0x300A, // LEFT DOUBLE ANGLE BRACKET
		0x300B, // RIGHT DOUBLE ANGLE BRACKET
		0x300C, // LEFT CORNER BRACKET
		0x300D, // RIGHT CORNER BRACKET
		0x300E, // LEFT WHITE CORNER BRACKET
		0x300F, // RIGHT WHITE CORNER BRACKET
		0x3010, // LEFT BLACK LENTICULAR BRACKET
		0x3011, // RIGHT BLACK LENTICULAR BRACKET
		0x3014, // LEFT TORTOISE SHELL BRACKET
		0x3015, // RIGHT TORTOISE SHELL BRACKET
		0x3016, // LEFT WHITE LENTICULAR BRACKET
		0x3017, // RIGHT WHITE LENTICULAR BRACKET
		0x3018, // LEFT WHITE TORTOISE SHELL BRACKET
		0x3019, // RIGHT WHITE TORTOISE SHELL BRACKET
		0x301A, // LEFT WHITE SQUARE BRACKET
		0x301B, // RIGHT WHITE SQUARE BRACKET
		0x301D, // REVERSED DOUBLE PRIME QUOTATION MARK
		0x301E, // DOUBLE PRIME QUOTATION MARK
		0x3041, // HIRAGANA LETTER SMALL A
		0x3043, // HIRAGANA LETTER SMALL I
		0x3045, // HIRAGANA LETTER SMALL U
		0x3047, // HIRAGANA LETTER SMALL E
		0x3049, // HIRAGANA LETTER SMALL O
		0x3063, // HIRAGANA LETTER SMALL TU
		0x3083, // HIRAGANA LETTER SMALL YA
		0x3085, // HIRAGANA LETTER SMALL YU
		0x3087, // HIRAGANA LETTER SMALL YO
		0x308E, // HIRAGANA LETTER SMALL WA
		0x309B, // KATAKANA-HIRAGANA VOICED SOUND MARK
		0x309C, // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
		0x309D, // HIRAGANA ITERATION MARK
		0x309E, // HIRAGANA VOICED ITERATION MARK
		0x30A1, // KATAKANA LETTER SMALL A
		0x30A3, // KATAKANA LETTER SMALL I
		0x30A5, // KATAKANA LETTER SMALL U
		0x30A7, // KATAKANA LETTER SMALL E
		0x30A9, // KATAKANA LETTER SMALL O
		0x30C3, // KATAKANA LETTER SMALL TU
		0x30E3, // KATAKANA LETTER SMALL YA
		0x30E5, // KATAKANA LETTER SMALL YU
		0x30E7, // KATAKANA LETTER SMALL YO
		0x30EE, // KATAKANA LETTER SMALL WA
		0x30F5, // KATAKANA LETTER SMALL KA
		0x30F6, // KATAKANA LETTER SMALL KE
		0x30FC, // KATAKANA-HIRAGANA PROLONGED SOUND MARK
		0x30FD, // KATAKANA ITERATION MARK
		0x30FE, // KATAKANA VOICED ITERATION MARK
		0xFD3E, // ORNATE LEFT PARENTHESIS
		0xFD3F, // ORNATE RIGHT PARENTHESIS
		0xFE30, // VERTICAL TWO DOT LEADER
		0xFE31, // VERTICAL EM DASH
		0xFE33, // VERTICAL LOW LINE
		0xFE34, // VERTICAL WAVY LOW LINE
		0xFE35, // PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS
		0xFE36, // PRESENTATION FORM FOR VERTICAL RIGHT PARENTHESIS
		0xFE37, // PRESENTATION FORM FOR VERTICAL LEFT CURLY BRACKET
		0xFE38, // PRESENTATION FORM FOR VERTICAL RIGHT CURLY BRACKET
		0xFE39, // PRESENTATION FORM FOR VERTICAL LEFT TORTOISE SHELL BRACKET
		0xFE3A, // PRESENTATION FORM FOR VERTICAL RIGHT TORTOISE SHELL BRACKET
		0xFE3B, // PRESENTATION FORM FOR VERTICAL LEFT BLACK LENTICULAR BRACKET
		0xFE3C, // PRESENTATION FORM FOR VERTICAL RIGHT BLACK LENTICULAR BRACKET
		0xFE3D, // PRESENTATION FORM FOR VERTICAL LEFT DOUBLE ANGLE BRACKET
		0xFE3E, // PRESENTATION FORM FOR VERTICAL RIGHT DOUBLE ANGLE BRACKET
		0xFE3F, // PRESENTATION FORM FOR VERTICAL LEFT ANGLE BRACKET
		0xFE40, // PRESENTATION FORM FOR VERTICAL RIGHT ANGLE BRACKET
		0xFE41, // PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET
		0xFE42, // PRESENTATION FORM FOR VERTICAL RIGHT CORNER BRACKET
		0xFE43, // PRESENTATION FORM FOR VERTICAL LEFT WHITE CORNER BRACKET
		0xFE44, // PRESENTATION FORM FOR VERTICAL RIGHT WHITE CORNER BRACKET
		0xFE4F, // WAVY LOW LINE
		0xFE50, // SMALL COMMA
		0xFE51, // SMALL IDEOGRAPHIC COMMA
		0xFE59, // SMALL LEFT PARENTHESIS
		0xFE5A, // SMALL RIGHT PARENTHESIS
		0xFE5B, // SMALL LEFT CURLY BRACKET
		0xFE5C, // SMALL RIGHT CURLY BRACKET
		0xFE5D, // SMALL LEFT TORTOISE SHELL BRACKET
		0xFE5E, // SMALL RIGHT TORTOISE SHELL BRACKET
		0xFF08, // FULLWIDTH LEFT PARENTHESIS
		0xFF09, // FULLWIDTH RIGHT PARENTHESIS
		0xFF0C, // FULLWIDTH COMMA
		0xFF0E, // FULLWIDTH FULL STOP
		0xFF1C, // FULLWIDTH LESS-THAN SIGN
		0xFF1E, // FULLWIDTH GREATER-THAN SIGN
		0xFF3B, // FULLWIDTH LEFT SQUARE BRACKET
		0xFF3D, // FULLWIDTH RIGHT SQUARE BRACKET
		0xFF40, // FULLWIDTH GRAVE ACCENT
		0xFF5B, // FULLWIDTH LEFT CURLY BRACKET
		0xFF5C, // FULLWIDTH VERTICAL LINE
		0xFF5D, // FULLWIDTH RIGHT CURLY BRACKET
		0xFF5E, // FULLWIDTH TILDE
		0xFF61, // HALFWIDTH IDEOGRAPHIC FULL STOP
		0xFF62, // HALFWIDTH LEFT CORNER BRACKET
		0xFF63, // HALFWIDTH RIGHT CORNER BRACKET
		0xFF64, // HALFWIDTH IDEOGRAPHIC COMMA
		0xFF67, // HALFWIDTH KATAKANA LETTER SMALL A
		0xFF68, // HALFWIDTH KATAKANA LETTER SMALL I
		0xFF69, // HALFWIDTH KATAKANA LETTER SMALL U
		0xFF6A, // HALFWIDTH KATAKANA LETTER SMALL E
		0xFF6B, // HALFWIDTH KATAKANA LETTER SMALL O
		0xFF6C, // HALFWIDTH KATAKANA LETTER SMALL YA
		0xFF6D, // HALFWIDTH KATAKANA LETTER SMALL YU
		0xFF6E, // HALFWIDTH KATAKANA LETTER SMALL YO
		0xFF6F, // HALFWIDTH KATAKANA LETTER SMALL TU
		0xFF70, // HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK
		0xFF9E, // HALFWIDTH KATAKANA VOICED SOUND MARK
		0xFF9F, // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK
		0xFFE9, // HALFWIDTH LEFTWARDS ARROW
		0xFFEB, // HALFWIDTH RIGHTWARDS ARROW
	};

	INT iMin = 0;
	INT iMax = ARRAYSIZE(szBreakChars) - 1;

	while (iMax - iMin >= 2) {
		INT iTry = (iMax + iMin + 1) / 2;

		if (wch < szBreakChars[iTry]) {
			iMax = iTry;
		} else if (wch > szBreakChars[iTry]) {
			iMin = iTry;
		} else {
			return TRUE;
		}
	}

	return (wch == szBreakChars[iMin] || wch == szBreakChars[iMax]);
}

//#define ISDELIMITERW(ch) (((ch) == L' ') || ((ch) == L'\t'))

// CharPrev is slow as fuck bloatware. We don't need it.
#undef CharPrev
#define CharPrev(lpszStart, lpszCurrent) max((lpszStart), (lpszCurrent) - 1)

// This function is new in NotepadEx, ripped from some shell code
// for the edit control.
// This function is not actually set as the edit control's word
// break procedure. It is used only for Ctrl+Backspace support.
INT CALLBACK EditWordBreakProc(
	LPWSTR	lpszEditText,
	INT		ichCurrent,
	INT		cch,
	INT		code)
{
	LPWSTR lpsz = lpszEditText + ichCurrent;
	INT iIndex = ichCurrent;
	BOOL bFoundNonDelimiter = FALSE;
	
	switch (code) {
	case WB_LEFT:
		while ((lpsz = CharPrev(lpszEditText, lpsz)) != lpszEditText) {
			if (IsBreakChar(*lpsz)) {
				if (bFoundNonDelimiter) {
					break;
				}
			} else {
				bFoundNonDelimiter = TRUE;
			}
		}

		iIndex = (INT) (lpsz - lpszEditText);

		if (iIndex > 0 && iIndex < cch) {
			iIndex++;
		}

		break;
	}

	return iIndex;
}

LRESULT CALLBACK EditWndProc(
		HWND		hwnd,
		UINT		message,
		WPARAM		wParam,
		LPARAM		lParam)
{
	// NotepadEx change from some time ago, that I forgot to comment:
	// Support Ctrl+Backspace to delete a word.
	if (message == WM_CHAR && wParam == VK_F16) {
		HLOCAL hBuf;
		LPWSTR lpszBuf;
		DWORD dwCurrentSelStart;
		DWORD dwNewSelStart;

		SendMessage(hwnd, EM_GETSEL, (WPARAM) &dwCurrentSelStart, 0);
		hBuf = (HLOCAL) SendMessage(hwnd, EM_GETHANDLE, 0, 0);

		if (!hBuf) {
			goto Error;
		}

		lpszBuf = (LPWSTR) LocalLock(hBuf);
		dwNewSelStart = EditWordBreakProc(
			lpszBuf,
			dwCurrentSelStart,
			GetWindowTextLength(hwnd),
			WB_LEFT);
		LocalUnlock(hBuf);

		// The WM_SETREDRAW messages are necessary to avoid briefly flashing
		// the selection on the edit control.
		SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
		SendMessage(hwnd, EM_SETSEL, dwNewSelStart, dwCurrentSelStart);
		SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM) L"");
		SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
			
		return 0;
	}

Error:
	return CallWindowProc(DefEditWindowProc, hwnd, message, wParam, lParam);
}

/* Notepad window class procedure */
WNDPROC FAR NPWndProc(
        HWND       hwnd,
        UINT       message,
        WPARAM     wParam,
        LPARAM     lParam)
{
    LPFINDREPLACE lpfr;
    DWORD dwFlags;
    INT iParts[3];


    switch (message)
    {
/* If we're being run by Setup and it's the system menu, be certain that
 * the minimize menu item is disabled.  Note that hSysMenuSetup is only
 * initialized if Notepad is being run by Setup.  Don't use it outside
 * the fRunBySetup conditional!    28 June 1991    Clark Cyr
 */
        case WM_INITMENUPOPUP:
            if (fRunBySetup && HIWORD(lParam))
               EnableMenuItem(hSysMenuSetup,SC_MINIMIZE,MF_GRAYED|MF_DISABLED);
            break;

        case WM_SYSCOMMAND:
            if (fRunBySetup)
            {
                /* If we have been spawned by SlipUp we need to make sure the
                 * user doesn't minimize us or alt tab/esc away.
                 */
                if (wParam == SC_MINIMIZE ||
                    wParam == SC_NEXTWINDOW ||
                    wParam == SC_PREVWINDOW)
                    break;
            }
            DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_SETFOCUS:
            if (!IsIconic(hwndNP))
            {
               SetFocus(hwndEdit);
            }
            break;

        case WM_KILLFOCUS:
            SendMessage (hwndEdit, message, wParam, lParam);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:

            // Save any globals in the registry if need be

            SaveGlobals();

            if (CheckSave(FALSE))
            {
                /* Exit help */
                if(!WinHelp(hwndNP, (LPTSTR)szHelpFile, HELP_QUIT, 0))
                    DeepTrouble();

                DestroyWindow(hwndStatus);
                DestroyWindow(hwndNP);
                DeleteObject(hFont);
            }

            break;

        case WM_QUERYENDSESSION:
            if (fInSaveAsDlg)
            {
                MessageBeep (0);
                MessageBeep (0);
                MessageBox (hwndNP, szCannotQuit, szNN, MB_OK|MB_SYSTEMMODAL);
                return FALSE;
            }
            else
                return (WNDPROC) (CheckSave(TRUE));
            break;


        case WM_ACTIVATEAPP:
            if (wParam)
            {
            /* This causes the caret position to be at the end of the selection
             * but there's no way to ask where it was or set it if known.  This
             * will cause a caret change when the selection is made from bottom
             * to top.
             */
                if( dwCurrentSelectionStart != 0 || dwCurrentSelectionEnd != 0 )
                {
                   SendMessage( hwndEdit, EM_SETSEL,
                                dwCurrentSelectionStart,
                                dwCurrentSelectionEnd );
                   SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
                }
            }
            else
            {
                SendMessage( hwndEdit, EM_GETSEL,
                            (WPARAM) &dwCurrentSelectionStart,
                            (LPARAM) &dwCurrentSelectionEnd);
                if (dwCurrentSelectionStart == dwCurrentSelectionEnd)
                {
                    dwCurrentSelectionStart = 0L;
                    dwCurrentSelectionEnd = 0L;
                }
                else
                {
                   SendMessage (hwndEdit, EM_SETSEL, dwCurrentSelectionStart,
                                dwCurrentSelectionEnd);
                   SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0);
                }
            }
            break;

        case WM_ACTIVATE:
            if ((LOWORD(wParam) == WA_ACTIVE       ||
                 LOWORD(wParam) == WA_CLICKACTIVE) &&
                !IsIconic(hwndNP)
               )
               {
                   // active doesn't always mean foreground (ntbug# 53048)
                   if( GetForegroundWindow() == hwndNP )
                   {
                       SetFocus(GetForegroundWindow());
                   }
               }
            break;

        case WM_SIZE:
            switch (wParam)
            {
                case SIZENORMAL:
                case SIZEFULLSCREEN:
                    // resize the status window.
                    SendMessage(hwndStatus, WM_SIZE, 0, 0L);

                    // Divide the status window into three parts
					iParts[0] = 2 * (MAKEPOINTS(lParam).x)/4;
					iParts[1] = 3 * (MAKEPOINTS(lParam).x)/4;
                    iParts[2] = -1;
                    SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) sizeof(iParts)/sizeof(INT), (LPARAM) &iParts[0]);

                    NPSize(MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
                    break;

                case SIZEICONIC:
                    return (WNDPROC) (DefWindowProc(hwnd, message, wParam, lParam));
                }
            break;

        case WM_INITMENU:
            NpResetMenu( hwnd );
            break;

        //
        // Some keyboards come with a "Search" button which the shell team
        // wanted us to handle.  See ntbug# 380067
        //

        case WM_APPCOMMAND:

            if( ( GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_SEARCH ) ) 
            {
                NPCommand(hwnd, M_FIND, 0);
                break;
            }
            // otherwise fall through
 
        case WM_COMMAND:

            if ((HWND)(lParam) == hwndEdit &&
                (HIWORD(wParam) == EN_ERRSPACE ||
                 HIWORD(wParam) == EN_MAXTEXT))
            {
                if (dwEmSetHandle == SETHANDLEINPROGRESS)
                    dwEmSetHandle = SETHANDLEFAILED;
                else
                    DeepTrouble();
                return 0L;
            }

            if (!NPCommand(hwnd, wParam, lParam))
               return (WNDPROC) (DefWindowProc(hwnd, message, wParam, lParam));
            break;

        case WM_DROPFILES: /*case added 03/26/91 for file drag/drop support*/
            doDrop (wParam,hwnd);
            break;

        case PWM_CHECK_HKL: /* private message: corresponding to HKL change message */
            {
                LPARAM lParam = 0;

                if (PRIMARYLANGID(LOWORD((DWORD) (INT_PTR) GetKeyboardLayout(0))) == LANG_JAPANESE) {
                    /*
                     * If new current HKL is Japanese, handle the result string at once.
                     */
                    lParam = EIMES_GETCOMPSTRATONCE;
                }
                SendMessage(hwndEdit, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, lParam);
            }
            break;

        default:
            /* this can be a message from the modeless Find Text window */
            if (message == wFRMsg)
            {
                BOOL bStatus;    // true if found text

                lpfr = (LPFINDREPLACE)lParam;
                dwFlags = lpfr->Flags;

                fReverse	= (dwFlags & FR_DOWN      ? FALSE : TRUE);
				// NotepadEx: These are no longer required because we are setting
				// them directly in the find&replace hook procedure.
                //fCase		= (dwFlags & FR_MATCHCASE ? TRUE  : FALSE);
				//fWholeWord	= (dwFlags & FR_WHOLEWORD ? TRUE  : FALSE);
				//fWrapAround = (lpfr->lCustData & 1	  ? TRUE  : FALSE);

                if( dwFlags & FR_FINDNEXT )
                {
                    SetCursor( hWaitCursor );
                    Search( szSearch );
                    SetCursor( hStdCursor );
                }
                else if( dwFlags & FR_REPLACE )
                {
                    //
                    // Replace current selection if it matches
                    // then highlight the next occurence of the string.
                    //

                    SetCursor( hWaitCursor );
                    ReplaceSel( TRUE );
                    Search( szSearch );
                    SetCursor( hStdCursor );
                }
                else if( dwFlags & FR_REPLACEALL )
                {
                   //
                   // The replace dialog doesn't allow reverse searches
                   // but just it cases it changes, for it to false.
                   //
                   if( fReverse )
                   {
                       fReverse= FALSE;
                   }

                   //
                   // Replace all occurances of text in the file
                   // starting from the top.  Reset the selection
                   // to the top of the file.
                   //
                   SetCursor( hWaitCursor );
                   SendMessage( hwndEdit, EM_SETSEL, 0, 0 );
                   do
                   {
                      ReplaceSel( FALSE );
                      bStatus= Search( szSearch );
                   }
                   while( bStatus );
                   SetCursor( hStdCursor );
                   //
                   // back to the top of the file.
                   //
                   SendMessage( hwndEdit, EM_SETSEL, 0, 0 );
                   SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0);
                   UpdateStatusBar( TRUE ); // pass TRUE because it's more efficient considering we're at the top
				}
                else if (dwFlags & FR_DIALOGTERM)
                    hDlgFind = NULL;   /* invalidate modeless window handle */
                break;
            }
            return (WNDPROC) (DefWindowProc(hwnd, message, wParam, lParam));
    }
    return (0L);
}

LPTSTR SkipProgramName (LPTSTR lpCmdLine)
{
    LPTSTR  p = lpCmdLine;
    BOOL    bInQuotes = FALSE;

    //
    // Skip executable name
    //
    for (p; *p; p = CharNext(p))
    {
       if ((*p == TEXT(' ') || *p == TEXT('\t')) && !bInQuotes)
          break;

       if (*p == TEXT('\"'))
          bInQuotes = !bInQuotes;
    }

    while (*p == TEXT(' ') || *p == TEXT('\t'))
       p++;

    return (p);
}

/* ** Main loop */

VOID EntryPoint(
	VOID)
{
	ExitProcess(WinMain(GetModuleHandle(NULL), NULL, NULL, TRUE));
}

INT WINAPI WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR lpAnsiCmdLine,
   INT cmdShow)
{
    MSG msg;
    LPTSTR lpCmdLine = GetCommandLine ();
    HWINEVENTHOOK hEventHook = NULL;

	CoInitialize(NULL);

    if (!NPInit(hInstance, hPrevInstance, SkipProgramName (lpCmdLine), cmdShow))
    {
       return FALSE;
    }

    // set an event hook to get the cursor position! this event hook is used to update
    // the line & column position of the caret shown in the statusbar.
    hEventHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, NULL, WinEventFunc, 
                                (DWORD) GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
 
    while (GetMessage((LPMSG)&msg, (HWND)NULL, 0, 0))
    {
        //
        // To handle IME status when active KL is changed.
        //
        if (msg.message == WM_INPUTLANGCHANGEREQUEST) {
            //
            // WM_INPUTLANGCHANGE will be *sent* to WndProc,
            // so there's no chance to catch WM_INPUTLANGCHANGE from the frame window.
            // Instead, we post the private message to check the active HKL later.
            //
            PostMessage(hwndNP, PWM_CHECK_HKL, 0, 0);
        }

        if (!hDlgFind || !IsDialogMessage(hDlgFind, &msg))
        {
            if (TranslateAccelerator(hwndNP, hAccel, (LPMSG)&msg) == 0)
            {
               TranslateMessage ((LPMSG)&msg);
               DispatchMessage ((LPMSG)&msg);
            }
        }
    }

    /* Clean up any global allocations */
    FreeGlobal();
    LocalFree( hEdit );

    if (hEventHook) {
		UnhookWinEvent(hEventHook);
	}

	return (INT) (msg.wParam);
}

static DWORD iLastCol;
static DWORD iLastLine;
static DWORD iLastSelStart = 0; // NotepadEx
static DWORD SelRoot;
static NP_LINETYPE ltLastLineType;

// This function was added for NotepadEx.
// Given a zero-based character index, return:
//   - the 1-based real line number
//   - the 1-based real column number
VOID GetEditControlRealPosition(
	IN	HWND	Window,
	IN	ULONG	CharacterIndex,
	OUT	PUINT	LineNumber,
	OUT	PUINT	ColumnNumber)
{
	HLOCAL EditControlBufferHandle;
	PWSTR EditControlBuffer;
	ULONG Index;
	UINT LineNumberTemp;
	UINT ColumnNumberTemp;

	Index = 0;
	LineNumberTemp = 1;
	ColumnNumberTemp = 1;

	EditControlBufferHandle = Edit_GetHandle(Window);

	if (!EditControlBufferHandle) {
		return;
	}

	EditControlBuffer = (PWSTR) LocalLock(EditControlBufferHandle);

	if (!EditControlBuffer) {
		return;
	}

	while (TRUE) {
		if (EditControlBuffer[Index] == '\0') {
			break;
		}

		if (EditControlBuffer[Index] == '\n') {
			++LineNumberTemp;
			ColumnNumberTemp = 0;
		}

		if (Index == CharacterIndex) {
			break;
		}

		++Index;
		++ColumnNumberTemp;
	}

	LocalUnlock(EditControlBufferHandle);

	*LineNumber = LineNumberTemp;
	*ColumnNumber = ColumnNumberTemp;
}

VOID UpdateEditControlRealPosition(
	IN	HWND	Window,
	IN	ULONG	CharacterIndex,
	IN	ULONG	LastCharacterIndex,
	OUT	PUINT	LineNumber,
	OUT	PUINT	ColumnNumber)
{
	HLOCAL EditControlBufferHandle;
	PWSTR EditControlBuffer;
	ULONG Index;
	ULONG EndIndex;
	UINT LineNumberDelta;
	UINT ColumnNumberTemp;

	Index = min(CharacterIndex, LastCharacterIndex);
	EndIndex = max(CharacterIndex, LastCharacterIndex);
	LineNumberDelta = 0;
	ColumnNumberTemp = 0;

	EditControlBufferHandle = Edit_GetHandle(Window);

	if (!EditControlBufferHandle) {
		return;
	}

	EditControlBuffer = (PWSTR) LocalLock(EditControlBufferHandle);

	if (!EditControlBuffer) {
		return;
	}

	// calculate line number delta by counting the number of \n characters
	// between the current position and the previous position
	while (Index < EndIndex) {
		if (EditControlBuffer[Index] == '\n') {
			++LineNumberDelta;
		}

		++Index;
	}

	// calculate column number - how many chars forward is the last \n?
	Index = CharacterIndex;

	do {
		if (EditControlBuffer[Index] == '\n') {
			break;
		}

		++ColumnNumberTemp;
		--Index;
	} while (Index != -1);

	LocalUnlock(EditControlBufferHandle);

	*ColumnNumber = ColumnNumberTemp;
	
	if (CharacterIndex > LastCharacterIndex) {
		*LineNumber += LineNumberDelta;
	} else {
		*LineNumber -= LineNumberDelta;
	}
}

VOID UpdateStatusBar( BOOL fForceStatus )
{
    DWORD SelStart, SelEnd;
    UINT  iLine, iCol;
    TCHAR szStatusText[128];
	NP_LINETYPE ltLineType;

    // get the current caret position.
	SendMessage(hwndEdit, EM_GETSEL, (WPARAM) &SelStart, (WPARAM) &SelEnd);

	if (fWrap) {
		// NotepadEx change: Support calculating the real cursor position
		// when word wrap is enabled.

		if (fForceStatus) {
			// fully recalculate position
			GetEditControlRealPosition(hwndEdit, SelStart, &iLine, &iCol);
		} else {
			// partial recalculation based on last position
			iLine = iLastLine;
			iCol = iLastCol;
			UpdateEditControlRealPosition(hwndEdit, SelStart, iLastSelStart, &iLine, &iCol);
		}
	} else {
		// the line numbers are 1 based instead 0 based. hence add 1.
		iLine = (UINT) SendMessage( hwndEdit, EM_LINEFROMCHAR, SelStart, 0 ) + 1;
		iCol = SelStart - (UINT) SendMessage( hwndEdit, EM_LINEINDEX, iLine-1, 0 ) + 1;
	}

	// 18-08-2022 NotepadEx change
	// Try to get the actual position of the cursor instead of just the start of the
	// selection. The mechanism to do this "properly" was added in Windows 10 so that's
	// why we have to use this hacky method. But it works, and it works well.
	if (SelStart != SelEnd) {
		// there is actual text selected, not just a simple cursor.
		// we have to figure out the "direction" that the user selected text - whether
		// it is "upwards" or "downwards". If it's "upwards" we don't need to do anything.
		// Otherwise we need to set the line/column to the end of the selection.

		if (SelRoot < SelEnd) {
			iLine = (UINT) SendMessage(hwndEdit, EM_LINEFROMCHAR, SelEnd, 0) + 1;
			iCol = SelEnd - (UINT) SendMessage(hwndEdit, EM_LINEINDEX, iLine - 1, 0) + 1;
		}
	} else {
		SelRoot = SelStart;
	} // end of NotepadEx change

    // don't bother to update status if it hasn't changed
    if( fForceStatus || (iCol!=iLastCol) || (iLine!=iLastLine) )
    {
        // prepare and display the statusbar.
        // make sure you don't overflow the buffer boundary.
        _sntprintf(szStatusText, sizeof(szStatusText)/sizeof(TCHAR) -1, szLineCol, iLine, iCol);
        szStatusText[ sizeof(szStatusText)/sizeof(TCHAR) -1 ] = TEXT('\0');
        SetStatusBarText(szStatusText, 2);
    }

	ltLineType = g_ltOpenedAs;

	if (ltLineType != ltLastLineType) {
		switch (g_ltOpenedAs) {
		case LT_WINDOWS:	SetStatusBarText(szWindowsFile, 1); break;
		case LT_UNIX:		SetStatusBarText(szUnixFile, 1); break;
		}
	}

	ltLastLineType = ltLineType;
    iLastCol=  iCol;
    iLastLine= iLine;
	iLastSelStart = SelStart;
};

// WinEventFunc is called whenever the location of the caret changes
// in the edit window. The function updates the statusbar with the current
// line number, column of the caret.  This event is called when the mouse is moved.
// If the caret moves without mouse input, the UpdateStatusBar is called.
//
// UpdateStatusBar( TRUE ) is called to force the display being changed.
VOID CALLBACK WinEventFunc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject,
                      LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    UpdateStatusBar( FALSE );
}


#define MAX_UNTITLED_LENGTH 50   /* max chars of "untitled" */

/* ** Set Window caption text */
void FAR SetTitle( TCHAR  *sz )
{
    TCHAR    szWindowText[MAX_PATH+MAX_UNTITLED_LENGTH];
    TCHAR    szFileName[MAX_PATH];
    HANDLE   hFindFile;
    WIN32_FIND_DATA info;
    TCHAR    szStatusText[128] = TEXT("");
    DWORD    dwAttributes;

    // if "untitled" then don't do all this work...
    if( lstrcmp( sz, szUntitled ) == 0 )
    {
       // don't check return: if the string gets truncated, 
       // it will just be a short version of "Untitled" in some language.
       // Whoever translated "untitled" to something > 260 chars is strange
       StringCchCopy( szWindowText, 
                      sizeof(szWindowText)/sizeof(TCHAR), 
                      szUntitled );
    }
    else
    {
       // get the attributes for file. these will be shown
       // in the status bar.
       dwAttributes = GetFileAttributes(sz);

       // Get real(file system) name for the file.
       hFindFile= FindFirstFile( sz, &info );

       if( hFindFile != INVALID_HANDLE_VALUE )
       {
          // don't check return.  cFileName is MAX_PATH long
          StringCchCopy( szFileName, MAX_PATH, info.cFileName );
          FindClose( hFindFile );
       }
       else
       {
          // don't check return.  Truncation fine for UI.
          StringCchCopy( szFileName, MAX_PATH, sz );
       }

       GetFileTitle(szFileName, szWindowText, MAX_PATH);



       // Removed the left part of the status bar to show file properties.
       // The correct way to fix it is to make sure that the correct delimiters (like commas),
       // and correct order is used for various localized builds.
       // prepare the status bar text and show 
       // if the file has any special properties (such as hidden, readonly etc.)

        if (dwAttributes & FILE_ATTRIBUTE_COMPRESSED)
            if ((lstrlen(szStatusText) + lstrlen(szCompressedFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)
                lstrcpy(szStatusText, szCompressedFile);

        if (dwAttributes & FILE_ATTRIBUTE_ENCRYPTED)   
            if ((lstrlen(szStatusText) + lstrlen(szEncryptedFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szEncryptedFile);

        if (dwAttributes & FILE_ATTRIBUTE_HIDDEN)
            if ((lstrlen(szStatusText) + lstrlen(szHiddenFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szHiddenFile);

        if (dwAttributes & FILE_ATTRIBUTE_OFFLINE)
            if ((lstrlen(szStatusText) + lstrlen(szOfflineFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szOfflineFile);

        if (dwAttributes & FILE_ATTRIBUTE_READONLY)
        {
            if ((lstrlen(szStatusText) + lstrlen(szReadOnlyFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szReadOnlyFile);
        }

        if (dwAttributes & FILE_ATTRIBUTE_SYSTEM)
            if ((lstrlen(szStatusText) + lstrlen(szSystemFile) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)            
                lstrcat(szStatusText, szSystemFile);

        // if the status did get updated by file properties
        if (*szStatusText != TEXT('\0'))
        {
            // get rid of the last comma
            szStatusText[lstrlen(szStatusText)-1] = TEXT(' ');

            if ((lstrlen(szStatusText) + lstrlen(szFile)) < sizeof(szStatusText)/sizeof(TCHAR) - 1)           
                lstrcat(szStatusText, szFile);
        }

    }

    // set the line ending type displayed to the user upon opening a new
	// file
	switch (g_ltOpenedAs) {
	case LT_WINDOWS:	SetStatusBarText(szWindowsFile, 1);	break;
	case LT_UNIX:		SetStatusBarText(szUnixFile, 1);	break;
	}
	// Note: Commenting out the two lines below, and replacing with a call to UpdateStatusBar,
	// fixes a bug in all official Microsoft versions of Notepad, in which saving the file resets
	// the displayed cursor position (but not the actual cursor position) to 1,1 for some reason.
	// If this introduces a regression, the old behavior can be restored by uncommenting.
    //_sntprintf(szStatusText, sizeof(szStatusText)/sizeof(TCHAR) -1, szLineCol, 1, 1);
    //SetStatusBarText(szStatusText, 2);
	UpdateStatusBar(TRUE);

    lstrcat(szWindowText, szNpTitle);
    SetWindowText(hwndNP, (LPTSTR)szWindowText);
	g_bDirty = FALSE;
}

/* ** Given filename which may or maynot include path, return pointer to
      filename (not including path part.) */
LPTSTR PASCAL far PFileInPath(
    LPTSTR sz)
{
    LPTSTR pch = sz;
    LPTSTR psz;

    /* Strip path/drive specification from name if there is one */
    /* Ripped out AnsiPrev calls.     21 March 1991  clarkc     */
    for (psz = sz; *psz; psz = CharNext(psz))
      {
        if ((*psz == TEXT(':')) || (*psz == TEXT('\\')))
            pch = psz;
      }

    if (pch != sz)   /* If found slash or colon, return the next character */
        pch++;       /* increment OK, pch not pointing to DB character     */

    return(pch);
}

/* ** Enable or disable menu items according to selection state
      This routine is called when user tries to pull down a menu. */

VOID NpResetMenu( HWND hwnd )
{
    INT     mfcc;   /* menuflag for cut, copy  find, findnext */
    BOOL    fCanUndo;
    HMENU  hMenu;
    BOOL    fPaste= FALSE;
    UINT    uSelState;
    DWORD   dwSelStart;
    DWORD   dwSelEnd;

    hMenu = GetMenu(hwndNP);

    // cut, copy and delete only get enabled if there is text selected.

    SendMessage( hwndEdit, 
                 EM_GETSEL, 
                 (WPARAM) &dwSelStart,
                 (LPARAM) &dwSelEnd);

    mfcc= (dwSelStart == dwSelEnd ) ? MF_GRAYED : MF_ENABLED;

    EnableMenuItem(GetSubMenu(hMenu, 1), M_CUT, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_COPY, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CLEAR, mfcc);

    // check if the selectall is gray (that means the user has already
    // done select-all) and if the user has deselected - if so, time
    // to re-enable selectall menu.

    uSelState = GetMenuState(GetSubMenu(hMenu, 1), M_SELECTALL, MF_BYCOMMAND);
    if ((uSelState == MF_GRAYED) && (mfcc == MF_GRAYED))
    {
        EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_ENABLED);
    }

    // paste is enabled if there is text in the clipboard

    if( OpenClipboard(hwnd) )
    {
        fPaste= IsClipboardFormatAvailable(CF_TEXT);
        CloseClipboard();
    }
    EnableMenuItem(GetSubMenu(hMenu, 1), M_PASTE, fPaste ? MF_ENABLED : MF_GRAYED);

    // enable the find/find next iff there is some text in the edit window

    mfcc= MF_GRAYED;
    if( SendMessage( hwndEdit, WM_GETTEXTLENGTH, 0, 0 ) )
    {
        mfcc= MF_ENABLED;
    }

    EnableMenuItem( GetSubMenu(hMenu,1), M_FIND,			mfcc );
    EnableMenuItem( GetSubMenu(hMenu,1), M_FINDNEXT,		mfcc );
	EnableMenuItem( GetSubMenu(hMenu,1), M_FINDPREVIOUS,	mfcc ); // NotepadEx
	EnableMenuItem( GetSubMenu(hMenu,1), M_REPLACE,         mfcc ); // NotepadEx
	EnableMenuItem( GetSubMenu(hMenu,1), M_GOTO,			mfcc ); // NotepadEx

    // enable Undo only if editcontrol says we can do it.
    fCanUndo = (BOOL) SendMessage(hwndEdit, EM_CANUNDO, 0, 0L);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_UNDO, fCanUndo ? MF_ENABLED : MF_GRAYED);

    // check the word wrap item correctly
    CheckMenuItem(GetSubMenu(hMenu, 2), M_WW, fWrap ? MF_CHECKED : MF_UNCHECKED);

	// tab stops/tab width menu
	CheckMenuItem(GetSubMenu(hMenu, 2), M_TW2, iTabStops == 8 ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetSubMenu(hMenu, 2), M_TW4, iTabStops == 16 ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetSubMenu(hMenu, 2), M_TW8, iTabStops == 32 ? MF_CHECKED : MF_UNCHECKED);

    // check the status bar

    CheckMenuItem (GetSubMenu(hMenu, 3), M_STATUSBAR, fStatus ? MF_CHECKED: MF_UNCHECKED );
}

/* ** Scan sz1 for merge spec.    If found, insert string sz2 at that point.
      Then append rest of sz1 NOTE! Merge spec guaranteed to be two chars.
      returns TRUE if it does a merge, false otherwise. */
BOOL MergeStrings(
    TCHAR    *szSrc,
    TCHAR    *szMerge,
    TCHAR    *szDst)
    {
    register    TCHAR *pchSrc;
    register    TCHAR *pchDst;

    pchSrc = szSrc;
    pchDst = szDst;

    /* Find merge spec if there is one. */
    while ( *pchSrc != chMerge)
        {
        *pchDst++ = *pchSrc;

        /* If we reach end of string before merge spec, just return. */
        if (!*pchSrc++)
            return FALSE;

        }

    /* If merge spec found, insert sz2 there. (check for null merge string */
    if (szMerge)
        {
        while (*szMerge)
            *pchDst++ = *szMerge++;
        }

    /* Jump over merge spec */
    pchSrc++,pchSrc++;

    /* Now append rest of Src String */
    while (*pchDst++ = *pchSrc++);
    return TRUE;

    }

/* ** Post a message box */
INT FAR AlertBox(
    HWND    hwndParent,
    TCHAR    *szCaption,
    TCHAR    *szText1,
    TCHAR    *szText2,
    UINT     style)
{
    INT iResult;                      // result of function
    INT iAllocSize;                   // size needed for message
    TCHAR*  pszMessage;               // combined message

    // Allocate a message buffer assuming there will be a merge.
    // If we cannot do the allocation, tell the user something
    // related to the original problem. (not the allocation failure)
    // Then pray that MessageBox can get enough memory to actually work.

    iAllocSize= (lstrlen(szText1) + (szText2 ? lstrlen(szText2) : 0) + 1 ) * sizeof(TCHAR);

    pszMessage= (TCHAR*) LocalAlloc( LPTR, iAllocSize );

    if( pszMessage )
    {
        MergeStrings( szText1, szText2, pszMessage );
        iResult= MessageBox( hwndParent, pszMessage, szCaption, style );
        LocalFree( (HLOCAL) pszMessage );
    }
    else
    {
        iResult= MessageBox( hwndParent, szText1, szCaption, style );
    }

    return( iResult );
}

// SignalCommDlgError
//
// If a common dialog error occurred, put up reasonable message box.
//
// returns: TRUE if error occurred, FALSE if no error.
//

typedef struct tagMAPERROR
{
    DWORD   rc;            // return code from CommDlgExtendedError()
    PTCHAR* ppszMsg;       // text of message pointer
} MAPERROR;

// errors not in this list get generic "common dialog error %x" message.
static TCHAR* szNull= TEXT("");

MAPERROR maperror[]=
{
    CDERR_DIALOGFAILURE,  &szErrSpace,
    CDERR_INITIALIZATION, &szCommDlgInitErr,
    CDERR_MEMLOCKFAILURE, &szPDIE,
    CDERR_LOADSTRFAILURE, &szErrSpace,
    CDERR_FINDRESFAILURE, &szErrSpace,
    PDERR_LOADDRVFAILURE, &szLoadDrvFail,
    PDERR_GETDEVMODEFAIL, &szErrSpace,
    PDERR_NODEFAULTPRN,   &szNull,          // don't report; common dialog does already
};

BOOL SignalCommDlgError(VOID)
{
    DWORD rc;               // return code
    TCHAR* pszMsg;          // message
    INT    i;
    TCHAR  szBuf[200];      // just for common dialog failure

    rc= CommDlgExtendedError();

    // no failure - just return

    if( rc == 0 )
    {
        return FALSE;
    }

    // some sort of error - pick up message

    pszMsg= NULL;
    for( i=0; i< sizeof(maperror)/sizeof(maperror[0]); i++ )
    {
        if( rc == maperror[i].rc )
        {
            pszMsg= *maperror[i].ppszMsg;
        }
    }

    // if no known mapping - tell user the actual return code
    // this may be a bit confusing, but rare hopefully.

    if( !pszMsg )
    {
        _sntprintf(szBuf, sizeof(szBuf)/sizeof(TCHAR) -1, szCommDlgErr, rc);
        pszMsg= szBuf;
    }

    // popup if there is any message to give user

    if( *pszMsg )
    {
        MessageBox(hwndNP, pszMsg, szNN, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
    }

    return TRUE;

}

// ReplaceSel
//
// Replace the current selection with string from FR struct
// if the current selection matches our search string.
//
// MLE will show selection if bView is true.
//


VOID ReplaceSel( BOOL bView )
{
    DWORD StartSel;    // start of selected text
    DWORD EndSel;      // end of selected text

    HANDLE hEText;
    TCHAR* pStart;
    DWORD  ReplaceWithLength;  // length of replacement string
    DWORD  FindWhatLength;

    ReplaceWithLength= lstrlen(FR.lpstrReplaceWith);
    FindWhatLength= lstrlen(FR.lpstrFindWhat);

    SendMessage( hwndEdit, EM_GETSEL, (WPARAM) &StartSel, (LPARAM) &EndSel );
    hEText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hEText )  // silently return if we can't get it
    {
        return;
    }

    pStart= LocalLock( hEText );
    if( !pStart )
    {
        return;
    }

    if(  (EndSel-StartSel) == FindWhatLength )
    {
       if( (fCase &&
            !_tcsncmp(  FR.lpstrFindWhat, pStart+StartSel, FindWhatLength) ) ||
           (!fCase &&
           ( 2 == CompareString(LOCALE_USER_DEFAULT,
                  NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
                  FR.lpstrFindWhat, FindWhatLength,
                  pStart+StartSel,  FindWhatLength ) ) ) )
        {
            SendMessage( hwndEdit, EM_REPLACESEL,
                         TRUE, (LPARAM) FR.lpstrReplaceWith);
            SendMessage( hwndEdit, EM_SETSEL,
                         StartSel, StartSel+ReplaceWithLength );

            if( bView )
            {
                SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
            }
        }
    }

    LocalUnlock( hEText );
}

// GotoDlgProc
//
// Handle the Goto Dialog window processing
//
// Returns:
//
// 1 if successfull
// 0 if not (cancelled)
//
// Modifies global lGotoLine
//

const DWORD s_GotoHelpIDs[] = {
    IDC_GOTO, IDH_GOTO,
    0, 0
};

#define GOTOBUFSIZE 100
INT_PTR CALLBACK GotoDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    TCHAR szBuf[GOTOBUFSIZE];
    UINT LineNum;
    DWORD SelStart, SelEnd;

    switch (message)
    {
        //
        // initialize input field to size of file
        //
        case WM_INITDIALOG:
            SendMessage(hwndEdit,EM_GETSEL,(WPARAM) &SelStart,(WPARAM)&SelEnd);

            // the line numbers are 1 based instead 0 based. hence add 1.
            LineNum= (UINT)SendMessage( hwndEdit, EM_LINEFROMCHAR, SelStart, 0 ) + 1;
            wsprintf(szBuf, TEXT("%d"), LineNum);
            SetDlgItemText( hDlg, IDC_GOTO, szBuf );
            SetFocus( hDlg );
            return TRUE;
            break;

        // context sensitive help.
        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, szHelpFile,
                HELP_WM_HELP, (ULONG_PTR) (LPVOID) s_GotoHelpIDs);
            break;

        case WM_CONTEXTMENU:

            WinHelp((HWND) wParam, szHelpFile, HELP_CONTEXTMENU,
                (ULONG_PTR) (LPVOID) s_GotoHelpIDs);
            break;


        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                BOOLEAN Success;

                case IDC_GOTO:
                    return TRUE;
                    break;

                case IDOK:
                    GetDlgItemText( hDlg, IDC_GOTO, szBuf, GOTOBUFSIZE );

                    // convert all unicode numbers to range L'0' to L'9'

                    FoldString( MAP_FOLDDIGITS, szBuf, -1, szBuf, GOTOBUFSIZE);
                    lGotoLine= _ttol( szBuf );

                    //
                    // see if valid line number
                    //

					Success = GotoAndScrollInView(lGotoLine);

                    if (Success) {
                        EndDialog(hDlg, 0);  // successfull
                        return TRUE;
                    }

                    //
                    // Invalid line number - warn the user.
					// NotepadEx change: Use edit control balloon tips on Windows
					// versions that support it (XP and up).
                    //

					if (*(PULONG)0x7FFE026C > 6 || *(PULONG)0x7FFE0270 > 0) {
						EDITBALLOONTIP BalloonTip;

						BalloonTip.cbStruct	= sizeof(BalloonTip);
						BalloonTip.pszTitle	= NULL;
						BalloonTip.pszText	= szLineTooLarge;
						BalloonTip.ttiIcon	= TTI_NONE;
						Edit_ShowBalloonTip(GetDlgItem(hDlg, IDC_GOTO), &BalloonTip);
					} else {
						MessageBox(hDlg, szLineTooLarge, szLineError, MB_OK);
						SetFocus(hDlg);
					}

                    break;

                case IDCANCEL :
                    EndDialog(hDlg, 1 );   // cancelled
                    return TRUE;
                    break;

                default:

                    break;

            } // switch (wParam)

            break;

       default:

             break;

    } // switch (message)


    return FALSE;     // Didn't process a message


} // GotoDlgProc()


// This procedure looks at the iSize bytes pointed by lpFileContents and
// determines based on that the encoding of the file (Ansi, Unicode, Reverse Unicode, UTF8)


NP_FILETYPE fDetermineFileType(LPBYTE lpFileContents, UINT iSize)
{
    NP_FILETYPE ftFileType = FT_ANSI;
    LPTSTR szBuf = (LPTSTR) lpFileContents;

    if (iSize <= 1)
        return ftFileType;

    switch(*szBuf)        
    {
        // look for the standard BOMs.
        case BYTE_ORDER_MARK:
            ftFileType = FT_UNICODE;
            break;

        case REVERSE_BYTE_ORDER_MARK:
            ftFileType = FT_UNICODEBE;
            break;

        // UTF bom has 3 bytes.
        case BOM_UTF8_HALF:            
            if (iSize > 2 && ((BYTE) *((lpFileContents)+2) == BOM_UTF8_2HALF) )
            {
                ftFileType = FT_UTF8;
            }
            break;

        default:
            // Is the file unicode without BOM ?
            if (IsInputTextUnicode(lpFileContents, iSize))
            {
                ftFileType = FT_UNICODE;
            }      
            else
            {
                // Is the file UTF-8 even though it doesn't have UTF-8 BOM ?
                if (IsTextUTF8(lpFileContents, iSize))
                {
                    ftFileType = FT_UTF8;
                }
                // well, it is most likely an ansi file.
                else
                {
                    ftFileType = FT_ANSI;
                }
            }
            break;
        }             

    return ftFileType;

}