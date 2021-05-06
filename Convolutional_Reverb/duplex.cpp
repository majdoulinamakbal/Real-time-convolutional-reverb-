/******************************************/
/*
  duplex.cpp
  by Gary P. Scavone, 2006-2007.

  This program opens a duplex stream and passes
  input directly through to the output.
*/
/******************************************/

#include "RtAudio.h"
#include "somefunc.cpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#define _USE_MATH_DEFINES
#include <cmath>
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


typedef double MY_TYPE;
#define FORMAT RTAUDIO_FLOAT64

// On définit la structure data_t , qu'on va instancier dans le main et appeler dans la fonction callback 
typedef struct{
    int buffer_size;//bufferbytes
    int size_conv;//taille de la convolution (M+L-1)
    std::vector<MY_TYPE> M;//réponse impulsionelle
    // MY_TYPE* convolution;//résultat de la convolution temporelle
    MY_TYPE* overlap;//overlap
    int sizefft;//taille de la fft (get_nextpow2(M+L-1))
    int type_conv;//type de la convolution (0 pour fréquentielle 1 pour temporelle)
    MY_TYPE* fftinput_r;//les parties réelles et imaginaires de la fft du signal
    MY_TYPE* fftinput_i;
    MY_TYPE* fftimp_r;//les parties réelles et imaginaires de la fft de la réponse imp
    MY_TYPE* fftimp_i;
    MY_TYPE* multiplication_r;//les parties réelles et imaginaires de la multiplication des fft de (la réponse imp et le signal)
    MY_TYPE* multiplication_i;
    MY_TYPE* ifft_r;//les parties réelles et imaginaires de la ifft 
    MY_TYPE* ifft_i;
    MY_TYPE* Y;//vecteur des (M+L-1) premiers element de la partie réelle de la ifft
  }data_t;


void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage: duplex N fs <iDevice> <oDevice> <iChannelOffset> <oChannelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    iDevice = optional input device to use (default = 0),\n";
  std::cout << "    oDevice = optional output device to use (default = 0),\n";
  std::cout << "    iChannelOffset = an optional input channel offset (default = 0),\n";
  std::cout << "    and oChannelOffset = optional output channel offset (default = 0).\n\n";
  exit( 0 );
}

int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *data )
{

	int i, j;
  int kmin,kmax;
	float tmp;
  

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;
  //instanciation
  //paramètres temporels

  data_t* donnees=(data_t*)data;
  unsigned int *bytes = (unsigned int *) (&(donnees->buffer_size));
  std::vector<MY_TYPE> impulse=donnees->M;
  int Size=donnees->buffer_size;
  int imp_Size=impulse.size();
  int nconv = donnees->size_conv;
  MY_TYPE* Last=donnees->overlap;
  MY_TYPE* C=(MY_TYPE*) calloc(nconv, sizeof(MY_TYPE));

  //paramètres fréquentiels
  int taille_fft=donnees->sizefft;
  MY_TYPE* buffer2= (MY_TYPE*) calloc(Size, sizeof(MY_TYPE));
  MY_TYPE* buffer_Re= (MY_TYPE*) calloc(taille_fft, sizeof(MY_TYPE));
  MY_TYPE* buffer_Im= (MY_TYPE*) calloc(taille_fft, sizeof(MY_TYPE));
  MY_TYPE* Imp_Im= (MY_TYPE*) calloc(taille_fft, sizeof(MY_TYPE));
  MY_TYPE* Y_IFFT_r= (MY_TYPE*) calloc(taille_fft, sizeof(MY_TYPE));
  MY_TYPE* Y_IFFT_i= (MY_TYPE*) calloc(taille_fft, sizeof(MY_TYPE));
  MY_TYPE* Imp_r= (MY_TYPE*) calloc(taille_fft, sizeof(MY_TYPE));
  for(i=0;i<impulse.size();i++){
    Imp_r[i]=impulse[i];
  }
  MY_TYPE* resultat=donnees->Y;
  int s=donnees->type_conv;

  
  
  if(s==1){
  // Copier inputbuffer dans buffer2
  memcpy(buffer2,inputBuffer,*bytes);

    //convolution process
	for (i=0; i<nconv; i++)
	{
    tmp = 0;
    if (i>imp_Size){
      kmin = i-imp_Size;

    }
    else{
      kmin=0;
    }
    if (i<Size){
      kmax = i;

    }
    else{
      kmax=Size;
    }		
		for (j=kmin; j<kmax; j++){
      
      tmp = tmp + impulse[i-j]*buffer2[j];     
		}
    C[i] = tmp;    

	}

  //filtrage temporelle : implementation de overlape-add
  
  for (i = 0; i <imp_Size; i = i + 1) {
      
      C[i] = C[i] + Last[i];
  }
  for (i = 0; i <imp_Size; i = i + 1) { 
      Last[i] = C[i+Size];
  }


  memcpy(outputBuffer, C, *bytes);

  std::cout << get_process_time() << std::endl;

  return 0;



  
  }
  else{
  //Convolution frequentielle

  memcpy(buffer_Re,inputBuffer,*bytes);
  fftr(buffer_Re,buffer_Im,taille_fft);
  fftr(Imp_r,Imp_Im,taille_fft);
  for(i=0;i<taille_fft;i++){
    Y_IFFT_r[i]=buffer_Re[i]*Imp_r[i];
    Y_IFFT_i[i]=buffer_Im[i]*Imp_Im[i];
  }
  ifftr(Y_IFFT_r,Y_IFFT_i,taille_fft);
  for(i=0;i<nconv;i++){
    resultat[i]=Y_IFFT_r[i];
   
  }
  memcpy(outputBuffer,resultat,*bytes);
  std::cout << get_process_time() << std::endl;


  return 0;

  }

}
	


