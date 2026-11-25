/*
	Tabulatorweite: 3
	Kommentare ab: Spalte 60											*Spalte 60*
*/

#include "VT52_GEM.H"
#include "DRAGDROP.H"

/*----------------------------------------------------------------------------------------*/
/* Drag & Drop - Pipe öffnen (für den Sender)															*/
/* Funktionsresultat:	Handle der Pipe, -1 für Fehler oder -2 für Fehler bei appl_write	*/
/*	app_id:					ID des Senders (der eigenen Applikation)									*/
/*	rcvr_id:					ID des Empfängers																	*/
/*	window:					Handle des Empfänger-Fensters													*/
/*	mx:						x-Koordinate der Maus beim Loslassen oder -1								*/
/*	my:						y-Koordinate der Maus beim Loslassen oder -1								*/
/*	kbstate:					Status der Kontrolltasten														*/
/*	format:					Feld für die max. 8 vom Empfänger unterstützten Formate				*/
/*	oldpipesig:				Zeiger auf den alten Signal-Dispatcher										*/
/*----------------------------------------------------------------------------------------*/
WORD	ddcreate( WORD	app_id, WORD rcvr_id, WORD window, WORD mx, WORD my, WORD kbstate, ULONG format[8], void **oldpipesig )
{
	BYTE	pipe[24];
	WORD	mbuf[8];
	LONG	handle_mask;
	WORD	handle, i;

	strcpy( pipe, "U:\\PIPE\\DRAGDROP.AA" );
	pipe[18] = 'A' - 1;

	do
	{
		pipe[18]++;															/* letzten Buchstaben weitersetzen */
		if ( pipe[18] > 'Z' )											/* kein Buchstabe des Alphabets? */
		{
			pipe[17]++;														/* ersten Buchstaben der Extension ändern */
			if ( pipe[17] > 'Z' )										/* ließ sich keine Pipe öffnen? */
				return( -1 );
		}

		handle = (WORD) Fcreate( pipe, 0x02 );						/* Pipe anlegen, 0x02 bedeutet, daß EOF zurückgeliefert wird, */
																				/* wenn die Pipe von niemanden zum Lesen geöffnet wurde */
	} while ( handle == EACCDN );

	if ( handle < 0 )														/* ließ sich die Pipe nicht anlegen? */
		return( handle );

	mbuf[0] = AP_DRAGDROP;												/* Drap&Drop-Message senden */
	mbuf[1] = app_id;														/* ID der eigenen Applikation */
	mbuf[2] = 0;
	mbuf[3] = window;														/* Handle des Fensters */
	mbuf[4] = mx;															/* x-Koordinate der Maus */
	mbuf[5] = my;															/* y-Koordinate der Maus */
	mbuf[6] = kbstate;													/* Tastatur-Status */
	mbuf[7] = (((WORD) pipe[17]) << 8 ) + pipe[18];				/* Endung des Pipe-Namens */

	if ( appl_write( rcvr_id, 16, mbuf ) == 0 )					/* Fehler bei appl_write()? */
	{
		Fclose( handle );													/* Pipe schließen */
		return( -2 );
	}

	handle_mask = 1L << handle;
	i = Fselect( DD_TIMEOUT, &handle_mask, 0L, 0L );			/* auf Antwort warten */

	if ( i && handle_mask )												/* kein Timeout? */
	{
		BYTE	reply;
		
		if ( Fread( handle, 1L, &reply ) == 1 )					/* Antwort vom Empfänger lesen */
		{
			if ( reply == DD_OK )										/* alles in Ordnung? */
			{
				if ( Fread( handle, DD_EXTSIZE, format ) == DD_EXTSIZE )	/* unterstützte Formate lesen */
				{
					*oldpipesig = (void *)(long) Psignal( SIGPIPE, (void *) SIG_IGN );	/* Dispatcher ausklinken */
					return( handle );
				}
			}
		}
	}

	Fclose( handle );														/* Pipe schließen */
	return( -1 );
}


/*----------------------------------------------------------------------------------------*/
/* Drag & Drop - überprüfen ob der Empfänger ein Format akzeptiert								*/
/* Funktionsresultat:	DD_OK: Empfänger untersützt das Format										*/
/*								DD_EXT: Empfänger akzeptiert das Format nicht							*/
/*								DD_LEN: Daten sind zu lang für den Empfänger								*/
/*								DD_NAK: Fehler bei Kommunikation												*/								
/*	handle:					Handle der Pipe																	*/
/*	format:					Kürzel für das Format															*/
/*	name:						Beschreibung des Formats als C-String										*/
/*	size:						Länge der zu sendenen Daten													*/
/*----------------------------------------------------------------------------------------*/
WORD	ddstry( WORD handle, ULONG format, BYTE *name, LONG size )
{
	LONG	str_len;
	WORD	hdr_len;
	
	str_len = strlen( name ) + 1;										/* Länge des Strings inklusive Nullbyte */
	hdr_len = 4 + 4 + (WORD) str_len;								/* Länge des Headers */

	if ( Fwrite( handle, 2, &hdr_len ) == 2 )						/* Länge des Headers senden */
	{
		LONG	written;
		
		written = Fwrite( handle, 4, &format );					/* Formatkürzel */
		written += Fwrite( handle, 4, &size );						/* Länge der zu sendenden Daten */
		written += Fwrite( handle, str_len, name );				/* Beschreibung des Formats als C-String */

		if ( written == hdr_len )										/* ließ sich der Header schreiben? */
		{
			BYTE	reply;
			
			if ( Fread( handle, 1, &reply ) == 1 )
				return( reply );											/* Antwort zurückliefern */
		}
	}	
	return( DD_NAK );
}

