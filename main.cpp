#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <dsound.h>

//C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\dsound.h

#pragma comment(lib,"dsound.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"ws2_32.lib")

#define PRINT(val,type) printf("%s:\t" type "\n",#val,val);

const float pi=3.141592653589793;


void ds_check(HRESULT hr,const char *code,int line,const char *file){
	switch(hr){
	#define CASE(val) case val:{\
		fprintf(stderr,"%s = %s line %d, file %s\n",#val,code,line,file);\
		exit(1);\
	}
	case DS_OK: break;
	CASE(DS_NO_VIRTUALIZATION);
	CASE(DSERR_ALLOCATED);
	CASE(DSERR_CONTROLUNAVAIL);
	CASE(DSERR_INVALIDPARAM);
	CASE(DSERR_INVALIDCALL);
	CASE(DSERR_GENERIC);
	CASE(DSERR_PRIOLEVELNEEDED);
	CASE(DSERR_OUTOFMEMORY);
	CASE(DSERR_BADFORMAT);
	CASE(DSERR_UNSUPPORTED);
	CASE(DSERR_NODRIVER);
	CASE(DSERR_ALREADYINITIALIZED);
	CASE(DSERR_NOAGGREGATION);
	CASE(DSERR_BUFFERLOST);
	CASE(DSERR_OTHERAPPHASPRIO);
	CASE(DSERR_UNINITIALIZED);
	CASE(DSERR_NOINTERFACE);
	CASE(DSERR_ACCESSDENIED);
	CASE(DSERR_BUFFERTOOSMALL);
	CASE(DSERR_DS8_REQUIRED);
	CASE(DSERR_SENDLOOP);
	CASE(DSERR_BADSENDBUFFERGUID);
	CASE(DSERR_OBJECTNOTFOUND);
	CASE(DSERR_FXUNAVAILABLE);
	default: puts("unknown error");
	#undef CASE
	}
}

#define DS_CHECK(call) ds_check(call,#call,__LINE__,__FILE__)

LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam){
	switch(message){
		case WM_CLOSE: 
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,message,wparam,lparam);
}

typedef struct{
	uint32_t riff;
	uint32_t file_size;
	uint32_t magic;
	uint32_t fmt;
	uint32_t fmt_len;
	uint16_t fmt_type;
	uint16_t nchans;
	uint32_t samps_per_sec;
	uint32_t bytes_per_sec;
	uint16_t bytes_per_samp;
	uint16_t bits_per_mono_samp;
	uint32_t data_header;
	uint32_t data_size;
}wav_header;