int main(int argc, char *argv[])
{
  unsigned int channels, fs,type_conv,buffer_size,imp_size, bufferBytes, oDevice = 0, iDevice = 0, iOffset = 0, oOffset = 0;
  // Minimal command-line checking
  if (argc < 3 || argc > 7 ) usage();

  RtAudio adac;
  if ( adac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  type_conv = (unsigned int) atoi(argv[3]);
  imp_size = (unsigned int) atoi(argv[4]);
  buffer_size = (unsigned int) atoi(argv[5]);

  channels = (unsigned int) atoi(argv[1]);
  fs = (unsigned int) atoi(argv[2]);
  if ( argc > 6 )
    iDevice = (unsigned int) atoi(argv[6]);
  if ( argc > 7 )
    oDevice = (unsigned int) atoi(argv[7]);
  if ( argc > 8 )
    iOffset = (unsigned int) atoi(argv[8]);
  if ( argc > 9 )
    oOffset = (unsigned int) atoi(argv[9]);

  // Let RtAudio print messages to stderr.
  adac.showWarnings( true );

  // Set the same number of channels for both input and output.
  unsigned int bufferFrames = buffer_size;
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = iDevice;
  iParams.nChannels = channels;
  iParams.firstChannel = iOffset;
  oParams.deviceId = oDevice;
  oParams.nChannels = channels;
  oParams.firstChannel = oOffset;
  
  if ( iDevice == 0 )
    iParams.deviceId = adac.getDefaultInputDevice();
  if ( oDevice == 0 )
    oParams.deviceId = adac.getDefaultOutputDevice();

  RtAudio::StreamOptions options;
  //options.flags |= RTAUDIO_NONINTERLEAVED;
  bufferBytes = bufferFrames * channels * sizeof( MY_TYPE );
  //récuperer la réponse impulsionelle
  std::ifstream myfile("/Users/mac/Downloads/ressources_tstr_v1_1 2/c/imp.txt");
  std::vector<MY_TYPE> doubles;
  //MY_TYPE *C;
  //stocker la réponse impulsionelle dans le vecteur doubles
  for (double a; myfile >> a;) {
    doubles.push_back(a);
    }
  doubles.resize(imp_size);
  //initialisation des variables et allocations de mémoire
  int nconv=doubles.size()+bufferBytes-1;
  int taille_fft=get_nextpow2(nconv);//taille de fft
  //C = (MY_TYPE*) calloc(nconv, sizeof(MY_TYPE));
  MY_TYPE* Last= (MY_TYPE*) calloc(nconv, sizeof(MY_TYPE));
  MY_TYPE* resultat= (MY_TYPE*) calloc(nconv, sizeof(MY_TYPE));

  
  data_t data;
  data.buffer_size=bufferBytes;
  data.M=doubles;
  //data.convolution=C;
  data.size_conv=nconv;
  data.overlap=Last;
  data.sizefft=taille_fft;
  data.Y=resultat;
  data.type_conv=type_conv;

  
  try {
    adac.openStream( &oParams, &iParams, FORMAT, fs,&bufferFrames, &inout, (void *)&data, &options) ;
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    exit( 1 );
  }

  // Test RtAudio functionality for reporting latency.

  std::cout << "\nStream latency = " << adac.getStreamLatency() << " frames" << std::endl;


  try {
    std::cout << options.numberOfBuffers << std::endl;
    adac.startStream();


    char input;

    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << bufferFrames << ").\n";

    std::cin.get(input);
    


    // Stop the stream.
    adac.stopStream();

  }

  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

 cleanup:
  if ( adac.isStreamOpen() ) adac.closeStream();

  return 0;
}