/*----------------------------------------------------------------------------------------*/
/* Drag & Drop - Pipe schließen																				*/
/*	handle:					Handle der Pipe																	*/
/* oldpipesig:				Zeiger auf den alten Signalhandler											*/
/*----------------------------------------------------------------------------------------*/
void	ddclose( WORD handle, void *oldpipesig )
{
	Psignal( SIGPIPE, oldpipesig );									/* wieder alten Dispatcher eintragen */
	Fclose( handle );														/* Pipe schließen */
}

/*----------------------------------------------------------------------------------------*/
/* Drag & Drop - Pipe öffnen (für den Empfänger)														*/
/* Funktionsresultat:	Handle der Pipe oder -1 (Fehler)												*/
/* pipe:						Zeiger auf den Namen der Pipe ("DRAGDROP.??")							*/
/* format:					Zeiger auf Array mit unterstützten Datenformaten						*/
/* oldpipesig:				Zeiger auf den Zeiger auf den alten Signalhandler						*/
/*----------------------------------------------------------------------------------------*/
WORD	ddopen( BYTE *pipe, ULONG format[8], void **oldpipesig )
{
	WORD	handle;
	BYTE	reply;

	handle = (WORD) Fopen( pipe, FO_RW );							/* Handle der Pipe erfragen	*/
	if ( handle < 0 )
		return( -1 );

	reply = DD_OK;															/* Programm unterstützt Drag & Drop	*/

	*oldpipesig = (void *)(long) Psignal( SIGPIPE, (void *) SIG_IGN );		/* Signal ignorieren	*/

	if ( Fwrite( handle, 1, &reply ) == 1 )
	{
		if ( Fwrite( handle, DD_EXTSIZE, format ) == DD_EXTSIZE )
			return( handle );
	}

	ddclose( handle, *oldpipesig );									/* Pipe schließen */
	return( -1 );
}

/*----------------------------------------------------------------------------------------*/
/* Header für Drag & Drop einlesen																			*/
/* Funktionsresultat:	0 Fehler 1: alles in Ordnung													*/
/*	handle:					Handle der Pipe																	*/
/* name:						Zeiger auf Array für den Datennamen											*/
/* format:					Zeiger auf ein Long, das das Datenformat anzeigt						*/
/* size:						Zeiger auf ein Long für die Länge der Daten								*/
/*----------------------------------------------------------------------------------------*/
WORD	ddrtry( WORD handle, BYTE *name, ULONG *format, LONG *size )
{
	WORD	hdr_len;

	if ( Fread( handle, 2, &hdr_len ) == 2 )						/* Headerlänge auslesen	*/
	{
		if ( hdr_len >= 9 )												/* kompletter Header?	*/
		{
			if ( Fread( handle, 4, format ) == 4 )					/* Datentyp auslesen	*/
			{
				if ( Fread( handle, 4, size ) == 4 )				/* Länge der Daten in Bytes auslesen */
				{	
					WORD	name_len;
					
					name_len = hdr_len -= 8;							/* Länge des Namens inklusive Nullbyte */

					if ( name_len > DD_NAMEMAX )				
						name_len = DD_NAMEMAX;

					if ( Fread( handle, name_len, name ) == name_len )	/* Datennamen auslesen	*/
					{
						BYTE	buf[64];
					
						hdr_len -= name_len;
	
						while ( hdr_len > 64 )							/* Headerrest auslesen	*/
						{
							Fread( handle, 64, buf );
							hdr_len -= 64;
						}
		
						if ( hdr_len > 0 )
							Fread( handle, hdr_len, buf );
	
						return( 1 );
					}
				}
			}
		}
	}
	return( 0 );															/* Fehler */
}

/*----------------------------------------------------------------------------------------*/
/* Meldung an den Drag & Drop - Initiator senden														*/
/* Funktionsresultat:	0: Fehler 1: alles in Ordnung													*/
/*	handle:					Handle der Pipe																	*/
/* msg:						Nachrichtennummer																	*/
/*----------------------------------------------------------------------------------------*/
WORD	ddreply( WORD handle, WORD msg )
{
	if ( Fwrite( handle, 1, ((BYTE *) &msg ) + 1 ) != 1 )		/* Fehler? */
	{
		Fclose( handle );													/* Pipe schließen */
		return( 0 );
	}
	return( 1 );
}
 