static_assert(sizeof(wav_header)==44,"");

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,PSTR lpCmdLine, INT nCmdShow){
	AttachConsole(-1);
	freopen("CONIN$","r",stdin);
	freopen("CONOUT$","w",stdout);
	freopen("CONOUT$","w",stderr);
	puts("");

	// test pipes
	// puts("stdout");
	// fputs("stderr\n",stderr);

	bool gen_wave=false;
	enum wave_type{
		WT_SQUARE,
		WT_TRI,
		WT_SAW,
		WT_SINE,
	};

	wave_type wave=WT_SINE;

	uint16_t nchans=2;
	uint32_t samps_per_sec=32000;
	uint16_t bits_per_mono_samp=16;
	uint64_t data_size=7680000;
	int16_t *data_ptr=NULL;

	puts(lpCmdLine);
	HANDLE file=CreateFile(lpCmdLine,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
	if(file!=INVALID_HANDLE_VALUE){

		uint64_t file_size;
		static_assert(sizeof(_LARGE_INTEGER)==8,"");
		assert(GetFileSizeEx(file,(_LARGE_INTEGER*)&file_size));

		char *raw_file=(char*)malloc(file_size);

		unsigned long read_size;
		assert(ReadFile(file,raw_file,file_size,&read_size,0));
		assert(read_size==file_size);

		wav_header *head=(wav_header*)raw_file;
		assert(head->riff==htonl('RIFF'));
		assert(head->file_size+offsetof(wav_header,magic)==file_size);
		assert(head->magic==htonl('WAVE'));
		assert(head->fmt_len==offsetof(wav_header,fmt_len));
		assert(head->fmt_type==1);
		assert(head->nchans<=2);

		data_size=file_size-sizeof(wav_header);
		data_ptr=(int16_t*)(raw_file+sizeof(wav_header));

		nchans=head->nchans;
		samps_per_sec=head->samps_per_sec;
		bits_per_mono_samp=head->bits_per_mono_samp;

	}else{
		gen_wave=true;
		if(strcmp(lpCmdLine,"-sine")==0) wave=WT_SINE;
		else if(strcmp(lpCmdLine,"-saw")==0) wave=WT_SAW;
		else if(strcmp(lpCmdLine,"-square")==0) wave=WT_SQUARE;
		else if(strcmp(lpCmdLine,"-tri")==0) wave=WT_SQUARE;
		else{
			puts("invalid argument");
			puts("Usage:\n\twav_player file.wav");
			return 1;
		}
	}

	WNDCLASS wndclass={0};
	wndclass.style        =CS_OWNDC;
	wndclass.lpfnWndProc  =WndProc;
	wndclass.hInstance    =hInstance;
	wndclass.hCursor      =LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszClassName="window_class";

	assert(RegisterClass(&wndclass));
	HWND hwnd=CreateWindow(
		"window_class",
		"wav player ._____.",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		300,
		20,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	assert(hwnd);
	ShowWindow(hwnd,nCmdShow);

	struct IDirectSound *ds;
	DS_CHECK(DirectSoundCreate(NULL,&ds,NULL));
	DS_CHECK(ds->SetCooperativeLevel(hwnd,DSSCL_PRIORITY));

	DSBUFFERDESC pri_dsbufdesc={0};
	pri_dsbufdesc.dwSize=sizeof(DSBUFFERDESC);
	pri_dsbufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
	pri_dsbufdesc.dwBufferBytes=0;
	pri_dsbufdesc.dwReserved=0;
	pri_dsbufdesc.lpwfxFormat=NULL;

	struct IDirectSoundBuffer *pri_dsbuf;
	DS_CHECK(ds->CreateSoundBuffer(&pri_dsbufdesc,&pri_dsbuf,NULL));

	WAVEFORMATEX wavfmt={0};
	wavfmt.wFormatTag=WAVE_FORMAT_PCM;    
	wavfmt.nChannels=nchans;     
	wavfmt.nSamplesPerSec=samps_per_sec;
	wavfmt.wBitsPerSample=bits_per_mono_samp;
	wavfmt.nBlockAlign=(wavfmt.wBitsPerSample*wavfmt.nChannels)/8;   
	wavfmt.nAvgBytesPerSec=wavfmt.nBlockAlign*wavfmt.nSamplesPerSec;
	wavfmt.cbSize=0;        

	DS_CHECK(pri_dsbuf->SetFormat(&wavfmt));
	
	DSBUFFERDESC sec_dsbufdesc={0};
	sec_dsbufdesc.dwSize=sizeof(DSBUFFERDESC);
	sec_dsbufdesc.dwFlags=0;
	sec_dsbufdesc.dwBufferBytes=data_size;
	sec_dsbufdesc.dwReserved=0;
	sec_dsbufdesc.lpwfxFormat=&wavfmt;

	struct IDirectSoundBuffer *sec_dsbuf;
	DS_CHECK(ds->CreateSoundBuffer(&sec_dsbufdesc,&sec_dsbuf,NULL));

	unsigned long play_cursor;
	unsigned long write_cursor;
	DS_CHECK(sec_dsbuf->GetCurrentPosition(&play_cursor,&write_cursor));

	void *reg_ptrs[2]={NULL,NULL};
	unsigned long reg_sizes[2];
	DS_CHECK(sec_dsbuf->Lock(
		0,
		sec_dsbufdesc.dwBufferBytes,
		&reg_ptrs[0],
		&reg_sizes[0],
		&reg_ptrs[1],
		&reg_sizes[1],
		DSBLOCK_ENTIREBUFFER
	));

	assert(reg_ptrs[0]);

	if(gen_wave){
		int smp_per_sec=wavfmt.nSamplesPerSec;
		int cyc_per_sec=432/2;
		int smp_per_cyc=smp_per_sec/cyc_per_sec;

		int16_t vol=0x7fff;
		for(int j=0;j<1;j++){
			typedef struct{
				uint16_t left;
				uint16_t right;
			}sample;
			sample *buf=(sample*)reg_ptrs[j];
			if(buf==NULL) break;
			for(int i=0;i<reg_sizes[j]/4;i++){
				int16_t val;
				switch(wave){
				case WT_SQUARE:
					val=((i%smp_per_cyc)>(smp_per_cyc/2))?vol:-vol;
					break;
				case WT_SINE:
					val=vol*sinf(2*pi*i/(float)smp_per_cyc);
					break;
				case WT_SAW:
					val=(2*vol*((i%smp_per_cyc)-smp_per_cyc/2))/smp_per_cyc;
					break;
				case WT_TRI:
					int16_t k=i%smp_per_cyc;
					int16_t half_period=smp_per_cyc/2;
					if(k>=half_period){
						k-=half_period;
						k=half_period-k;
					}
					val=(2*vol*(k-half_period/2))/half_period;
					break;
				}
				buf[i].left=val;
				buf[i].right=val;
			}
		}
	}else{
		memcpy(reg_ptrs[0],data_ptr,reg_sizes[0]);
	}
	
	DS_CHECK(sec_dsbuf->Unlock(reg_ptrs[0],reg_sizes[0],reg_ptrs[1],reg_sizes[1]));

	DS_CHECK(sec_dsbuf->Play(0,0,DSBPLAY_LOOPING));
   
	MSG msg;
	while(GetMessage(&msg,NULL,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	puts("done");
}
