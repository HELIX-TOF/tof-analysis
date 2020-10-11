// This is a real simple-minded reader / demo for TOF test data

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <iostream>
#include <TTree.h>
#include <TFile.h>

#define NSAMPLES 28   // for instance, this is run001.dat's NSAMPLES

// this will (probably) change for zero suppressed data later on
// (or maybe can keep same and it's further encoded in flags array ?)
struct data {
  uint16_t marker;          // always 0xda7a
  uint16_t evn;             // event number in hardware (reset when?)
  uint16_t ctime_sys;       // coarse timestamp
  uint16_t prog_header;     // programmable header word (reg 12)
  uint16_t ctime_tof;       // coarse timestamp in TOFCLK (i.e. ADC sample index) use this one for analysis!
  uint16_t adc[16][NSAMPLES];   // unpacked from hardware ADC words
  uint16_t flags[16][NSAMPLES]; // unpacked from hardware ADC words
  uint16_t t1;   // always 0x0000 ?
  uint16_t t2;   // always 0x0000 ?
  uint16_t length;   // some length count, but unsure about exact meaning or stability for the time being
  uint16_t endmarker;   // always 0xf007
} ;

struct tof_evt {
  uint16_t marker;          // always 0x5446 "TF"
  uint16_t hw_nbytes;  // byte count of data from the hardware. This should be even, but don't crash if it isn't.
  // the byte count should always be the right value as well -- check, drop bad events
  uint32_t evn;        // event number filled in by software, 0 = 1st event of the run
  struct data data;
  // note: padding will be present here in the file if necessary to make total size a multiple of 4 bytes
} ;

// check return value when using -- data will be filled in with garbage when it fails!!
int readevent(FILE *file, struct tof_evt *p) {
  uint8_t buf[3000];
  uint16_t x;
  int dropit=0,i,j;

  while (1) {
    // first get just the header, to be able to find out how much more to get
    if (fread(buf,1,8,file)!=8)
      return -1;
    if ((p->marker=*((uint16_t *) buf)) != 0x5446) {
      fprintf(stderr,"bad marker 0x%04x, dropping event\n",p->marker);
      dropit=1;
    }
    if ((p->hw_nbytes=*((uint16_t *) (buf+2))) != 2*(16*NSAMPLES+9) ) {
      fprintf(stderr,"bad hw length %d, expected %d, dropping event\n",p->hw_nbytes,2*(16*NSAMPLES+9));
      dropit=1;
    }
    p->evn=*((uint32_t *) (buf+4));

    if (fread(buf+8,1,(p->hw_nbytes+3)/4*4,file)!=(p->hw_nbytes+3)/4*4)
      return -1;

    if (dropit) {  // from the checks before we read the hw part of the data, see above
      dropit=0;
      continue;    // better luck on next event...
    }

    if ((p->data.marker = *((uint16_t *) (buf+8)))!=0xda7a) {
      fprintf(stderr,"bad hw marker 0x%04x, dropping event\n",p->data.marker);
      continue;
    }
    p->data.evn = *((uint16_t *) (buf+10));
    p->data.ctime_sys = *((uint16_t *) (buf+12));
    p->data.prog_header = *((uint16_t *) (buf+14));
    p->data.ctime_tof = *((uint16_t *) (buf+16));

    for(i=0;i<16;i++)
      for(j=0;j<NSAMPLES;j++) {
	x = *((uint16_t *) (buf+18+2*(NSAMPLES*i+j)));
	p->data.adc[i][j] = 16384 - (x & 0x3fff);   // restore correct polarity here (maybe we move this to firmware????)
	p->data.flags[i][j] = (x & 0xc000)>>14;
      }

    p->data.t1 = *((uint16_t *) (buf+18+2*(NSAMPLES*16)));
    p->data.t2 = *((uint16_t *) (buf+18+2*(NSAMPLES*16)+2));
    p->data.length = *((uint16_t *) (buf+18+2*(NSAMPLES*16)+4));
    if ((p->data.endmarker = *((uint16_t *) (buf+18+2*(NSAMPLES*16)+6))) != 0xf007) {
      fprintf(stderr,"bad hw end marker 0x%04x, dropping event\n",p->data.endmarker);
      continue;
    }

    break;  // it was a good event and we're done with it
  }
  return 0;

}

int main (int argc, const char *argv[]) 
{
  int i,j;
  
	//get input file name
  if (argc < 2) 
  {
    std::cout << "Usage: tof2root run001.dat" << std::endl;
    std::cout << "Output file will be named run001.root" << std::endl;
    exit(1);
  }
	std::string outfname = argv[1];
  outfname.erase(outfname.length()-3,3);
	outfname.append("root");
  std::cout << "Output file will be named " << outfname << std::endl;	
	
	//open input file
	FILE *inpf;
	inpf = fopen(argv[1],"r");
	
	if(inpf == NULL) 
	{
		std::cout << "Unable to open input file " << argv[1] << std::endl;
    exit(1);
	}
	else
		std::cout << "Opening input file " << argv[1] << std::endl;
	
	
  //output ROOT file, static for now
  TFile *outfile = new TFile(outfname.c_str(),"RECREATE");
  
  //ROOT tree to put in file
  TTree *T = new TTree("T","HELIX TOF output tree");
  
  //variables for ROOT tree (subset fo the variables from the structs above)
  int evn;             // event number in hardware (reset when?)
  int ctime_sys;       // coarse timestamp
  int ctime_tof;       // coarse timestamp in TOFCLK (i.e. ADC sample index) use this one for analysis!
  int adc[16][NSAMPLES];   // unpacked from hardware ADC words
  int flags[16][NSAMPLES]; // unpacked from hardware ADC words
  int length;   // some length count, but unsure about exact meaning or stability for the time being
  
  //branches for the ROOT tree
  T->Branch("evn",&evn,"evn/I");
  T->Branch("ctime_sys",&ctime_sys,"ctime_sys/I");
  T->Branch("ctime_tof",&ctime_tof,"ctime_tof/I");
  T->Branch("adc",&adc,"adc[16][28]/I");//change the second index if necessary
  T->Branch("flags",&flags,"flags[16][28]/I");//change the second index if necessary
  T->Branch("length",&length,"length/I");

  struct tof_evt x;

 // while (!readevent(stdin,&x)) {
  while (!readevent(inpf,&x)) {
	evn = x.evn;
	ctime_sys = x.data.ctime_sys;
	ctime_tof = x.data.ctime_tof;
	length = x.data.length;
		
	for(i=0;i<16;i++){
		for(j=0;j<NSAMPLES;j++){
			//printf("%d %d %d %d\n",x.evn,i,j,x.data.adc[i][j]);
		  adc[i][j]=x.data.adc[i][j];
		  flags[i][j]=x.data.flags[i][j];
	  }	
    //printf("\n\n");
  }

//	if (x.evn>10)  break;
    T->Fill();
  }

  outfile->Write();
  outfile->Close();
	
  fclose (inpf);	

  return 0;
}
